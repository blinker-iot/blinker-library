#!/bin/sh
set -eu

example_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_dir=$(CDPATH= cd -- "$example_dir/../.." && pwd)
aily_dir="${AILY_PROJECT_DIR:-$HOME/Library/aily-project}"
gateway_host="${1:?Usage: ./build.sh <Gateway LAN IP>}"

"$aily_dir/npm-global/bin/aily-builder" compile \
    "$example_dir/Esp32S3InterfaceDemo.ino" \
    --board esp32:esp32:esp32s3 \
    --sdk-path "$aily_dir/sdk/esp32_3.3.1" \
    --tools-path "$aily_dir/tools" \
    --build-path "$repo_dir/.build/esp32s3-interface" \
    --libraries-path "$repo_dir" \
    --libraries-path "$repo_dir/.build/libraries" \
    --build-macros "BLINKER_OFFICIAL_CONTROL_HOST=\"$gateway_host\"" \
    --build-macros BLINKER_OFFICIAL_CONTROL_PORT=17000 \
    --board-options CDCOnBoot=default \
    --board-options FlashSize=16M \
    --board-options PSRAM=disabled \
    --tool-versions 'esp-x32@14.2.0,esp32-arduino-libs@5.5.0,esptool_py@5.1.0' \
    --no-archive-cloud-cache \
    --no-fetch-archive-cloud-cache
