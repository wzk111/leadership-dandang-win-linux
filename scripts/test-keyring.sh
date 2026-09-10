#!/usr/bin/env bash
set -euo pipefail
# Ephemeral CI-only keyring. Never target a real user's existing keyring.
test_home="$(mktemp -d)"
trap 'rm -rf -- "$test_home"' EXIT
export HOME="$test_home"
export XDG_DATA_HOME="$test_home/.local/share"
export XDG_CONFIG_HOME="$test_home/.config"
mkdir -p "$XDG_DATA_HOME" "$XDG_CONFIG_HOME"
eval "$(printf 'ci-test-password' | gnome-keyring-daemon --unlock --components=secrets)"
./build/test_secret
