"""Balance per-weapon damage so each weapon contributes a similar share in
a bot run with all 9 weapons unlocked.

Method: direct iterative scaling
  1. Run N bot games with --all-weapons.
  2. Sum per-weapon damage_dealt over runs.
  3. Target = median of weapon DPS across the 9 weapons.
  4. For each weapon: new_damage = round(old_damage * target / weapon_dps).
  5. Write back to src/config.h, rebuild, repeat.

After 2-3 iterations weapons converge to a similar effective DPS share.
The bot's playstyle is held constant, so this measures "weapon effectiveness
when the player moves like a typical human" — perfect for relative balance.

Usage:
    python tools/tune_weapons.py                    # default 12 runs, 3 iters
    python tools/tune_weapons.py --runs 20 --iters 5
    python tools/tune_weapons.py --duration 180     # shorter games (faster)
    python tools/tune_weapons.py --dry-run          # measure only, no edit
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

# Maps WeaponID (matches enum order in game.h) to its damage constant in config.h
WEAPON_DEFINES = [
    ("PULSE",     "BULLET_DAMAGE"),
    ("ORBITERS",  "ORBITER_DAMAGE"),
    ("BEAM",      "BEAM_DAMAGE"),
    ("NOVA",      "NOVA_DAMAGE"),
    ("MINES",     "MINE_DAMAGE"),
    ("CHAIN",     "CHAIN_DAMAGE"),
    ("BOOMERANG", "BOOMERANG_DAMAGE"),
    ("TRAIL",     "TRAIL_DAMAGE"),
    ("WHIP",      "WHIP_DAMAGE"),
]


def read_damages():
    text = CONFIG_H.read_text()
    out = []
    for name, define in WEAPON_DEFINES:
        m = re.search(rf"#define\s+{define}\s+(\d+)", text)
        if not m:
            print(f"ERROR: {define} not found in config.h")
            sys.exit(1)
        out.append(int(m.group(1)))
    return out


def write_damages(new):
    text = CONFIG_H.read_text()
    for (name, define), val in zip(WEAPON_DEFINES, new):
        text = re.sub(
            rf"(#define\s+{define}\s+)\d+",
            rf"\g<1>{val}",
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
    cmd = [
        str(EXE), "--headless", "--bot", "--all-weapons",
        f"--seed={seed}", f"--output={out_path}",
        f"--duration={duration}",
    ]
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
    """Return (per-weapon DPS list, mean duration)."""
    results = []
    with tempfile.TemporaryDirectory() as td:
        with concurrent.futures.ThreadPoolExecutor(max_workers=workers) as ex:
            futs = [ex.submit(run_once, seed, duration, td) for seed in range(runs)]
            for f in concurrent.futures.as_completed(futs):
                results.append(f.result())
    n = len(WEAPON_DEFINES)
    total_dmg = [0] * n
    total_dur = 0.0
    for r in results:
        total_dur += r["duration"]
        for w in range(n):
            total_dmg[w] += int(r.get(f"wpn_dmg_{w}", 0))
    mean_dur = total_dur / len(results)
    dps = [total_dmg[w] / total_dur for w in range(n)]
    return dps, mean_dur


def median(values):
    s = sorted(values)
    n = len(s)
    return s[n // 2] if n % 2 else (s[n // 2 - 1] + s[n // 2]) / 2


def print_table(dps, damages):
    print(f"  {'WEAPON':<11} {'DMG':>4}   {'DPS':>7}  bar")
    max_dps = max(dps) if max(dps) > 0 else 1
    for (name, _), d, base in zip(WEAPON_DEFINES, dps, damages):
        bar_len = int(40 * d / max_dps)
        print(f"  {name:<11} {base:>4}   {d:7.2f}  {'█' * bar_len}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--runs", type=int, default=12,
                    help="Games per iteration (more = less noise)")
    ap.add_argument("--iters", type=int, default=3,
                    help="Tuning iterations")
    ap.add_argument("--duration", type=float, default=180.0,
                    help="Game duration in seconds")
    ap.add_argument("--workers", type=int, default=os.cpu_count() or 4)
    ap.add_argument("--dry-run", action="store_true",
                    help="Measure only; don't modify config.h")
    ap.add_argument("--max-damage", type=int, default=40,
                    help="Per-weapon damage cap (avoid runaway)")
    ap.add_argument("--target", choices=["median", "mean", "max"],
                    default="median",
                    help="DPS target across weapons")
    args = ap.parse_args()

    if not EXE.exists():
        print(f"Build first. Missing: {EXE}")
        return 1

    orig = read_damages()
    print(f"Initial damages: {dict(zip([n for n,_ in WEAPON_DEFINES], orig))}")
    print()

    current = orig[:]
    for it in range(args.iters):
        if it > 0:
            rebuild()
        print(f"=== Iteration {it} ({args.runs} runs × {args.duration:.0f}s) ===")
        dps, mean_dur = measure(args.runs, args.duration, args.workers)
        print_table(dps, current)
        if args.target == "median":
            tgt = median(dps)
        elif args.target == "mean":
            tgt = sum(dps) / len(dps)
        else:
            tgt = max(dps)
        print(f"  target DPS = {tgt:.2f}  (mean run duration {mean_dur:.1f}s)")

        # Scale damage proportionally to bring DPS toward target.
        new = []
        for w, (name, _) in enumerate(WEAPON_DEFINES):
            if dps[w] < 0.01:
                # Weapon never landed a hit — bump damage modestly
                new_d = current[w] + 1
            else:
                scale = tgt / dps[w]
                # Damp to avoid wild swings
                scale = max(0.5, min(2.0, scale))
                new_d = max(1, round(current[w] * scale))
            new_d = min(new_d, args.max_damage)
            new.append(new_d)
        print(f"  -> new damages: {dict(zip([n for n,_ in WEAPON_DEFINES], new))}")
        print()

        if args.dry_run:
            print("--dry-run: not writing config.h")
            return 0

        current = new
        write_damages(current)

    # Final rebuild + measure
    rebuild()
    print(f"=== Final ({args.runs} runs × {args.duration:.0f}s) ===")
    dps, _ = measure(args.runs, args.duration, args.workers)
    print_table(dps, current)
    print(f"\nFinal: {dict(zip([n for n,_ in WEAPON_DEFINES], current))}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
