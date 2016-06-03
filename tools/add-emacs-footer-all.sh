#!/bin/sh

set -e

POS="${0%/*}"; test "$POS" = "$0" && POS=.

cd "$POS/.."

find src \( -name "*.cpp" -o -name "*.hpp" -o -name "*.cl" \) -print0 | xargs -0 tools/add-emacs-footer.sh
