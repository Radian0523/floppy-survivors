"""Tune the XP curve so bots get a level-up roughly every TARGET_INTERVAL seconds.

Direct iterative scaling: measure level_mean, scale config XP requirements,
rebuild, re-measure. Converges in 2-3 iterations.

Usage:
    python tools/tune_xp.py --target-interval 7  # one level-up per ~7s
    python tools/tune_xp.py --target-levels 45   # equivalent: 45 levels per 5min
"""
import argparse
import os
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from runner import run_batch, summarize

CONFIG_H = Path(__file__).parent.parent / "src" / "config.h"


def read_xp_values():
    text = CONFIG_H.read_text()
    base = int(re.search(r"#define\s+XP_BASE_REQUIREMENT\s+(\d+)", text).group(1))
    per = int(re.search(r"#define\s+XP_PER_LEVEL\s+(\d+)", text).group(1))
    return base, per


def write_xp_values(base: int, per: int):
    text = CONFIG_H.read_text()
    text = re.sub(r"(#define\s+XP_BASE_REQUIREMENT\s+)\d+",
                  rf"\g<1>{base}", text)
    text = re.sub(r"(#define\s+XP_PER_LEVEL\s+)\d+",
                  rf"\g<1>{per}", text)
    CONFIG_H.write_text(text)


def rebuild():
    r = subprocess.run(["make", "mac"], capture_output=True, text=True)
    if r.returncode != 0:
        print("BUILD FAILED:", r.stderr[-500:])
        sys.exit(1)


def measure(exe: Path, runs: int, workers: int):
    """Return (summary, target_level, note).

    target_level = median level of bots who survived 5 min (preferred),
    fallback to median level of all when nobody made it.
    """
    results = run_batch(exe, "", runs, None, workers)
    s = summarize(results)
    survivors = sorted([r["level"] for r in results if r["duration"] >= 290.0])
    if survivors:
        rate = survivors[len(survivors) // 2]
        note = f"{len(survivors)}/{len(results)} survived"
    else:
        all_levels = sorted([r["level"] for r in results])
        rate = all_levels[len(all_levels) // 2]
        note = f"0/{len(results)} survived (curve may be too hard)"
    return s, rate, note


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", default="./disk_survivor")
    ap.add_argument("--runs", type=int, default=12)
    ap.add_argument("--workers", type=int, default=os.cpu_count() or 4)
    ap.add_argument("--target-interval", type=float, default=None,
                    help="Target seconds per level-up (e.g. 7.0)")
    ap.add_argument("--target-levels", type=float, default=None,
                    help="Target levels per 5-min game (overrides --target-interval)")
    ap.add_argument("--max-iters", type=int, default=4)
    ap.add_argument("--tolerance", type=float, default=0.10,
                    help="Acceptable relative error (0.10 = within 10 percent)")
    ap.add_argument("--no-write", action="store_true",
                    help="Print recommended values without modifying config.h")
    args = ap.parse_args()

    if args.target_levels is not None:
        target = args.target_levels
    elif args.target_interval is not None:
        target = 300.0 / args.target_interval
    else:
        target = 45.0  # default: ~6.7s per level-up

    exe = Path(args.exe).resolve()
    if not exe.exists():
        print(f"Build first. Missing: {exe}")
        return 1

    orig_base, orig_per = read_xp_values()
    print(f"Target: ~{target:.0f} levels per 5-min game "
          f"({300.0/target:.1f}s per level-up)")
    print(f"Starting from XP_BASE_REQUIREMENT={orig_base}, XP_PER_LEVEL={orig_per}")
    print()

    best_base, best_per = orig_base, orig_per
    best_err = float("inf")

    saved_base, saved_per = best_base, best_per
    for it in range(args.max_iters):
        rebuild()
        s, median_level, note = measure(exe, args.runs, args.workers)
        err = abs(median_level - target) / target
        print(f"iter {it}: base={best_base} per={best_per}  "
              f"median_survivor_level={median_level:5.1f}  "
              f"raw level_mean={s['level_mean']:5.1f}  "
              f"win%={s['win_rate']*100:3.0f}  "
              f"err={err*100:4.1f}%  [{note}]")

        if err < best_err:
            best_err = err
            saved_base, saved_per = best_base, best_per

        if err <= args.tolerance:
            print(f"Converged within {args.tolerance*100:.0f}%.")
            break

        # Total XP collected is ~constant. Level reached scales inversely with
        # XP requirement, so to lift levels by factor X we divide config by X.
        scale = median_level / target
        new_base = max(1, round(best_base * scale))
        new_per = max(1, round(best_per * scale))
        # Avoid degenerate no-op
        if (new_base, new_per) == (best_base, best_per):
            new_base = max(1, best_base + (1 if scale > 1 else -1))
            new_per = max(1, best_per + (1 if scale > 1 else -1))
            if new_base == best_base and new_per == best_per:
                print("Cannot scale further with integer grid.")
                break
        best_base, best_per = new_base, new_per
        write_xp_values(best_base, best_per)

    if args.no_write:
        print(f"\n--no-write: restoring original ({orig_base}, {orig_per})")
        write_xp_values(orig_base, orig_per)
        rebuild()
    else:
        # Write the best one we saw
        write_xp_values(saved_base, saved_per)
        rebuild()
        print(f"\nFinal: XP_BASE_REQUIREMENT={saved_base}, "
              f"XP_PER_LEVEL={saved_per}  (err={best_err*100:.1f}%)")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
