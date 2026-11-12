#!/bin/sh
# Render the boot eyes on the host: preview.sh lid iris lashes open gazeX gazeY out.ppm
# Indices are positions in lids::kStyles / iriscolor::kIrises.
set -e
here=$(cd "$(dirname "$0")" && pwd)
src="$here/../../firmware/src/core/booteyes"
bin="${TMPDIR:-/tmp}/eye_preview"
c++ -std=c++17 -O2 -DBOOT_EYES -I"$here/stubs" -I"$src" "$here/preview.cpp" "$src/EyeRenderer.cpp" -o "$bin"
"$bin" "$@"
