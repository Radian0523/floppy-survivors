"""Run the full tuning suite in sequence.

Steps (each can be skipped via flags):
  1. Build (`make mac`)
  2. Baseline sweep — show DIFFICULTY curve as-is
  3. XP-curve tune — adjust XP_BASE_REQUIREMENT / XP_PER_LEVEL so the bot
     levels up roughly every TARGET_INTERVAL seconds
  4. Post-XP sweep — verify difficulty curve is still sane after XP change
  5. (optional) Bayesian difficulty tune at a given target survival

Usage:
    python tools/tune_all.py                    # full default pass
    python tools/tune_all.py --skip-bayes       # quick pass (no slow Bayes step)
    python tools/tune_all.py --quick            # 8 runs everywhere (fast)
    python tools/tune_all.py --target-interval 8
"""
import argparse
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).parent


def section(title: str):
    print()
    print("=" * 64)
    print(f"  {title}")
    print("=" * 64)


def sh(cmd, **kw):
    print(f"$ {' '.join(cmd)}")
    return subprocess.run(cmd, **kw)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--runs", type=int, default=15)
    ap.add_argument("--quick", action="store_true",
                    help="Use small run counts everywhere (fast smoke test)")
    ap.add_argument("--target-interval", type=float, default=5.5,
                    help="Target seconds between level-ups (default 5.5)")
    ap.add_argument("--bayes-target", type=float, default=180.0,
                    help="Bayes difficulty tune target (median survival sec)")
    ap.add_argument("--bayes-calls", type=int, default=30)
    ap.add_argument("--bayes-runs", type=int, default=10)
    ap.add_argument("--skip-build", action="store_true")
    ap.add_argument("--skip-sweep", action="store_true")
    ap.add_argument("--skip-xp", action="store_true")
    ap.add_argument("--skip-bayes", action="store_true",
                    help="Skip the slow Bayesian difficulty step")
    args = ap.parse_args()

    if args.quick:
        args.runs = 8
        args.bayes_calls = 12
        args.bayes_runs = 6

    t0 = time.time()

    if not args.skip_build:
        section("[1/5] Build")
        if sh(["make", "mac"]).returncode != 0:
            print("Build failed."); return 1

    if not args.skip_sweep:
        section("[2/5] Baseline difficulty sweep")
        sh(["python3", str(HERE / "sweep_difficulty.py"),
            "--runs", str(args.runs)])

    if not args.skip_xp:
        section(f"[3/5] XP curve tune (target ~{args.target_interval}s/level-up)")
        sh(["python3", str(HERE / "tune_xp.py"),
            "--target-interval", str(args.target_interval),
            "--runs", str(args.runs)])

    if not args.skip_sweep and not args.skip_xp:
        section("[4/5] Post-XP difficulty sweep")
        sh(["python3", str(HERE / "sweep_difficulty.py"),
            "--runs", str(args.runs)])

    if not args.skip_bayes:
        section(f"[5/5] Bayesian difficulty tune (target {args.bayes_target}s)")
        out_path = HERE.parent / "tuned_difficulty.json"
        sh(["python3", str(HERE / "tune.py"),
            "--target", str(args.bayes_target),
            "--runs", str(args.bayes_runs),
            "--calls", str(args.bayes_calls),
            "--out", str(out_path)])
        print(f"\nBayes result: {out_path}")

    section(f"Done in {time.time()-t0:.1f}s")
    print("Next: make mac && ./disk_survivor")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
