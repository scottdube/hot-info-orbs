#!/bin/sh
# Render the boot eyes on the host: preview.sh lid iris lashes open gazeX gazeY out.ppm
# Indices are positions in lids::kStyles / iriscolor::kIrises.
# open is 0..256 (0 = lids shut, 256 = lids free); gaze is 0..1023 with 512 = centre.
# A fraction like 0.6 for open reads as 0 (shut), and gaze 0 0 looks hard into a corner.
# Night King, eyes open, looking ahead: preview.sh 10 9 0 256 512 512 out.ppm
set -e
here=$(cd "$(dirname "$0")" && pwd)
src="$here/../../firmware/src/core/booteyes"
bin="${TMPDIR:-/tmp}/eye_preview"
c++ -std=c++17 -O2 -DBOOT_EYES -I"$here/stubs" -I"$src" "$here/preview.cpp" "$src/EyeRenderer.cpp" -o "$bin"
"$bin" "$@"
