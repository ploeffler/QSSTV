#!/usr/bin/env bash
# Run in the MINGW64 build shell. Outputs exactly one portable release file.
set -euo pipefail
repo="$(cd "$(dirname "$0")/.." && pwd)"
cd "$repo"
[[ ! -e dist-windows && ! -e out-windows ]] || { echo 'Use fresh packaging directories' >&2;exit 1; }
mkdir dist-windows out-windows
if [[ -f build-windows/release/qsstv.exe ]]; then
  cp build-windows/release/qsstv.exe dist-windows/
else
  cp build-windows/qsstv.exe dist-windows/
fi
/mingw64/bin/windeployqt-qt5.exe --release --compiler-runtime dist-windows/qsstv.exe
plugins="$(cygpath -u "$(/mingw64/bin/qmake-qt5.exe -query QT_INSTALL_PLUGINS | tr -d '\r')")"
# Audio and image codecs can be loaded dynamically and are not ordinary imports.
for category in platforms audio imageformats bearer iconengines styles; do
  if [[ -d "$plugins/$category" ]]; then cp -r "$plugins/$category" dist-windows/;fi
done
test -f dist-windows/platforms/qwindows.dll
find dist-windows/audio -type f -iname '*.dll' -print -quit | grep .
printf '[Paths]\nPlugins=.\n' > dist-windows/qt.conf
# OpenSSL is runtime-loaded by Qt Network on some MSYS2 versions.
for dll in /mingw64/bin/libssl-*.dll /mingw64/bin/libcrypto-*.dll; do
  if [[ -f "$dll" ]]; then cp "$dll" dist-windows/;fi
done
g++ -std=c++11 -O2 -municode -mwindows -static -static-libgcc -static-libstdc++ \
  packaging/windows/portable-launcher.cpp -o dist-windows/portable-launcher.exe
bash scripts/collect-windows-dlls.sh dist-windows /mingw64 "$(cygpath -u "$SYSTEMROOT")/System32"
cp COPYING dist-windows/COPYING.txt
mkdir -p dist-windows/licenses
# Include the installed dependency notices with the bundled runtime.
cp -r /mingw64/share/licenses/. dist-windows/licenses/
version="$(sed -n 's/^const QString MAJORVERSION  = "\([0-9.]*\)";.*/\1/p' src/appglobal.cpp)$(sed -n 's/^const QString MINORVERSION  = "\([0-9.]*\)";.*/\1/p' src/appglobal.cpp)"
[[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || { echo 'Cannot determine source version' >&2;exit 1; }
python scripts/package-windows-portable.py dist-windows "out-windows/QSSTV-${version}-Windows-x86_64-portable.exe"
