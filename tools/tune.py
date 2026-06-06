"""Bayesian-optimize game parameters so the bot's median survival time
matches a target.

Usage:
    python tools/tune.py --target 180 --runs 12 --calls 30 --out calib.json

For each candidate parameter set, runs the headless bot N times and computes
the median survival time. Minimizes |median - target|.
"""
import argparse
import json
import os
from pathlib import Path

import numpy as np
from skopt import gp_minimize
from skopt.space import Real
from skopt.utils import use_named_args

from runner import run_batch, summarize


# Search space (multipliers). Tighter bounds = faster convergence.
SPACE = [
    Real(0.5,  3.0, name="enemy_hp_mult"),
    Real(0.3,  2.0, name="enemy_spawn_min_mult"),
    Real(0.3,  3.0, name="enemy_speed_bonus_mult"),
    Real(0.5,  2.0, name="enemy_damage_mult"),
    Real(0.5,  3.0, name="spawn_count_mult"),
    Real(0.7,  1.6, name="player_speed_mult"),
    Real(0.6,  2.0, name="player_hp_mult"),
    Real(0.5,  2.0, name="player_invincible_mult"),
]


def build_params(values: dict) -> str:
    return ",".join(f"{k}={v:.4f}" for k, v in values.items())


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", default="./disk_survivor")
    ap.add_argument("--target", type=float, required=True,
                    help="target median survival time in seconds")
    ap.add_argument("--runs", type=int, default=10,
                    help="runs per evaluation (more=stable, less=fast)")
    ap.add_argument("--calls", type=int, default=30,
                    help="total optimization steps")
    ap.add_argument("--workers", type=int, default=os.cpu_count() or 4)
    ap.add_argument("--out", default=None, help="write optimal params here")
    args = ap.parse_args()

    exe = Path(args.exe).resolve()
    if not exe.exists():
        print(f"ERROR: executable not found: {exe}")
        return 1

    iter_count = {"n": 0}

    @use_named_args(SPACE)
    def objective(**values):
        iter_count["n"] += 1
        params = build_params(values)
        results = run_batch(exe, params, args.runs, None, args.workers)
        s = summarize(results)
        loss = abs(s["duration_median"] - args.target)
        print(f"[{iter_count['n']:3d}/{args.calls}] "
              f"median={s['duration_median']:6.1f}  "
              f"mean={s['duration_mean']:6.1f}  "
              f"win={s['win_rate']*100:4.0f}%  "
              f"loss={loss:6.2f}  "
              f"params={params}")
        return loss

    print(f"Optimizing to target median = {args.target}s "
          f"({args.runs} runs/step, {args.calls} steps)")
    res = gp_minimize(objective, SPACE, n_calls=args.calls,
                      n_initial_points=8, random_state=0)

    best_values = {dim.name: float(v) for dim, v in zip(SPACE, res.x)}
    print("\n=== BEST ===")
    print(f"loss     = {res.fun:.3f}")
    print(f"target   = {args.target}")
    for k, v in best_values.items():
        print(f"  {k:30s} = {v:.4f}")

    # Verify with bigger sample
    print("\nVerifying best params with 30 runs...")
    params_str = build_params(best_values)
    results = run_batch(exe, params_str, max(30, args.runs * 2), None,
                        args.workers)
    s = summarize(results)
    print(f"  median={s['duration_median']:.1f}  mean={s['duration_mean']:.1f} "
          f" win_rate={s['win_rate']*100:.1f}%")

    if args.out:
        with open(args.out, "w") as f:
            json.dump({
                "target": args.target,
                "params": best_values,
                "verification": s,
            }, f, indent=2)
        print(f"\nSaved: {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
