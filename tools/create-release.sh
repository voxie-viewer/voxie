#!/bin/sh
#
# Copyright (c) 2014-2022 The Voxie Authors
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

set -e

POS="${0%/*}"; test "$POS" = "$0" && POS=.

cd "$POS/.."

if [ "$1" = "--meson" ]; then
    shift
fi

rm -rf build/licenses
mkdir -p build/licenses

MESONLOC=
if [ "$1" = "--unpack-meson" ]; then
    # MESONVERSION=0.56.2
    # #MESONVERSION=0.57.0
    # tools/download_dep.py --unpack "meson-$MESONVERSION.tar.gz"
    # MESONLOC="--meson-binary=../tools/unpack-build-dep/meson-$MESONVERSION.tar.gz/meson-$MESONVERSION/meson.py"
    MESONLOC="--unpack-meson"
    shift
fi

if [ "$1" = "--unpack-ninja" ]; then
    tools/download_dep.py --unpack "ninja-linux.zip"
    export PATH="$(pwd)/tools/unpack-build-dep/ninja-linux.zip:$PATH"
    shift
fi

if [ "$VOXIEBUILD_PATH_HDF5" = "" ]; then
    HDF5_BIN="hdf5-1.10.0-patch1-linux-centos7-x86_64-gcc485-shared.tar.gz"
    HDF5_SRC="hdf5-1.10.0-patch1.tar.bz2"
    tools/download_dep.py --license-output-dir=build/licenses --unpack "$HDF5_BIN"
    VOXIEBUILD_PATH_HDF5="tools/unpack-build-dep/$HDF5_BIN/${HDF5_BIN%.tar.gz}"
fi

if [ "$VOXIEBUILD_PATH_HDF5" = "" ]; then
    echo "VOXIEBUILD_PATH_HDF5 not set" >&2
    exit 1
fi

if [ "$VOXIEBUILD_PATH_LIBJPEG" = "" ]; then
    LIBJPEG_BIN="libjpeg-turbo-official_3.0.2_amd64.deb"
    LIBJPEG_SRC="libjpeg-turbo-3.0.2.tar.gz"
    tools/download_dep.py --license-output-dir=build/licenses --unpack "$LIBJPEG_BIN"
    VOXIEBUILD_PATH_LIBJPEG="tools/unpack-build-dep/$LIBJPEG_BIN/opt/libjpeg-turbo"
fi

# VOXIE_TAG=$CI_BUILD_REF
# if [ "$CI_BUILD_TAG" != "" ]; then
#     VOXIE_TAG=$CI_BUILD_TAG
# fi
# VOXIE_REF=$CI_BUILD_REF
VOXIE_REF="$(git rev-parse HEAD)"
VOXIE_REF_SHORT="$(git rev-parse --short=10 "$VOXIE_REF")"
VOXIE_TAG="$VOXIE_REF"

rm -rf build/release/install build/release/*-lin*.tar.gz voxie-*lin*.tar.gz

BUILD_ARGS="--no-use-system-cmark-gfm"

tools/build $MESONLOC --verbose "--hdf5-path=$VOXIEBUILD_PATH_HDF5" "--additional-licenses-file=build/licenses/list.jsonl" $BUILD_ARGS "$@"
DESTDIR=$(pwd)/build/release/install tools/build $MESONLOC --verbose "--hdf5-path=$VOXIEBUILD_PATH_HDF5" "--libjpeg-path=$VOXIEBUILD_PATH_LIBJPEG" "--additional-licenses-file=build/licenses/list.jsonl" $BUILD_ARGS "$@" install

VOXIE_VERSION="$(cat src/version.txt)"
VOXIE_VERSION="${VOXIE_VERSION%\"}"
VOXIE_VERSION="${VOXIE_VERSION#\"}"
VOXIE_VERSION_SUFFIX=""
if [ -f "intern/version-suffix.txt" ]; then
    VOXIE_VERSION_SUFFIX="$(cat intern/version-suffix.txt)"
    VOXIE_VERSION_SUFFIX="${VOXIE_VERSION_SUFFIX%\"}"
    VOXIE_VERSION_SUFFIX="${VOXIE_VERSION_SUFFIX#\"}"
    VOXIE_VERSION_SUFFIX="$VOXIE_VERSION_SUFFIX"
fi
TAG="voxie-${VOXIE_VERSION}${VOXIE_VERSION_SUFFIX}-${VOXIE_REF_SHORT}"

echo "Voxie version is: '$TAG'"

mv build/release/install/usr/local/lib/voxie "build/release/install/$TAG"
cp LICENSE "build/release/install/$TAG/"
cp "$VOXIEBUILD_PATH_HDF5/lib/libhdf5.s"* "build/release/install/$TAG/lib"
sed "
s<%VOXIE_TAG%<$VOXIE_TAG<g
s<%VOXIE_COMMIT%<$VOXIE_REF<g
" < tools/README-linux.tmpl > "build/release/install/$TAG/README"

tar czf build/release/"$TAG-lin64.tar.gz" -C build/release/install "$TAG"

cp build/release/"$TAG-lin64.tar.gz" .

# Build documentation
echo "Building documentation..."
rm -rf build/doc
mkdir build/doc
LD_LIBRARY_PATH="build/release/install/$TAG/lib${LD_LIBRARY_PATH+:$LD_LIBRARY_PATH}" ./voxie.sh --no-opencl --dbus-p2p --output-help-directory build/doc
tar cjf doc.tar.bz2 -C build doc
echo "Building documentation done"

rm -rf build/release/install
