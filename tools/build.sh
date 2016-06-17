#!/bin/sh

set -e

POS="${0%/*}"; test "$POS" = "$0" && POS=.
POS="$(readlink -f -- "$POS")"

: "${VOXIE_BUILD_DIR:=$POS/../build}"

mkdir -p "$VOXIE_BUILD_DIR"
cd "$VOXIE_BUILD_DIR"

FLAGS=-s
if [ "$1" = "--verbose" ]; then
    shift
    FLAGS=
fi

ASSIGN1="NOTHING="
if [ "$1" != "${1#--hdf5-path}" ]; then
    ASSIGN1="HDF5_PATH=${1#--hdf5-path}"
    shift
fi

qmake "$ASSIGN1" "$(readlink -f -- "$POS/../src")"
make $FLAGS -j16 "$@"
