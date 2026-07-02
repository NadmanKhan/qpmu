#!/usr/bin/env bash
#
# start-pru.sh - runtime PRU init after reboot.
#
# Run from the repo root:  sudo ./core/daq/am335x.mcp3208.pru-shram/start-pru.sh
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TOOLS_DIR="$SCRIPT_DIR/tools"
SHARED_DIR="$SCRIPT_DIR/shared"
BUILD_DIR="/tmp/qpmu-am335x-mcp3208-pru-shram-build"
FIRMWARE="pru0_mcp3208"

info() { printf '\033[1;34m>> %s\033[0m\n' "$*"; }
ok()   { printf '\033[1;32m   OK: %s\033[0m\n' "$*"; }
fail() { printf '\033[1;31m   FAIL: %s\033[0m\n' "$*"; exit 1; }

[[ $EUID -eq 0 ]] || fail "must run as root (sudo)"
[[ -f "/lib/firmware/$FIRMWARE" ]] || fail "missing /lib/firmware/$FIRMWARE; run install.sh first"

RPROC=""
for rp in /sys/class/remoteproc/remoteproc*/; do
    name="$(cat "${rp}name" 2>/dev/null || true)"
    if [[ "$name" == *"4a334000"* ]]; then
        RPROC="$rp"
        break
    fi
done
if [[ -z "$RPROC" ]]; then
    modprobe pru_rproc 2>/dev/null || true
    for rp in /sys/class/remoteproc/remoteproc*/; do
        name="$(cat "${rp}name" 2>/dev/null || true)"
        if [[ "$name" == *"4a334000"* ]]; then
            RPROC="$rp"
            break
        fi
    done
fi
[[ -n "$RPROC" ]] || fail "cannot find PRU0 remoteproc (4a334000.pru)"

mkdir -p "$BUILD_DIR"
gcc -O2 -I"$SHARED_DIR" -o "$BUILD_DIR/test_shram" "$TOOLS_DIR/test_shram.c"

info "Starting PRU0 firmware"
echo stop > "${RPROC}state" 2>/dev/null || true
"$BUILD_DIR/test_shram" --clear --frames 0
echo "$FIRMWARE" > "${RPROC}firmware"
echo start > "${RPROC}state"
sleep 1

state="$(cat "${RPROC}state")"
[[ "$state" == "running" ]] || fail "PRU0 state: $state (expected running)"
"$BUILD_DIR/test_shram" --frames 3 --timeout-ms 3000
ok "PRU0 shared RAM stream verified"
