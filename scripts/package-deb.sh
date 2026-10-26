#!/usr/bin/env bash
set -euo pipefail
if [[ $# -ne 2 ]]; then echo "Usage: $0 <compiled-qsstv> <version>" >&2; exit 2; fi
binary=$(realpath "$1")
version=$2
[[ -x "$binary" ]] || { echo "Executable not found: $binary" >&2; exit 1; }
[[ "$version" =~ ^[0-9][0-9A-Za-z.+:~_-]*$ ]] || { echo "Invalid Debian version" >&2; exit 1; }
command -v dpkg-shlibdeps >/dev/null || { echo "Install dpkg-dev" >&2; exit 1; }
arch=$(dpkg --print-architecture)
[[ "$arch" == amd64 ]] || { echo "Expected amd64, got $arch" >&2; exit 1; }
root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
icon="$root/src/icons/qsstv.png"
desktop="$root/src/qsstv.desktop"
[[ -f "$icon" && -f "$desktop" ]] || { echo "Missing icon or desktop entry" >&2; exit 1; }
mkdir -p "$root/out"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
pkg="$work/qsstv"
install -Dm755 "$binary" "$pkg/usr/bin/qsstv"
install -Dm644 "$icon" "$pkg/usr/share/icons/hicolor/128x128/apps/qsstv.png"
install -Dm644 "$desktop" "$pkg/usr/share/applications/qsstv.desktop"
mkdir -p "$pkg/DEBIAN" "$work/debian"
cat > "$work/debian/control" <<'CONTROL'
Source: qsstv
Section: hamradio
Priority: optional
Maintainer: QSSTV contributors <noreply@github.com>
Standards-Version: 4.6.0

Package: qsstv
Architecture: any
Description: QSSTV amateur radio SSTV and digital image application
CONTROL
(cd "$work"; dpkg-shlibdeps -O -e"$binary" > deps)
deps=$(sed -n 's/^shlibs:Depends=//p' "$work/deps")
[[ -n "$deps" ]] || { echo "No runtime dependencies resolved" >&2; exit 1; }
cat > "$pkg/DEBIAN/control" <<CONTROL
Package: qsstv
Version: $version
Section: hamradio
Priority: optional
Architecture: $arch
Maintainer: QSSTV contributors <noreply@github.com>
Depends: $deps
Description: QSSTV amateur radio image communication
 QSSTV receives and transmits SSTV and digital image modes.
 Experimental package built from QSSTV source.
CONTROL
out="$root/out/qsstv_${version}_${arch}.deb"
dpkg-deb --build --root-owner-group "$pkg" "$out"
dpkg-deb --info "$out"
echo "Created $out"
