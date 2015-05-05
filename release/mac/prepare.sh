#!/bin/sh

set -e
set -x

SOURCE="../../build-mac"
TARGET="Archive/Applications/Utilities/TREZOR_Bridge"
TARGET_FINAL="Archive/Applications/Utilities/TREZOR Bridge"

mkdir -p "$TARGET/"

cp "$SOURCE/trezord" "$TARGET/trezord"

../../vendor/macdylibbundler/dylibbundler -od -b \
             -x "$TARGET/trezord" \
             -d "$TARGET/libs/" \
             -p "@executable_path/libs/"

mv "$TARGET" "$TARGET_FINAL"
