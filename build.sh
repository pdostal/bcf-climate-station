#!/bin/bash
set -eu -o xtrace

rm -rf out_release
mkdir out_release

make release PIXEL_COUNT=72
mv out/release/firmware.bin out_release/firmware-72pixel.bin

make release PIXEL_COUNT=142
mv out/release/firmware.bin out_release/firmware-142pixel.bin

make release PIXEL_COUNT=144
mv out/release/firmware.bin out_release/firmware-144pixel.bin
