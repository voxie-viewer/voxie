#!/bin/sh

set -e

POS="${0%/*}"; test "$POS" = "$0" && POS=.
POS="$(readlink -f -- "$POS")"

: "${VOXIE_BUILD_DIR:=$POS/../build}"

mkdir -p "$VOXIE_BUILD_DIR"
cd "$VOXIE_BUILD_DIR"

qmake "$(readlink -f -- "$POS/../src")"
make -sj16 "$@"
