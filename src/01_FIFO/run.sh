#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
ROOT_DIR=$(cd -- "$SCRIPT_DIR/../.." && pwd)

SERVER=${SERVER:-192.168.101.10}
PORT=${PORT:-5003}
SEEDS=${SEEDS:-"1 2 3 4 5"}
REPS=${REPS:-3}
TOTAL=${TOTAL:-500}
START=${START:-0}
DIFFICULTY=${DIFFICULTY:-30000000}
REP_PROB_PERCENT=${REP_PROB_PERCENT:-20}
DELAY_US=${DELAY_US:-600000}
PRIO_LAMBDA=${PRIO_LAMBDA:-1.5}

SERVER_BIN=${SERVER_BIN:-"$ROOT_DIR/server"}
BIN_PATH=$("$ROOT_DIR/get-bin-path.sh")
CLIENT_BIN=${CLIENT_BIN:-"$ROOT_DIR/$BIN_PATH/client"}
RAW_DIR="$SCRIPT_DIR/raw"

if [[ ! "$REPS" =~ ^[1-9][0-9]*$ ]]; then
    printf 'REPS must be a positive integer (got %s)\n' "$REPS" >&2
    exit 2
fi

if [[ ! -x "$CLIENT_BIN" ]]; then
    printf 'Client binary is missing or not executable: %s\n' "$CLIENT_BIN" >&2
    exit 1
fi

mkdir -p "$RAW_DIR"

server_pid=

stop_server() {
    if [[ -n "$server_pid" ]]; then
        kill "$server_pid" 2>/dev/null || true
        wait "$server_pid" 2>/dev/null || true
        server_pid=
    fi
}

cleanup() {
    stop_server
}
trap cleanup EXIT

start_server() {
    local log_file=$1
    mkdir -p "$RAW_DIR"

    if ( : >/dev/tcp/127.0.0.1/"$PORT" ) 2>/dev/null; then
        printf 'Port %s is already in use; stop the existing server first\n' "$PORT" >&2
        return 1
    fi

    "$SERVER_BIN" "$PORT" >"$log_file" 2>&1 &
    server_pid=$!

    for ((attempt = 0; attempt < 100; attempt++)); do
        if ! kill -0 "$server_pid" 2>/dev/null; then
            printf 'Server exited before becoming ready; see %s\n' "$log_file" >&2
            return 1
        fi
        if ( : >/dev/tcp/127.0.0.1/"$PORT" ) 2>/dev/null; then
            return 0
        fi
        sleep 0.1
    done

    printf 'Server did not listen on port %s; see %s\n' "$PORT" "$log_file" >&2
    return 1
}

printf 'Building FIFO server...\n'
make -C "$ROOT_DIR" EXP=01_FIFO clean all

read -r -a seeds <<< "$SEEDS"
if (( ${#seeds[@]} == 0 )); then
    printf 'SEEDS must contain at least one seed\n' >&2
    exit 2
fi

for seed in "${seeds[@]}"; do
    if [[ ! "$seed" =~ ^[0-9]+$ ]]; then
        printf 'Invalid seed: %s\n' "$seed" >&2
        exit 2
    fi

    for ((rep = 1; rep <= REPS; rep++)); do
        name="seed${seed}_rep${rep}"
        client_log="$RAW_DIR/${name}.log"
        server_log="$RAW_DIR/${name}_server.log"

        printf 'Seed %s, run %s/%s: starting server\n' "$seed" "$rep" "$REPS"
        start_server "$server_log"
        "$CLIENT_BIN" "$SERVER" "$PORT" "$seed" "$TOTAL" "$START" \
            "$DIFFICULTY" "$REP_PROB_PERCENT" "$DELAY_US" "$PRIO_LAMBDA" \
            >"$client_log" 2>&1
        stop_server
        printf '  client output: %s\n' "$client_log"
    done
done

printf 'Completed %s runs across %s seeds. Logs are in %s\n' \
    "$((REPS * ${#seeds[@]}))" "${#seeds[@]}" "$RAW_DIR"
