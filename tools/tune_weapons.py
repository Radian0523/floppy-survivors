"""Pull outlier weapons toward the pack — preserve diversity, just close the gap.

Goal is NOT "all weapons identical DPS". It's "no weapon is 5x weaker/stronger
than the median". Weapons inside a tolerance band around the target are left
untouched; only outliers get nudged.

Method:
  1. Run N bot games with --all-weapons.
  2. Sum per-weapon damage_dealt over runs.
  3. Target = median (or mean/max) of weapon DPS across the 9 weapons.
  4. For each weapon:
       - if DPS in [target/band, target*band] → leave alone
       - else scale damage toward target (capped by --max-step per iter)
  5. Write back to src/config.h, rebuild, repeat.

The bot's playstyle is held constant, so this measures "weapon effectiveness
when the player moves like a typical human" — good for relative balance.

Usage:
    python tools/tune_weapons.py                          # default 12 runs, 3 iters
    python tools/tune_weapons.py --band 1.5               # ±50% tolerance (looser)
    python tools/tune_weapons.py --band 1.2 --iters 5     # tight; more passes
    python tools/tune_weapons.py --dry-run                # measure only
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

# Per-weapon "secondary knob" used when damage alone hits its cap. Tuned in
# the same direction as the gap (WEAK -> buff, STR -> nerf).
#
# (weapon, define, type, step_pct_to_strengthen, min_factor, max_factor)
#   step_pct_to_strengthen: per-iter delta applied to STRENGTHEN the weapon.
#     +0.15 means "multiply by 1.15 when WEAK, multiply by 0.85 when STR".
#     Negative means "decreasing the value strengthens" (interval-like knobs).
#   min_factor / max_factor: bounds vs the ORIGINAL value, so we don't drift
#     to absurd values (e.g. range=2000 or interval=0.001).
WEAPON_SECONDARY = [
    ("PULSE",     "WEAPON_FIRE_INTERVAL",    "float", -0.10, 0.40, 2.50),
    ("ORBITERS",  "ORBITER_ORBIT_RADIUS",    "float",  0.15, 0.50, 3.00),
    ("BEAM",      "BEAM_LENGTH",             "float",  0.15, 0.50, 2.50),
    ("NOVA",      "NOVA_RADIUS_BASE",        "float",  0.15, 0.50, 2.50),
    ("MINES",     "MINE_EXPLOSION_RADIUS",   "float",  0.20, 0.50, 3.00),
    ("CHAIN",     "CHAIN_RANGE",             "float",  0.15, 0.50, 2.50),
    ("BOOMERANG", "BOOMERANG_RANGE",         "float",  0.15, 0.50, 2.50),
    ("TRAIL",     "TRAIL_INTERVAL",          "float", -0.15, 0.30, 3.00),
    ("WHIP",      "WHIP_RANGE",              "float",  0.20, 0.50, 3.00),
]

# Captured at script start so factor bounds are computed against the source-of-
# truth original values, not the current (possibly already-tuned) ones.
_orig_secondary = None


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


def read_secondary():
    """Return list of floats matching WEAPON_SECONDARY order."""
    text = CONFIG_H.read_text()
    out = []
    for name, define, kind, *_ in WEAPON_SECONDARY:
        # float values may have an 'f' suffix; integers are also accepted
        m = re.search(rf"#define\s+{define}\s+([-\d.]+)f?\b", text)
        if not m:
            print(f"ERROR: {define} not found in config.h")
            sys.exit(1)
        out.append(float(m.group(1)))
    return out


def write_secondary(new):
    text = CONFIG_H.read_text()
    for (name, define, kind, *_), val in zip(WEAPON_SECONDARY, new):
        suffix = "f" if kind == "float" else ""
        formatted = f"{val:.3f}{suffix}" if kind == "float" else f"{int(val)}{suffix}"
        text = re.sub(
            rf"(#define\s+{define}\s+)[-\d.]+f?\b",
            rf"\g<1>{formatted}",
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


def print_table(dps, damages, target=None, band=None):
    print(f"  {'WEAPON':<11} {'DMG':>4}   {'DPS':>7}  status  bar")
    max_dps = max(dps) if max(dps) > 0 else 1
    lo, hi = (target / band, target * band) if (target and band) else (None, None)
    for (name, _), d, base in zip(WEAPON_DEFINES, dps, damages):
        bar_len = int(40 * d / max_dps)
        if lo is None:
            status = "    "
        elif d < lo:
            status = "WEAK"
        elif d > hi:
            status = "STR "
        else:
            status = "ok  "
        print(f"  {name:<11} {base:>4}   {d:7.2f}  {status}    {'█' * bar_len}")


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
    ap.add_argument("--max-damage", type=int, default=10,
                    help="Soft cap on damage. Above this we switch to "
                         "structural tuning (range/interval). Default 10.")
    ap.add_argument("--target", choices=["median", "mean", "max"],
                    default="median",
                    help="DPS target across weapons")
    ap.add_argument("--band", type=float, default=1.5,
                    help="Tolerance band: DPS in [target/band, target*band] is "
                         "considered fine and left alone (default 1.5 = within 50pct)")
    ap.add_argument("--max-step", type=float, default=1.8,
                    help="Largest per-iter damage scaling (default 1.8x)")
    args = ap.parse_args()

    if not EXE.exists():
        print(f"Build first. Missing: {EXE}")
        return 1

    global _orig_secondary
    orig = read_damages()
    _orig_secondary = read_secondary()
    print(f"Initial damages: {dict(zip([n for n,_ in WEAPON_DEFINES], orig))}")
    sec_print = {n: f"{v:g}" for (n, *_), v in zip(WEAPON_SECONDARY, _orig_secondary)}
    print(f"Initial secondary: {sec_print}")
    print()

    current = orig[:]
    current_sec = _orig_secondary[:]
    for it in range(args.iters):
        if it > 0:
            rebuild()
        print(f"=== Iteration {it} ({args.runs} runs × {args.duration:.0f}s) ===")
        dps, mean_dur = measure(args.runs, args.duration, args.workers)
        if args.target == "median":
            tgt = median(dps)
        elif args.target == "mean":
            tgt = sum(dps) / len(dps)
        else:
            tgt = max(dps)
        print_table(dps, current, target=tgt, band=args.band)
        lo, hi = tgt / args.band, tgt * args.band
        print(f"  target DPS = {tgt:.2f}  tolerance band = [{lo:.2f}, {hi:.2f}]  "
              f"(mean run duration {mean_dur:.1f}s)")

        # Only touch outliers; preserve weapons inside the band.
        # PULSE is the universal starter weapon — every player relies on it
        # even before any upgrade unlocks. Treat it as the FIXED baseline and
        # tune the other 8 weapons relative to it.
        new = []
        new_sec = current_sec[:]
        changes = []
        min_step = 1.0 / args.max_step
        for w, (name, _) in enumerate(WEAPON_DEFINES):
            if name == "PULSE":
                new.append(current[w])
                continue
            old_d = current[w]
            sec_info = WEAPON_SECONDARY[w]
            sec_name = sec_info[1]
            sec_step = sec_info[3]      # signed: positive = increasing buffs
            sec_min_factor = sec_info[4]
            sec_max_factor = sec_info[5]
            orig_sec_val = _orig_secondary[w]
            sec_lo = orig_sec_val * sec_min_factor
            sec_hi = orig_sec_val * sec_max_factor

            in_band = (dps[w] >= lo and dps[w] <= hi) if dps[w] > 0.01 else False
            is_weak = (not in_band) and dps[w] < lo
            is_str  = (not in_band) and dps[w] > hi

            if in_band:
                new.append(old_d)
                continue

            # Decide if we should touch damage or secondary this iter.
            # Prefer damage if it has room. Otherwise touch secondary.
            damage_has_room_up = (old_d < args.max_damage)
            damage_has_room_dn = (old_d > 1)

            new_d = old_d
            use_secondary = False
            if is_weak:
                if damage_has_room_up:
                    scale = tgt / max(dps[w], 0.01)
                    scale = max(min_step, min(args.max_step, scale))
                    proposed = max(1, round(old_d * scale))
                    proposed = min(proposed, args.max_damage)
                    if proposed != old_d:
                        new_d = proposed
                    else:
                        use_secondary = True
                else:
                    use_secondary = True
            elif is_str:
                if damage_has_room_dn:
                    scale = tgt / dps[w]
                    scale = max(min_step, min(args.max_step, scale))
                    proposed = max(1, round(old_d * scale))
                    if proposed != old_d:
                        new_d = proposed
                    else:
                        use_secondary = True
                else:
                    use_secondary = True

            new.append(new_d)

            if use_secondary:
                # WEAK -> apply +sec_step (in strengthen direction)
                # STR  -> apply -sec_step (opposite direction)
                step = sec_step if is_weak else -sec_step
                factor = 1.0 + step
                proposed_sec = current_sec[w] * factor
                proposed_sec = max(sec_lo, min(sec_hi, proposed_sec))
                if abs(proposed_sec - current_sec[w]) > 1e-4:
                    pct = (dps[w] / tgt - 1) * 100
                    changes.append(
                        f"{name}.{sec_name} "
                        f"{current_sec[w]:.2f}->{proposed_sec:.2f} "
                        f"({pct:+.0f}% off, dmg at cap)"
                    )
                    new_sec[w] = proposed_sec
                # else: at structural cap too; nothing we can do
            elif new_d != old_d:
                pct = (dps[w] / tgt - 1) * 100
                changes.append(
                    f"{name}.dmg {old_d}->{new_d} ({pct:+.0f}% off)"
                )

        if not changes:
            print("  all weapons within band — converged.")
            print()
            break

        print(f"  adjusting:")
        for c in changes:
            print(f"    {c}")
        print()

        if args.dry_run:
            print("--dry-run: not writing config.h")
            return 0

        current = new
        current_sec = new_sec
        write_damages(current)
        write_secondary(current_sec)

    # Final rebuild + measure
    rebuild()
    print(f"=== Final ({args.runs} runs × {args.duration:.0f}s) ===")
    dps, _ = measure(args.runs, args.duration, args.workers)
    if args.target == "median":
        tgt = median(dps)
    elif args.target == "mean":
        tgt = sum(dps) / len(dps)
    else:
        tgt = max(dps)
    print_table(dps, current, target=tgt, band=args.band)
    print(f"\nFinal damages: {dict(zip([n for n,_ in WEAPON_DEFINES], current))}")
    sec_final = read_secondary()
    print(f"Final secondary:")
    for (name, define, *_), orig_v, new_v in zip(WEAPON_SECONDARY,
                                                  _orig_secondary, sec_final):
        if abs(orig_v - new_v) > 1e-4:
            ratio = new_v / orig_v
            print(f"  {define}: {orig_v:g} -> {new_v:.3g}  ({ratio:.2f}x)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
