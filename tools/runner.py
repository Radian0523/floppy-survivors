"""Run the headless game in parallel and aggregate stats.

Usage:
    python tools/runner.py --runs 30 --params enemy_hp_mult=1.5
    python tools/runner.py --runs 30  # default params
"""
import argparse
import concurrent.futures
import json
import os
import shutil
import subprocess
import tempfile
from pathlib import Path
from typing import Dict, List, Optional


def parse_stats(text: str) -> Dict[str, float]:
    out = {}
    for line in text.splitlines():
        if "=" not in line:
            continue
        k, v = line.split("=", 1)
        try:
            out[k] = float(v)
        except ValueError:
            out[k] = v
    return out


def run_once(exe: Path, params: str, seed: int, duration: Optional[float],
             tmpdir: Path) -> Dict[str, float]:
    out_path = tmpdir / f"run_{seed}.txt"
    cmd = [str(exe), "--headless", "--bot", f"--seed={seed}",
           f"--output={out_path}"]
    if params:
        cmd.append(f"--params={params}")
    if duration is not None:
        cmd.append(f"--duration={duration}")
    subprocess.run(cmd, check=True, capture_output=True)
    return parse_stats(out_path.read_text())


def run_batch(exe: Path, params: str, runs: int,
              duration: Optional[float], workers: int) -> List[Dict[str, float]]:
    results: List[Dict[str, float]] = []
    with tempfile.TemporaryDirectory() as td:
        tmpdir = Path(td)
        with concurrent.futures.ThreadPoolExecutor(max_workers=workers) as ex:
            futs = [ex.submit(run_once, exe, params, seed, duration, tmpdir)
                    for seed in range(runs)]
            for f in concurrent.futures.as_completed(futs):
                results.append(f.result())
    return results


def summarize(results: List[Dict[str, float]]) -> Dict[str, float]:
    n = len(results)
    durs = sorted(r["duration"] for r in results)
    kills = [r["kills"] for r in results]
    levels = [r["level"] for r in results]
    survived = [r["survived"] for r in results]
    boss_defeated = [r["boss_defeated"] for r in results]
    return {
        "runs": n,
        "duration_mean": sum(durs) / n,
        "duration_median": durs[n // 2],
        "duration_min": durs[0],
        "duration_max": durs[-1],
        "duration_p25": durs[max(0, n // 4)],
        "duration_p75": durs[min(n - 1, 3 * n // 4)],
        "kills_mean": sum(kills) / n,
        "level_mean": sum(levels) / n,
        "win_rate": sum(survived) / n,
        "boss_kill_rate": sum(boss_defeated) / n,
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", default="./disk_survivor")
    ap.add_argument("--runs", type=int, default=20)
    ap.add_argument("--params", default="")
    ap.add_argument("--duration", type=float, default=None)
    ap.add_argument("--workers", type=int, default=os.cpu_count() or 4)
    ap.add_argument("--json", action="store_true", help="output JSON only")
    args = ap.parse_args()

    exe = Path(args.exe).resolve()
    if not exe.exists():
        print(f"ERROR: executable not found: {exe}")
        return 1

    results = run_batch(exe, args.params, args.runs, args.duration,
                        args.workers)
    summary = summarize(results)
    summary["params"] = args.params

    if args.json:
        print(json.dumps(summary, indent=2))
    else:
        print(f"runs={summary['runs']}  params={args.params or '(default)'}")
        print(f"  duration  mean={summary['duration_mean']:6.1f}  "
              f"median={summary['duration_median']:6.1f}  "
              f"p25={summary['duration_p25']:6.1f}  "
              f"p75={summary['duration_p75']:6.1f}  "
              f"min={summary['duration_min']:6.1f}  "
              f"max={summary['duration_max']:6.1f}")
        print(f"  kills_mean={summary['kills_mean']:6.1f}  "
              f"level_mean={summary['level_mean']:5.2f}  "
              f"win_rate={summary['win_rate']*100:5.1f}%  "
              f"boss_kill={summary['boss_kill_rate']*100:5.1f}%")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
