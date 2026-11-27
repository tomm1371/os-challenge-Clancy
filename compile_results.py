#!/usr/bin/env python3
"""Compile client logs of one experiment into CSV files and print a summary.

Usage:
    ./compile_results.py 00_bruteforce
    ./compile_results.py 01_FIFO --baseline 00_bruteforce
"""
import argparse
import csv
import re
import statistics
import subprocess
import sys
from datetime import datetime
from pathlib import Path

LOG_RE = re.compile(r"seed(\d+)_rep(\d+)\.log$")
RESULT_RE = re.compile(r"Results:\s+([\d.]+)\s+([\d.]+)")
PRIO_RE = re.compile(r"Prio stats:\s*(.*)")

RUN_FIELDS = ["experiment", "seed", "rep", "reliability", "score_us", "score_s",
              "valid", "prio_stats", "log_file", "git_commit", "compiled_at"]


def git_commit(root: Path) -> str:
    """Short commit hash, or 'unknown' if git is unavailable."""
    try:
        out = subprocess.run(["git", "rev-parse", "--short", "HEAD"], cwd=root,
                             capture_output=True, text=True, check=True)
        return out.stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        return "unknown"


def parse_log(path: Path):
    """Return (reliability, score_us, prio_stats) from a log, or None if no result line.
    If a log holds several Results lines, the last one is used."""
    text = path.read_text(errors="replace")
    results = RESULT_RE.findall(text)
    if not results:
        return None
    reliability, score = results[-1]
    prio = PRIO_RE.findall(text)
    return float(reliability), float(score), (prio[-1].strip() if prio else "")


def collect_runs(exp: str, root: Path):
    exp_dir = root / "src" / exp
    raw_dir = exp_dir / "raw"
    if not raw_dir.is_dir():
        sys.exit(f"error: {raw_dir} not found")

    commit = git_commit(root)
    stamp = datetime.now().isoformat(timespec="seconds")
    rows, skipped = [], []

    logs = sorted(raw_dir.glob("seed*_rep*.log"),
                  key=lambda p: tuple(int(x) for x in LOG_RE.search(p.name).groups())
                  if LOG_RE.search(p.name) else (0, 0))
    for log in logs:
        m = LOG_RE.search(log.name)
        if not m:
            continue
        parsed = parse_log(log)
        if parsed is None:
            skipped.append(log.name)
            continue
        reliability, score, prio = parsed
        rows.append({
            "experiment": exp,
            "seed": int(m.group(1)),
            "rep": int(m.group(2)),
            "reliability": reliability,
            "score_us": int(score),
            "score_s": round(score / 1e6, 3),
            "valid": reliability >= 100.0,   # only fully reliable runs count for scores
            "prio_stats": prio,
            "log_file": str(log.relative_to(root)),
            "git_commit": commit,
            "compiled_at": stamp,
        })
    return rows, skipped


def seed_medians(rows):
    """{seed: [valid scores in us]} -> {seed: median}"""
    by_seed = {}
    for r in rows:
        if r["valid"]:
            by_seed.setdefault(r["seed"], []).append(r["score_us"])
    return by_seed


def load_baseline(exp: str, root: Path):
    path = root / "src" / exp / "results.csv"
    if not path.is_file():
        sys.exit(f"error: baseline {path} not found (run compile_results.py {exp} first)")
    by_seed = {}
    with path.open(newline="") as f:
        for r in csv.DictReader(f):
            if r["valid"] == "True":
                by_seed.setdefault(int(r["seed"]), []).append(float(r["score_us"]))
    return {s: statistics.median(v) for s, v in by_seed.items()}


def build_summary(exp, rows, baseline_medians):
    by_seed = seed_medians(rows)
    summary = []
    per_seed_med = {}
    for seed in sorted(by_seed):
        v = by_seed[seed]
        med = statistics.median(v)
        per_seed_med[seed] = med
        row = {"experiment": exp, "scope": f"seed{seed}", "n_runs": len(v),
               "median_us": med, "min_us": min(v), "max_us": max(v),
               "ratio_vs_baseline": ""}
        if baseline_medians and seed in baseline_medians:
            row["ratio_vs_baseline"] = round(baseline_medians[seed] / med, 3)
        summary.append(row)

    if per_seed_med:
        all_valid = [s for v in by_seed.values() for s in v]
        overall = {"experiment": exp, "scope": "overall(median of seed medians)",
                   "n_runs": len(all_valid),
                   "median_us": statistics.median(per_seed_med.values()),
                   "min_us": min(all_valid), "max_us": max(all_valid),
                   "ratio_vs_baseline": ""}
        if baseline_medians:
            ratios = [baseline_medians[s] / m for s, m in per_seed_med.items()
                      if s in baseline_medians]
            if ratios:
                overall["ratio_vs_baseline"] = round(statistics.median(ratios), 3)
        summary.append(overall)
    return summary, per_seed_med


def write_csv(path: Path, fields, rows):
    with path.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(rows)


def print_summary(exp, rows, summary, per_seed_med, baseline_name, baseline_medians):
    invalid = [r for r in rows if not r["valid"]]
    print(f"\n== {exp}: {len(rows)} runs, {len(invalid)} below 100% reliability ==")
    for r in invalid:
        print(f"  EXCLUDED seed{r['seed']}_rep{r['rep']}: reliability {r['reliability']}%")

    header = f"{'scope':<34}{'runs':>5}{'median (s)':>14}{'min (s)':>12}{'max (s)':>12}"
    if baseline_medians:
        header += f"{'x vs ' + baseline_name:>18}"
    print(header)
    for s in summary:
        line = (f"{s['scope']:<34}{s['n_runs']:>5}{s['median_us'] / 1e6:>14.3f}"
                f"{s['min_us'] / 1e6:>12.3f}{s['max_us'] / 1e6:>12.3f}")
        if baseline_medians:
            line += f"{s['ratio_vs_baseline']!s:>18}"
        print(line)

    if baseline_medians:
        common = [s for s in per_seed_med if s in baseline_medians]
        wins = sum(1 for s in common if per_seed_med[s] < baseline_medians[s])
        print(f"\nbetter than {baseline_name} on {wins} of {len(common)} common seeds")
        base_best = min(baseline_medians[s] for s in common) if common else None
        new_worst = max(per_seed_med[s] for s in common) if common else None
        if common and new_worst < base_best:
            print("seed medians do not overlap: every seed of this experiment beats "
                  "every seed of the baseline")
    print("\nscore = mean(latency x priority), lower is better; ratio > 1 means faster")


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("experiment", help="folder name under src/, e.g. 00_bruteforce")
    ap.add_argument("--baseline", help="experiment to compare against, e.g. 00_bruteforce")
    args = ap.parse_args()

    root = Path(__file__).resolve().parent
    exp = Path(args.experiment.rstrip("/")).name   # accepts "src/00_x/" too

    rows, skipped = collect_runs(exp, root)
    if not rows:
        sys.exit(f"error: no parsable logs found in src/{exp}/raw/")
    if skipped:
        print(f"warning: no 'Results:' line in: {', '.join(skipped)}")

    baseline_medians = load_baseline(args.baseline, root) if args.baseline else None
    summary, per_seed_med = build_summary(exp, rows, baseline_medians)

    exp_dir = root / "src" / exp
    write_csv(exp_dir / "results.csv", RUN_FIELDS, rows)
    write_csv(exp_dir / "results_summary.csv",
              ["experiment", "scope", "n_runs", "median_us", "min_us", "max_us",
               "ratio_vs_baseline"], summary)

    print_summary(exp, rows, summary, per_seed_med, args.baseline, baseline_medians)
    print(f"\nwrote {exp_dir / 'results.csv'}")
    print(f"wrote {exp_dir / 'results_summary.csv'}")


if __name__ == "__main__":
    main()