#!/usr/bin/env bash
# Limited Level C packaged-runtime smoke in a clean container, not a real
# GNOME X11/Wayland desktop acceptance gate.
set -euo pipefail
package=$(realpath -- "${1:?Usage: test-linux-package.sh package.deb [evidence-dir]}")
evidence_dir=$(realpath -m -- "${2:-$(dirname -- "$package")}")
mkdir -p -- "$evidence_dir"
docker run --rm -i --platform linux/amd64 \
  --mount "type=bind,source=$package,target=/package/worksidekick.deb,readonly" \
  ubuntu:22.04@sha256:076e7bf82319619f28dd0aa9c02ab34b055ad37100395b4f49a2ffba04fb5e11 bash -s <<'CONTAINER' 2>&1 | tee "$evidence_dir/install-uninstall.txt"
set -euo pipefail
export DEBIAN_FRONTEND=noninteractive
echo 'Environment: fresh ubuntu:22.04 amd64 container; synthetic Xvfb, not GNOME'
cat /etc/os-release
apt-get update
# Only the package and test harness are requested. Runtime libraries must be
# supplied by the DEB dependency declarations, not the CI build environment.
apt-get install -y --no-install-recommends /package/worksidekick.deb xvfb xauth dbus-x11 desktop-file-utils
echo 'Resolved runtime package versions (distribution repositories, not a pinned apt snapshot):'
dpkg-query -W -f='${binary:Package}=${Version}\n'
test "$(dpkg-query -W -f='${Version}' worksidekick)" = '0.7.0~rc1'
test "$(dpkg-query -W -f='${Architecture}' worksidekick)" = amd64
if dpkg-query -W -f='${Status}' qt6-base-dev 2>/dev/null | grep -q 'install ok installed'; then
  echo 'Unexpected Qt development dependency in clean runtime environment' >&2; exit 1
fi
dpkg-query -L libqt6gui6 | grep -E '/platforms/libqxcb\.so$'
dpkg-query -L qt6-wayland | grep -E '/platforms/libqwayland.*\.so$'
ldd /usr/bin/worksidekick | tee /tmp/worksidekick-ldd.txt
if grep -q 'not found' /tmp/worksidekick-ldd.txt; then exit 1; fi
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
runuser -u smoke -- env HOME=/home/smoke \
  XDG_CONFIG_HOME=/home/smoke/.config XDG_DATA_HOME=/home/smoke/.local/share \
  XDG_RUNTIME_DIR=/home/smoke/runtime QT_QPA_PLATFORM=xcb \
  dbus-run-session -- xvfb-run -a timeout 30 /usr/bin/worksidekick --smoke-test
echo 'PASS: installed binary launched as non-root with isolated synthetic settings'
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
echo 'PASS: uninstall removed package files and retained synthetic settings/user data'
echo 'NOT TESTED: real GNOME X11/Wayland, tray, Secret Service, previous-release upgrade'
CONTAINER
