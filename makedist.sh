#!/bin/sh
set -e

HERE=$(cd "$(dirname "$0")" && pwd)
CACHE=${CACHE:-$HOME/android-ndk-cache}
NDK_VER=${NDK_VER:-android-ndk-r29}
NDK_DIR="$CACHE/$NDK_VER"
PROJECT=${PROJECT:-sunstone}
STAMP=$(date +%Y%m%d)
DIST="$HERE/dist"
STAGE="$DIST/ghostlock-$PROJECT-$STAMP"
ZIP="$DIST/ghostlock-$PROJECT-$STAMP.zip"

fetch() {
  url=$1
  out=$2
  if [ -f "$out" ]; then
    echo "Cached $(basename "$out")"
    return 0
  fi
  echo "Fetching $(basename "$out")"
  mkdir -p "$(dirname "$out")"
  curl -sSL --retry 3 -o "$out.part" "$url"
  mv "$out.part" "$out"
}

mkdir -p "$CACHE"

if [ -d "$NDK_DIR" ]; then
  echo "Toolchain present at $NDK_DIR"
else
  fetch "https://dl.google.com/android/repository/$NDK_VER-linux.zip" "$CACHE/$NDK_VER-linux.zip"
  echo "Unpacking toolchain"
  unzip -q -o "$CACHE/$NDK_VER-linux.zip" -d "$CACHE"
fi
[ -d "$NDK_DIR" ] || { echo "Toolchain missing after unpack: $NDK_DIR"; exit 1; }

for os in linux darwin windows; do
  fetch "https://dl.google.com/android/repository/platform-tools-latest-$os.zip" \
        "$CACHE/platform-tools-$os.zip"
done

echo "Building preload"
ANDROID_NDK_HOME="$NDK_DIR" make -C "$HERE" PROJECT="$PROJECT" >/dev/null
PRELOAD="$HERE/build/$PROJECT/bin/preload"
[ -f "$PRELOAD" ] || { echo "Build produced no preload"; exit 1; }

rm -rf "$STAGE" "$ZIP"
mkdir -p "$STAGE/bin/linux" "$STAGE/bin/mac" "$STAGE/bin/windows"

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

unzip -q -o "$CACHE/platform-tools-linux.zip" -d "$TMP/linux"
cp "$TMP/linux/platform-tools/adb" "$STAGE/bin/linux/adb"
chmod 755 "$STAGE/bin/linux/adb"

unzip -q -o "$CACHE/platform-tools-darwin.zip" -d "$TMP/darwin"
cp "$TMP/darwin/platform-tools/adb" "$STAGE/bin/mac/adb"
chmod 755 "$STAGE/bin/mac/adb"

unzip -q -o "$CACHE/platform-tools-windows.zip" -d "$TMP/windows"
for f in adb.exe AdbWinApi.dll AdbWinUsbApi.dll; do
  cp "$TMP/windows/platform-tools/$f" "$STAGE/bin/windows/$f"
done

cp "$PRELOAD" "$STAGE/bin/preload"
cp "$HERE/root.sh" "$HERE/root.ps1" "$HERE/root.bat" "$STAGE/"
chmod 755 "$STAGE/root.sh"

( cd "$DIST" && zip -qr "$(basename "$ZIP")" "$(basename "$STAGE")" )

echo
echo "Wrote $ZIP"
unzip -l "$ZIP" | tail -n +4 | head -n -2
sha256sum "$ZIP"
