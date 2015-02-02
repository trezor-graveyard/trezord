#!/bin/sh

SOURCE="../../build"
TARGET="Archive/Applications/Utilities/TREZOR_Bridge"
TARGET_FINAL="Archive/Applications/Utilities/TREZOR Bridge"

mkdir -p "$TARGET/"

cp "$SOURCE/trezord" "$TARGET/trezord"

dylibbundler -od -b \
             -x "$TARGET/trezord" \
             -d "$TARGET/libs/" \
             -p "@executable_path/libs/"

mv "$TARGET" "$TARGET_FINAL"
