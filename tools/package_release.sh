#!/usr/bin/env bash
# Builds the game in Release (editor off) and packages it for another Mac:
#   dist/BuildToScale/            the game folder: BuildToScale (executable) + assets/
#   dist/BuildToScale-macos.zip   the same, zipped
# The executable links raylib, Bullet and the C++ runtime statically; it only needs the macOS system frameworks.
# Usage: tools/package_release.sh
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
build="$root/build-release"
out="$root/dist/BuildToScale"

cmake -S "$root" -B "$build" -DCMAKE_BUILD_TYPE=Release
cmake --build "$build" --parallel

rm -rf "$root/dist"
mkdir -p "$out"
cp "$build/my_app" "$out/BuildToScale"
cp -R "$root/assets" "$out/assets"
rm -f "$out/assets/README.md"            # developer notes, not game data
cp "$root/README.md" "$out/README.md"

(cd "$root/dist" && zip -qr BuildToScale-macos.zip BuildToScale)

echo
echo "Packaged: $out"
echo "Zip:      $root/dist/BuildToScale-macos.zip"
echo "Linked libraries (should be system libraries only):"
otool -L "$out/BuildToScale" | tail -n +2
