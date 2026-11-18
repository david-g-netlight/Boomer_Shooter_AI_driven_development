#!/usr/bin/env bash
# Builds the game in Release (editor off) and packages it for another Mac:
#   dist/BoomerShooter/            the game folder: BoomerShooter (executable) + assets/
#   dist/BoomerShooter-macos.zip   the same, zipped
# The executable links raylib, Bullet and the C++ runtime statically; it only needs the macOS system frameworks.
# Usage: tools/package_release.sh
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
build="$root/build-release"
out="$root/dist/BoomerShooter"

cmake -S "$root" -B "$build" -DCMAKE_BUILD_TYPE=Release
cmake --build "$build" --parallel

rm -rf "$root/dist"
mkdir -p "$out"
cp "$build/my_app" "$out/BoomerShooter"
cp -R "$root/assets" "$out/assets"
rm -f "$out/assets/README.md"            # developer notes, not game data
cp "$root/README.md" "$out/README.md"

(cd "$root/dist" && zip -qr BoomerShooter-macos.zip BoomerShooter)

echo
echo "Packaged: $out"
echo "Zip:      $root/dist/BoomerShooter-macos.zip"
echo "Linked libraries (should be system libraries only):"
otool -L "$out/BoomerShooter" | tail -n +2
