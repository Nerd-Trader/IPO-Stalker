#!/bin/sh
# Turns build/ipo-stalker.app (cmake --build build) into a self-contained
# build/IPO Stalker.app and zips it as build/IPO-Stalker-macos-<arch>.zip (macOS only).
# ARCH is arm64 or x86_64 (defaults to this machine's); Qt's universal frameworks
# get thinned down to that architecture.
# Ad-hoc signed, not notarized: the first time, open it with right-click → Open.
set -eu

root=$(cd "$(dirname "$0")/.." && pwd)
BUILD=${BUILD:-$root/build}
ARCH=${ARCH:-$(uname -m)}
src="$BUILD/ipo-stalker.app"
app="$BUILD/IPO Stalker.app"
iconset="$BUILD/ipo-stalker.iconset"

case "$ARCH" in
    arm64)  suffix=aarch64 ;;
    x86_64) suffix=x86_64 ;;
    *) echo "Unsupported ARCH: $ARCH" >&2; exit 1 ;;
esac

rm -rf "$app" "$iconset"

# Copy Qt frameworks and plugins (platform, SQLite driver, etc) into the bundle
macdeployqt "$src" -always-overwrite

# Keep only the requested architecture
ditto --arch "$ARCH" "$src" "$app"

# Icon: render every size iconutil wants from the large source PNG
mkdir -p "$iconset"
for size in 16 32 128 256 512; do
    sips -z $size $size "$root/res/images/ipo-stalker.png" \
        --out "$iconset/icon_${size}x${size}.png" > /dev/null
    double=$((size * 2))
    sips -z $double $double "$root/res/images/ipo-stalker.png" \
        --out "$iconset/icon_${size}x${size}@2x.png" > /dev/null
done
iconutil -c icns "$iconset" -o "$app/Contents/Resources/ipo-stalker.icns"
rm -rf "$iconset"

# Apple silicon refuses to run unsigned code; an ad-hoc signature is enough
codesign --force --deep --sign - "$app"

cd "$BUILD"
rm -f "IPO-Stalker-macos-$suffix.zip"
ditto -c -k --keepParent "IPO Stalker.app" "IPO-Stalker-macos-$suffix.zip"
echo "$BUILD/IPO-Stalker-macos-$suffix.zip"
