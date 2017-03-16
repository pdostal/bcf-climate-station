#!/bin/bash
set -eu -o xtrace

mkdir out_release

make release PIXEL_COUNT=72
mv out/firmware.bin out_release/firmware-72pixel.bin

make release PIXEL_COUNT=142
mv out/firmware.bin out_release/firmware-142pixel.bin

make release PIXEL_COUNT=144
mv out/firmware.bin out_release/firmware-144pixel.bin
