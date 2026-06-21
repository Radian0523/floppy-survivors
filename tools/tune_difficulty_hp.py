"""Tune game difficulty by directly scaling enemy HPs until bot win rate hits target.

Why a separate tool: tune.py (bayes) searches 8 dimensions and tends to find
local minima that don't actually hit the target. This script instead does ONE
thing: scale every enemy type's HP up/down until bot win rate ≈ --target-winrate.

Method (direct iterative):
  1. Run N games at current HP scale.
  2. Compute observed win rate.
  3. If too easy → multiply all enemy HPs (BIT/FRAGMENT/PACKET/GLITCH/SPLITTER/
     BOMBER/SWARM/PHASER/TRACKER + BOSS) by a scaling factor toward target.
  4. Write back to src/config.h, rebuild, repeat.
  5. Converge when within tolerance.

Usage:
    python tools/tune_difficulty_hp.py                    # target 50% win
    python tools/tune_difficulty_hp.py --target-winrate 0.4 --runs 15
    python tools/tune_difficulty_hp.py --dry-run
"""
import argparse
import concurrent.futures
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).parent.parent
CONFIG_H = ROOT / "src" / "config.h"
EXE = ROOT / "disk_survivor"

# Enemy HP constants to scale together. BOSS_HP scales too but at a softer rate
# so boss isn't proportionally as tanky as fodder.
ENEMY_HP_DEFINES = [
    "BIT_HP", "FRAGMENT_HP", "PACKET_HP", "GLITCH_HP",
    "SPLITTER_HP", "BOMBER_HP", "SWARM_HP",
    "PHASER_HP", "TRACKER_HP",
]
BOSS_HP_DEFINE = "BOSS_HP"


def read_hp(define):
    text = CONFIG_H.read_text()
    m = re.search(rf"#define\s+{define}\s+(\d+)", text)
    if not m:
        print(f"ERROR: {define} not found")
        sys.exit(1)
    return int(m.group(1))


def write_hp(define, value):
    text = CONFIG_H.read_text()
    text = re.sub(
        rf"(#define\s+{define}\s+)\d+",
        rf"\g<1>{value}",
        text,
    )
    CONFIG_H.write_text(text)


def rebuild():
    r = subprocess.run(["make", "mac"], cwd=ROOT, capture_output=True, text=True)
    if r.returncode != 0:
        print("BUILD FAILED:")
        print(r.stderr[-800:])
        sys.exit(1)


def run_once(seed, duration, tmpdir):
    out_path = Path(tmpdir) / f"run_{seed}.txt"
    cmd = [str(EXE), "--headless", "--bot", f"--seed={seed}",
           f"--output={out_path}", f"--duration={duration}"]
    subprocess.run(cmd, check=True, capture_output=True, cwd=ROOT)
    stats = {}
    for line in out_path.read_text().splitlines():
        if "=" in line:
            k, v = line.split("=", 1)
            try:
                stats[k] = float(v)
            except ValueError:
                stats[k] = v
    return stats


def measure(runs, duration, workers):
    """Return (win_rate, level_mean, boss_kill_rate)."""
    results = []
    with tempfile.TemporaryDirectory() as td:
        with concurrent.futures.ThreadPoolExecutor(max_workers=workers) as ex:
            futs = [ex.submit(run_once, s, duration, td) for s in range(runs)]
            for f in concurrent.futures.as_completed(futs):
                results.append(f.result())
    n = len(results)
    survived = sum(int(r.get("survived", 0)) for r in results)
    boss = sum(int(r.get("boss_defeated", 0)) for r in results)
    level_mean = sum(r.get("level", 0) for r in results) / n
    return survived / n, level_mean, boss / n


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--runs", type=int, default=12)
    ap.add_argument("--iters", type=int, default=5)
    ap.add_argument("--target-winrate", type=float, default=0.5,
                    help="Target bot win rate (default 0.5 = 50%%)")
    ap.add_argument("--duration", type=float, default=300.0)
    ap.add_argument("--workers", type=int, default=os.cpu_count() or 4)
    ap.add_argument("--tolerance", type=float, default=0.10,
                    help="Acceptable winrate error (default 0.10 = within 10ppt)")
    ap.add_argument("--max-step", type=float, default=1.6,
                    help="Biggest per-iter HP scaling (default 1.6x)")
    ap.add_argument("--boss-scale-ratio", type=float, default=0.6,
                    help="Boss HP scales at this fraction of mob scaling "
                         "(0.6 = boss gets buffed 60%% as hard as fodder)")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    if not EXE.exists():
        print(f"Build first. Missing: {EXE}")
        return 1

    # Snapshot current HPs
    orig = {d: read_hp(d) for d in ENEMY_HP_DEFINES}
    orig_boss = read_hp(BOSS_HP_DEFINE)
    print(f"Starting HPs: {orig}")
    print(f"Starting BOSS_HP: {orig_boss}")
    print(f"Target win rate: {args.target_winrate:.2f} (±{args.tolerance:.2f})")
    print()

    current = dict(orig)
    current_boss = orig_boss
    best_err = float("inf")
    best_snapshot = (dict(orig), orig_boss)

    for it in range(args.iters):
        rebuild()
        wr, level_mean, boss_rate = measure(
            args.runs, args.duration, args.workers)
        err = abs(wr - args.target_winrate)
        sample = {d: current[d] for d in ["BIT_HP", "PACKET_HP", "BOSS_HP" if False else "PHASER_HP"]}
        print(f"iter {it}: "
              f"BIT={current['BIT_HP']:2d} PACKET={current['PACKET_HP']:2d} "
              f"PHASER={current['PHASER_HP']:2d} BOSS={current_boss:3d}  "
              f"win%={wr*100:5.1f}  lvl={level_mean:5.1f}  "
              f"boss%={boss_rate*100:4.0f}  err={err*100:4.1f}ppt")

        if err < best_err:
            best_err = err
            best_snapshot = (dict(current), current_boss)

        if err <= args.tolerance:
            print("Converged.")
            break

        # Scale: too-high winrate -> increase HP (multiply > 1)
        #        too-low  winrate -> decrease HP (multiply < 1)
        # Use a saturating scale derived from how far off we are.
        if wr > args.target_winrate:
            # 1.0 (already at target) -> 1.0 ; 1.0 winrate -> max_step
            t = (wr - args.target_winrate) / (1.0 - args.target_winrate + 1e-6)
            scale = 1.0 + (args.max_step - 1.0) * min(1.0, max(0.0, t))
        else:
            t = (args.target_winrate - wr) / (args.target_winrate + 1e-6)
            scale = 1.0 / (1.0 + (args.max_step - 1.0) * min(1.0, max(0.0, t)))

        # Apply scale to all mob HPs (rounded, min 1)
        for d in ENEMY_HP_DEFINES:
            new_v = max(1, int(round(current[d] * scale)))
            current[d] = new_v
        # Boss scales softer so it doesn't become a wall
        boss_scale = 1.0 + (scale - 1.0) * args.boss_scale_ratio
        current_boss = max(20, int(round(current_boss * boss_scale)))

        print(f"  -> scale={scale:.3f}  applying...")

        if args.dry_run:
            print("--dry-run: not writing config.h")
            # Restore origs so the file isn't touched even on the iter 0 write
            for d, v in orig.items():
                write_hp(d, v)
            write_hp(BOSS_HP_DEFINE, orig_boss)
            return 0

        for d, v in current.items():
            write_hp(d, v)
        write_hp(BOSS_HP_DEFINE, current_boss)

    # Write back the best snapshot
    best_hps, best_boss = best_snapshot
    for d, v in best_hps.items():
        write_hp(d, v)
    write_hp(BOSS_HP_DEFINE, best_boss)
    rebuild()
    print()
    print(f"Final: BIT_HP={best_hps['BIT_HP']}, PACKET_HP={best_hps['PACKET_HP']}, "
          f"BOSS_HP={best_boss}  (err {best_err*100:.1f}ppt)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
