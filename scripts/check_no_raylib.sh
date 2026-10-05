#!/bin/sh
# Past: 6d38b79/0981fdf/c09e4dc removed direct raylib three times; backend is afterhours::graphics (Sokol/Metal, WebGL web). Only src/rl.h + sokol_impl* may name raylib.
targets="${@:-src}"
if grep -rnE 'raylib::|#include [<"]raylib\.h' $targets --include='*.h' --include='*.cpp' --include='*.mm' 2>/dev/null | grep -vE 'src/rl\.h|sokol_impl'; then echo 'check_no_raylib: FAIL — use afterhours::graphics, see src/rl.h' >&2; exit 1; fi
echo 'check_no_raylib: ok'
