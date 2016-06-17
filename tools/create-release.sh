#!/bin/sh

set -e

if [ "$VOXIEBUILD_PATH_HDF5" = "" ]; then
    echo "VOXIEBUILD_PATH_HDF5 not set" >&2
    exit 1
fi
if [ "$VOXIEBUILD_URL_HDF5" = "" ]; then
    echo "VOXIEBUILD_URL_HDF5 not set" >&2
    exit 1
fi

POS="${0%/*}"; test "$POS" = "$0" && POS=.

cd "$POS/.."

VOXIE_TAG=$CI_BUILD_REF
if [ "$CI_BUILD_TAG" != "" ]; then
    VOXIE_TAG=$CI_BUILD_TAG
fi

rm -rf build/release/install build/release/*-lin*.tar.gz voxie-*lin*.tar.gz manual.pdf

tools/build.sh --verbose --hdf5-path=$VOXIEBUILD_PATH_HDF5
INSTALL_ROOT=$(pwd)/build/release/install tools/build.sh --verbose install

if ! TAG="$(git describe --always)"; then
    TAG=
fi
TAG="voxie-${TAG#voxie-}"
mv build/release/install/usr/local/lib/voxie "build/release/install/$TAG"
cp COPYING "build/release/install/$TAG/"
cp lib/COPYING.hdf5 "build/release/install/$TAG/"
cp "$VOXIEBUILD_PATH_HDF5/lib/libhdf5.s"* "build/release/install/$TAG/lib"
sed "
s<%HDF5_URL%<$VOXIEBUILD_URL_HDF5<g
s<%VOXIE_TAG%<$VOXIE_TAG<g
s<%VOXIE_COMMIT%<$CI_BUILD_REF<g
" < tools/README-linux.tmpl > "build/release/install/$TAG/README"

tar czf build/release/"$TAG-lin64.tar.gz" -C build/release/install "$TAG"

cp build/release/"$TAG-lin64.tar.gz" .
#cp build/extra/manual.pdf .

rm -rf build/release/install
