#!/bin/sh

set -e

POS="${0%/*}"; test "$POS" = "$0" && POS=.

cd "$POS/.."

rm -rf build/release/install build/release/*-lin*.tar.gz voxie-*lin*.tar.gz manual.pdf

rm -rf build/extra
mkdir -p build/extra
cd build/extra
qmake ../../src/extra
make manual.pdf

cp manual.pdf ../..
