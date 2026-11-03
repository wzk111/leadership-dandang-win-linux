#!/usr/bin/env bash
# Limited Level C packaged-runtime smoke in a clean container, not a real
# GNOME X11/Wayland desktop acceptance gate.
set -Eeuo pipefail
trap 'echo "ERROR: package smoke host failed at line $LINENO: $BASH_COMMAND" >&2' ERR
package=$(realpath -- "${1:?Usage: test-linux-package.sh package.deb [evidence-dir]}")
evidence_dir=$(realpath -m -- "${2:-$(dirname -- "$package")}")
probe=$(realpath -- "${3:-build/test_package_state_probe}")
test -x "$probe"
mkdir -p -- "$evidence_dir"
docker run --rm -i --platform linux/amd64 \
  --mount "type=bind,source=$package,target=/package/worksidekick.deb,readonly" \
  --mount "type=bind,source=$probe,target=/package/test_package_state_probe,readonly" \
  ubuntu:22.04@sha256:076e7bf82319619f28dd0aa9c02ab34b055ad37100395b4f49a2ffba04fb5e11 bash -s <<'CONTAINER' 2>&1 | tee "$evidence_dir/install-uninstall.txt"
set -Eeuo pipefail
trap 'echo "ERROR: package smoke container failed at line $LINENO: $BASH_COMMAND" >&2' ERR
export DEBIAN_FRONTEND=noninteractive
echo 'Environment: fresh ubuntu:22.04 amd64 container; synthetic Xvfb, not GNOME'
cat /etc/os-release
echo 'Container dpkg path exclusion/inclusion rules before the test exception:'
grep -R -n -E '^[[:space:]]*path-(exclude|include)' /etc/dpkg/dpkg.cfg /etc/dpkg/dpkg.cfg.d || true
# Ubuntu's minimal container excludes most /usr/share/doc content at unpack
# time. Preserve only this application's docs so installed provenance can be
# tested; all dependency documentation keeps the container's default policy.
printf '%s\n' \
  'path-include=/usr/share/doc/worksidekick' \
  'path-include=/usr/share/doc/worksidekick/*' \
  > /etc/dpkg/dpkg.cfg.d/zz-worksidekick-docs
echo 'Test-specific dpkg exception:'
cat /etc/dpkg/dpkg.cfg.d/zz-worksidekick-docs
apt-get update
# Only the package and test harness are requested. Runtime libraries must be
# supplied by the DEB dependency declarations, not the CI build environment.
apt-get install -y --no-install-recommends /package/worksidekick.deb xvfb xauth dbus-x11 desktop-file-utils gnome-keyring
echo 'Resolved runtime package versions (distribution repositories, not a pinned apt snapshot):'
dpkg-query -W -f='${binary:Package}=${Version}\n'
echo 'Checking installed package version, architecture and absence of Qt development dependencies'
test "$(dpkg-query -W -f='${Version}' worksidekick)" = '0.7.0~rc1'
test "$(dpkg-query -W -f='${Architecture}' worksidekick)" = amd64
if dpkg-query -W -f='${Status}' qt6-base-dev 2>/dev/null | grep -q 'install ok installed'; then
  echo 'Unexpected Qt development dependency in clean runtime environment' >&2; exit 1
fi
echo 'Checking XCB platform plugin supplied by qt6-qpa-plugins'
dpkg-query -L qt6-qpa-plugins | grep -E '/platforms/libqxcb\.so$'
echo 'Checking Wayland platform plugins supplied by qt6-wayland'
dpkg-query -L qt6-wayland | grep -E '/platforms/libqwayland.*\.so$'
echo 'Checking installed binary library resolution'
ldd /usr/bin/worksidekick | tee /tmp/worksidekick-ldd.txt
if grep -q 'not found' /tmp/worksidekick-ldd.txt; then exit 1; fi
echo 'Checking desktop entry, icon and packaged build metadata'
desktop-file-validate /usr/share/applications/worksidekick.desktop
grep -qx 'Exec=worksidekick' /usr/share/applications/worksidekick.desktop
grep -qx 'Icon=worksidekick' /usr/share/applications/worksidekick.desktop
test -s /usr/share/icons/hicolor/scalable/apps/worksidekick.svg
cat /usr/share/doc/worksidekick/build-info.txt
useradd --create-home --uid 10001 smoke
install -d -m 700 -o smoke -g smoke /home/smoke/runtime /home/smoke/.config/WorkSidekick /home/smoke/.local/share/WorkSidekick
printf '%s\n' 'synthetic-settings-retention' > /home/smoke/.config/WorkSidekick/package-retention.txt
printf '%s\n' 'synthetic-userdata-retention' > /home/smoke/.local/share/WorkSidekick/package-retention.txt
chown -R smoke:smoke /home/smoke
# Test harness only: no probe or keyring daemon is shipped in the DEB. Each phase
# gets a fresh session bus and owned foreground daemon, but retains the same
# isolated home/keyring across package removal. No desktop user's vault is used.
cat > /tmp/worksidekick-keyring-phase.sh <<'PHASE'
#!/usr/bin/env bash
set -Eeuo pipefail
keyring_pid=''
cleanup() {
  if [[ -n "$keyring_pid" ]]; then
    kill "$keyring_pid" 2>/dev/null || true
    wait "$keyring_pid" 2>/dev/null || true
  fi
}
trap cleanup EXIT
printf '%s' 'synthetic-package-password' | gnome-keyring-daemon --foreground --unlock --components=secrets > /tmp/worksidekick-keyring-"$1".log 2>&1 &
keyring_pid=$!
ready=0
for attempt in {1..100}; do
  kill -0 "$keyring_pid"
  if dbus-send --session --print-reply --dest=org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus.NameHasOwner string:org.freedesktop.secrets | grep -q 'boolean true'; then ready=1; break; fi
  sleep 0.1
done
test "$ready" = 1
if [[ "$1" = '--seed' ]]; then
  # Verify the probe's refusal against pre-existing settings before seeding the
  # disposable account. This exact file is created and removed only by the test.
  settings_file="$XDG_CONFIG_HOME/WorkSidekick/WorkSidekick.conf"
  test ! -e "$settings_file"
  printf '%s\n' '[profile]' 'role=Synthetic pre-existing settings guard' > "$settings_file"
  original_hash=$(sha256sum "$settings_file")
  if WS_M6_PACKAGE_PROBE=1 WORKSIDEKICK_TEST_ISOLATED_KEYRING=1 timeout 30 /package/test_package_state_probe --seed; then
    echo 'Probe unexpectedly overwrote existing settings' >&2; exit 1
  fi
  test "$(sha256sum "$settings_file")" = "$original_hash"
  rm -- "$settings_file"
  echo 'PASS: probe refused existing settings and preserved their bytes'
fi
WS_M6_PACKAGE_PROBE=1 WORKSIDEKICK_TEST_ISOLATED_KEYRING=1 timeout 30 /package/test_package_state_probe "$1"
if [[ "$1" = '--seed' ]]; then
  xvfb-run -a timeout 30 /usr/bin/worksidekick --smoke-test
fi
PHASE
echo 'Checking installed application startup as a normal user under Xvfb'
runuser -u smoke -- env HOME=/home/smoke \
  XDG_CONFIG_HOME=/home/smoke/.config XDG_DATA_HOME=/home/smoke/.local/share \
  XDG_RUNTIME_DIR=/home/smoke/runtime QT_QPA_PLATFORM=xcb \
  dbus-run-session -- bash /tmp/worksidekick-keyring-phase.sh --seed
echo 'PASS: installed launch and production settings/4 distinct Secret Service provider adapter reads; provider deletion isolation verified'
# Capture files before removal; dpkg may keep directory entries shared with
# other packages, so check the actual files and links rather than directories.
mapfile -t installed_files < <(dpkg-query -L worksidekick | while IFS= read -r path; do
  if [[ -f "$path" || -L "$path" ]]; then printf '%s\n' "$path"; fi
done)
apt-get remove -y worksidekick
for path in "${installed_files[@]}"; do
  if [[ -e "$path" || -L "$path" ]]; then
    echo "Package file survived uninstall: $path" >&2; exit 1
  fi
done
grep -qx 'synthetic-settings-retention' /home/smoke/.config/WorkSidekick/package-retention.txt
grep -qx 'synthetic-userdata-retention' /home/smoke/.local/share/WorkSidekick/package-retention.txt
runuser -u smoke -- env HOME=/home/smoke \
  XDG_CONFIG_HOME=/home/smoke/.config XDG_DATA_HOME=/home/smoke/.local/share \
  XDG_RUNTIME_DIR=/home/smoke/runtime QT_QPA_PLATFORM=xcb \
  dbus-run-session -- bash /tmp/worksidekick-keyring-phase.sh --verify
echo 'PASS: uninstall removed package files and retained real QSettings/Profile/preferences and 4 distinct keyring credentials'
echo 'NOT TESTED: real GNOME X11/Wayland, tray, main executable AI credential use, previous-release upgrade'
CONTAINER
