#!/bin/sh

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PATHTOCOMMON=${OS_CHALLENGE_BIN_ROOT:-$SCRIPT_DIR}
PORT=${PORT:-5003}

BIN_PATH=$("$SCRIPT_DIR/get-bin-path.sh")
"$PATHTOCOMMON/$BIN_PATH/server" $PORT
