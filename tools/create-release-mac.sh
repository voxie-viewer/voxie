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

# TODO: Qt license (+ other licenses)

# TODO: HDF5 / boost (for ExtFileHdf5)
# TODO: lapack? (for PluginFitting)

# TODO: Download all files at the beginning?

set -e

POS="${0%/*}"; test "$POS" = "$0" && POS=.

cd "$POS/.."

if [ "$QT_VERSION" = "" ]; then
    echo "QT_VERSION environment variable not set" >&2
    exit 1
fi

PYTHON_VERSION="python-3.9.0-macosx10.9"
DBUS_VERSION="dbus-1.8.2"
#DBUS_PYTHON_VERSION="dbus-python-1.2.4"
DBUS_PYTHON_VERSION="dbus-python-1.2.16"

PYTHON_PACKAGES_ARGS="--python-tag=cp39 --abi-tag=cp39 --platform-tag=macosx_10_9_x86_64 --platform-tag=macosx_10_10_x86_64"

QT_SRC="qt-everywhere-src-${QT_VERSION}.tar.xz"

rm -rf build/licenses
mkdir -p build/licenses

tools/download_dep.py --license-output-dir=build/licenses --unpack "$DBUS_VERSION.tar.gz"

# Needed for license
tools/download_dep.py --license-only --license-output-dir=build/licenses "$DBUS_PYTHON_VERSION.tar.gz"
tools/download_dep.py --license-only --license-output-dir=build/licenses "$PYTHON_VERSION.pkg"
tools/macos/unpack-python-packages.sh --license-only $PYTHON_PACKAGES_ARGS --license-output-dir="build/licenses"
tools/download_dep.py --license-only --license-output-dir=build/licenses "$QT_SRC"

if [ "$1" = "--meson" ]; then
    shift
fi

MESONLOC=
if [ "$1" = "--no-unpack-meson" ]; then
    shift
else
    MESONVERSION=0.56.2
    #MESONVERSION=0.57.0
    tools/download_dep.py --unpack "meson-$MESONVERSION.tar.gz"
    MESONLOC="--meson-binary=../tools/unpack-build-dep/meson-$MESONVERSION.tar.gz/meson-$MESONVERSION/meson.py"
fi

if [ "$1" = "--no-unpack-ninja" ]; then
    shift
else
    tools/download_dep.py --unpack "ninja-mac.zip"
    export PATH="$(pwd)/tools/unpack-build-dep/ninja-mac.zip:$PATH"
fi

# if [ "$VOXIEBUILD_PATH_HDF5" = "" ]; then
#     HDF5_BIN="hdf5-1.10.0-patch1-linux-centos7-x86_64-gcc485-shared.tar.gz"
#     HDF5_SRC="hdf5-1.10.0-patch1.tar.bz2"
#     tools/download_dep.py --license-output-dir=build/licenses --unpack "$HDF5_BIN"
#     VOXIEBUILD_PATH_HDF5="tools/unpack-build-dep/$HDF5_BIN/${HDF5_BIN%.tar.gz}"
#     if [ "$OS" = "Darwin" ]; then
#         VOXIEBUILD_PATH_HDF5="$(perl -e 'use Cwd "abs_path"; print abs_path(@ARGV[0])' -- "$VOXIEBUILD_PATH_HDF5")"
#     else
#         VOXIEBUILD_PATH_HDF5="$(readlink -f -- "$VOXIEBUILD_PATH_HDF5")"
#     fi
# fi

# if [ "$VOXIEBUILD_PATH_HDF5" = "" ]; then
#     echo "VOXIEBUILD_PATH_HDF5 not set" >&2
#     exit 1
# fi

# VOXIE_TAG=$CI_BUILD_REF
# if [ "$CI_BUILD_TAG" != "" ]; then
#     VOXIE_TAG=$CI_BUILD_TAG
# fi
# VOXIE_REF=$CI_BUILD_REF
VOXIE_REF="$(git rev-parse HEAD)"
VOXIE_REF_SHORT="$(git rev-parse --short=10 "$VOXIE_REF")"
VOXIE_TAG="$VOXIE_REF"

rm -rf build/release/install build/release/*-mac*.tar.gz voxie-*mac*.tar.gz build/release/*.dmg voxie-*mac*.dmg old-manual.pdf

mkdir -p "build/release/install"

if [ "$1" != "--skip-build" ]; then
    tools/build.sh $MESONLOC --verbose "--hdf5-path=$VOXIEBUILD_PATH_HDF5" "--additional-licenses-file=build/licenses/list.jsonl" "$@"
    DESTDIR=$(pwd)/build/release/install tools/build.sh $MESONLOC --verbose "--hdf5-path=$VOXIEBUILD_PATH_HDF5" "--additional-licenses-file=build/licenses/list.jsonl" "$@" install
fi

tools/macos/build-dbus.sh "$DBUS_VERSION"

tools/macos/unpack-python.sh "$PYTHON_VERSION"

tools/macos/build-dbus-python.sh "$DBUS_PYTHON_VERSION"

tools/macos/unpack-python-packages.sh $PYTHON_PACKAGES_ARGS

# https://developer.apple.com/library/archive/documentation/CoreFoundation/Conceptual/CFBundles/BundleTypes/BundleTypes.html

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

TDIR="$TAG.app"
mkdir "build/release/install/$TDIR"
mkdir "build/release/install/$TDIR/Contents"
if [ "$1" != "--skip-build" ]; then
    mv build/release/install/usr/local/lib/voxie "build/release/install/$TDIR/Contents/Resources"
else
    mkdir "build/release/install/$TDIR/Contents/Resources"
fi
cp LICENSE "build/release/install/$TDIR/Contents/Resources/"
#cp "$VOXIEBUILD_PATH_HDF5/lib/libhdf5.s"* "build/release/install/$TDIR/Contents/Resources/lib"
sed "
s<%VOXIE_TAG%<$VOXIE_TAG<g
s<%VOXIE_COMMIT%<$CI_BUILD_REF<g
" < tools/README-macos.tmpl > "build/release/install/$TDIR/Contents/Resources/README"
if [ "$1" != "--skip-build" ]; then
    mv "build/release/install/$TDIR/Contents/Resources/lib" "build/release/install/$TDIR/Contents/Frameworks"
    mkdir "build/release/install/$TDIR/Contents/Resources/lib"
    mv "build/release/install/$TDIR/Contents/Frameworks/katex"* "build/release/install/$TDIR/Contents/Resources/lib" # TODO
    # mv "build/release/install/$TDIR/Contents/Resources/plugins" "build/release/install/$TDIR/Contents/Plugins"
    mkdir "build/release/install/$TDIR/Contents/MacOS"
    mv "build/release/install/$TDIR/Contents/Resources/bin/voxie" "build/release/install/$TDIR/Contents/MacOS/voxie.bin"
    mv "build/release/install/$TDIR/Contents/Resources/bin/voxie-path.json" "build/release/install/$TDIR/Contents/MacOS/voxie-path.json"
    rm -f "build/release/install/$TDIR/Contents/Resources/voxie"
    cp src/extra/wrapper-script/voxie.macos "build/release/install/$TDIR/Contents/MacOS/voxie"
else
    mkdir "build/release/install/$TDIR/Contents/Frameworks"
fi
cp src/extra/Info.plist "build/release/install/$TDIR/Contents/"

if [ "$1" != "--skip-build" ]; then
    echo "==================="
    echo "Copying Qt files..."
    # Rename main file to make macdeployqt happy
    mv "build/release/install/$TDIR/Contents/MacOS/voxie" "build/release/install/$TDIR/Contents/MacOS_voxie"
    mv "build/release/install/$TDIR/Contents/MacOS/voxie.bin" "build/release/install/$TDIR/Contents/MacOS/voxie"
    macdeployqt "build/release/install/$TDIR" -libpath="build/release/install/$TDIR/Contents/Frameworks" -verbose=2
    mv "build/release/install/$TDIR/Contents/MacOS/voxie" "build/release/install/$TDIR/Contents/MacOS/voxie.bin"
    mv "build/release/install/$TDIR/Contents/MacOS_voxie" "build/release/install/$TDIR/Contents/MacOS/voxie"
    echo "==================="
    # QtWebEngineCore... is too large, artifact is rejected by build server => Works when increasing the setting in the gitlab CI admin panel https://stackoverflow.com/questions/57367224/how-to-increase-maximum-artifacts-size-for-gitlab-on-premises
    #echo "Removing QtWebEngineCore..."
    #rm -rf "build/release/install/$TDIR/Contents/Frameworks/QtWebEngineCore.framework"
    echo "==================="
fi

echo "Copying python + libraries..."
cp -a build/dbus-install/lib/libdbus-1.* "build/release/install/$TDIR/Contents/Frameworks"
cp -a build/python-install/Python.framework "build/release/install/$TDIR/Contents/Frameworks"
TARGETDIR="$(echo "build/release/install/$TDIR/Contents/Frameworks/Python.framework/Versions/Current/lib/"python*)"
cp -a build/dbus-python-install/lib/python*/site-packages/* "$TARGETDIR"
rm -f "$TARGETDIR"/*.la
echo "==================="

echo "Creating .dmg file..."
hdiutil create build/release/new.dmg -ov -volname "Voxie" -fs HFS+ -srcfolder "build/release/install/$TAG.app"
echo "Converting .dmg file..."
hdiutil convert build/release/new.dmg -format UDZO -o "build/release/$TAG-mac64.dmg"
echo "Cleaning up..."
rm -f build/release/new.dmg

#tar czf build/release/"$TAG-mac64.tar.gz" -C build/release/install "$TDIR"

echo "Copy .dmg file..."
cp build/release/"$TAG-mac64.dmg" .
#cp build/extra/old-manual.pdf .

#rm -rf build/release/install
echo "Done."
