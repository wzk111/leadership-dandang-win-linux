#!/usr/bin/env bash
set -euo pipefail
openbox >/dev/null 2>&1 &
wm_pid=$!
trap 'kill "$wm_pid" 2>/dev/null || true' EXIT
./build/test_overlay_runtime
