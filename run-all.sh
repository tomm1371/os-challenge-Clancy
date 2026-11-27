#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
LOG_ROOT=${LOG_ROOT:-"$SCRIPT_DIR/run-logs"}
RUN_ID="$(date '+%Y%m%d_%H%M%S')_$$"
RUN_DIR="$LOG_ROOT/$RUN_ID"

mkdir -p "$RUN_DIR"

mapfile -t run_scripts < <(find "$SCRIPT_DIR/src" -mindepth 2 -maxdepth 2 \
    -type f -name run.sh -print | sort)

if (( ${#run_scripts[@]} == 0 )); then
    printf 'No experiment run.sh scripts found under %s/src\n' "$SCRIPT_DIR" >&2
    exit 1
fi

failed=0
summary_file="$RUN_DIR/summary.txt"
: > "$summary_file"

printf 'Running %s experiments sequentially. Logs: %s\n' \
    "${#run_scripts[@]}" "$RUN_DIR"

for run_script in "${run_scripts[@]}"; do
    experiment=${run_script%/run.sh}
    experiment=${experiment##*/}
    log_file="$RUN_DIR/$experiment.log"

    printf '\n===== Starting %s =====\n' "$experiment"
    if bash "$run_script" 2>&1 | tee "$log_file"; then
        printf '%s: PASS\n' "$experiment" | tee -a "$summary_file"
    else
        status=$?
        printf '%s: FAIL (exit %s; see %s)\n' \
            "$experiment" "$status" "$log_file" | tee -a "$summary_file"
        failed=1
    fi
done

printf '\nExperiment summary:\n'
cat "$summary_file"
printf 'Full logs: %s\n' "$RUN_DIR"

exit "$failed"
