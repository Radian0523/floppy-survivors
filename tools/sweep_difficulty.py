"""Sweep difficulty values and report bot survival stats.

Usage:
    python tools/sweep_difficulty.py --runs 20
"""
import argparse
import ctypes
import os
import subprocess
import sys
from pathlib import Path

# Build helpers from runner.py
sys.path.insert(0, str(Path(__file__).parent))
from runner import run_batch, summarize

# Difficulty curve (mirrors params_from_difficulty in C).
def pivot(d: float, lo: float, hi: float) -> float:
    if d <= 50:
        return lo + (1.0 - lo) * (d / 50.0)
    return 1.0 + (hi - 1.0) * ((d - 50.0) / 50.0)


def params_for_difficulty(d: float) -> str:
    return ",".join([
        f"enemy_hp_mult={pivot(d, 0.55, 1.70):.4f}",
        f"enemy_spawn_min_mult={pivot(d, 1.80, 0.65):.4f}",
        f"enemy_speed_bonus_mult={pivot(d, 0.50, 1.50):.4f}",
        f"enemy_damage_mult={pivot(d, 0.70, 1.40):.4f}",
        f"spawn_count_mult={pivot(d, 0.60, 1.45):.4f}",
        f"player_speed_mult={pivot(d, 1.25, 0.88):.4f}",
        f"player_hp_mult={pivot(d, 1.80, 0.80):.4f}",
        f"player_invincible_mult={pivot(d, 1.40, 0.82):.4f}",
    ])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", default="./disk_survivor")
    ap.add_argument("--runs", type=int, default=15)
    ap.add_argument("--workers", type=int, default=os.cpu_count() or 4)
    ap.add_argument("--difficulties", default="0,20,35,50,65,75,85,100")
    args = ap.parse_args()

    exe = Path(args.exe).resolve()
    diffs = [float(d) for d in args.difficulties.split(",")]

    print(f"{'DIFF':>5}  {'median':>7}  {'mean':>7}  {'p25':>5}  {'p75':>5}  "
          f"{'kills':>5}  {'lv':>4}  {'win%':>5}  {'boss%':>5}")
    print("-" * 70)
    for d in diffs:
        params = params_for_difficulty(d)
        results = run_batch(exe, params, args.runs, None, args.workers)
        s = summarize(results)
        label = f"{int(d):3d}"
        # Preset markers
        markers = {20: " E", 50: " N", 75: " H", 90: " B"}
        label += markers.get(int(d), "  ")
        print(f"{label:>5}  {s['duration_median']:7.1f}  {s['duration_mean']:7.1f}  "
              f"{s['duration_p25']:5.0f}  {s['duration_p75']:5.0f}  "
              f"{int(s['kills_mean']):5d}  {s['level_mean']:4.1f}  "
              f"{s['win_rate']*100:4.0f}%  {s['boss_kill_rate']*100:4.0f}%")


if __name__ == "__main__":
    main()
