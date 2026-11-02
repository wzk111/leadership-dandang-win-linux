#!/usr/bin/env bash
# Build the release DEB and inspect the actual archive payload. Run on Ubuntu
# 22.04 amd64 with the documented build dependencies and dpkg-dev installed.
set -Eeuo pipefail
trap 'echo "ERROR: package build failed at line $LINENO: $BASH_COMMAND" >&2' ERR
repo_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
build_dir=$(realpath -m -- "${1:-$repo_dir/build}")
output_dir=$(realpath -m -- "${2:-$repo_dir/dist/linux}")
[[ $(dpkg --print-architecture) == amd64 ]] || {
  echo 'M6 Linux release packages require Ubuntu 22.04 amd64.' >&2; exit 1;
}
grep -qx 'ID=ubuntu' /etc/os-release
grep -qx 'VERSION_ID="22.04"' /etc/os-release
mkdir -p -- "$output_dir"
cmake -S "$repo_dir" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build "$build_dir" --config Release --parallel 2
cpack --config "$build_dir/CPackConfig.cmake" -G DEB -B "$output_dir"
package="$output_dir/worksidekick_0.7.0~rc1_amd64.deb"
test -s "$package"
dpkg-deb --info "$package" | tee "$output_dir/deb-info.txt"
dpkg-deb --contents "$package" | tee "$output_dir/deb-contents.txt"
dpkg-deb --field "$package" Depends > "$output_dir/deb-dependencies.txt"
for dependency in libqt6gui6 qt6-qpa-plugins qt6-wayland libsecret-1-0 libatspi2.0-0 libx11-6; do
  grep -Eq "(^|, )${dependency}( |,|$)" "$output_dir/deb-dependencies.txt" || {
    echo "Missing runtime dependency: $dependency" >&2; exit 1;
  }
done
payload_dir=$(mktemp -d)
trap 'rm -rf -- "$payload_dir"' EXIT
dpkg-deb --extract "$package" "$payload_dir"
test -x "$payload_dir/usr/bin/worksidekick"
test -f "$payload_dir/usr/share/applications/worksidekick.desktop"
test -f "$payload_dir/usr/share/icons/hicolor/scalable/apps/worksidekick.svg"
test -s "$payload_dir/usr/share/doc/worksidekick/build-info.txt"
grep -qx 'qt=6.2.4' "$payload_dir/usr/share/doc/worksidekick/build-info.txt"
dpkg-query -W -f='${binary:Package}=${Version}\n' \
  cmake dpkg-dev g++ libqt6core6 libqt6gui6 libqt6widgets6 libqt6network6 \
  libqt6concurrent6 libqt6dbus6 qt6-qpa-plugins qt6-wayland libsecret-1-0 libatspi2.0-0 libx11-6 \
  > "$output_dir/build-package-versions.txt"
python3 "$repo_dir/scripts/audit-package.py" "$payload_dir" --manifest "$output_dir/package-manifest.json"
cp -- "$payload_dir/usr/share/doc/worksidekick/build-info.txt" "$output_dir/build-info.txt"
# The generated manifest stays beside the artifact: including it inside the
# package would introduce a circular package checksum.
(cd -- "$output_dir" && sha256sum -- "$(basename -- "$package")" > SHA256SUMS.txt)
echo "Package and evidence: $output_dir"
