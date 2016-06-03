#!/bin/sh

set -e

POS="${0%/*}"; test "$POS" = "$0" && POS=.

cd "$POS/.."

rm -rf build/release/install build/release/*-lin.tar.gz

INSTALL_ROOT=$(pwd)/build/release/install tools/build.sh install

if ! TAG="$(git describe --always --tags)"; then
    TAG=
fi
TAG="voxie-${TAG#voxie-}"
mv build/release/install/usr/local/lib/voxie "build/release/install/$TAG"

tar czf build/release/"$TAG-lin.tar.gz" -C build/release/install "$TAG"

rm -rf build/release/install
