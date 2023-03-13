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
OS="$(uname)"
if [ "$OS" = "Darwin" ]; then
    # https://stackoverflow.com/questions/3572030/bash-script-absolute-path-with-osx/30795461#30795461
    POS="$(perl -e 'use Cwd "abs_path"; print abs_path(@ARGV[0])' -- "$POS")"
else
    POS="$(readlink -f -- "$POS")"
fi

if [ "$1" = "--meson" ]; then
    shift
fi

ASSIGN2="-Ddebug=false -Doptimization=3"
if [ "$1" = "--debug" ]; then
    ASSIGN2="-Ddebug=true -Doptimization=0"
    shift
    : "${VOXIE_BUILD_DIR:=$POS/../build-debug}"
else
    : "${VOXIE_BUILD_DIR:=$POS/../build}"
fi

if [ -f "$VOXIE_BUILD_DIR/Makefile" ]; then
    # Remove qmake build directory
    rm -rf "$VOXIE_BUILD_DIR"
fi
mkdir -p "$VOXIE_BUILD_DIR"
cd "$VOXIE_BUILD_DIR"

MESON=meson
if [ "$1" != "${1#--meson-binary=}" ]; then
    MESON="${1#--meson-binary=}"
    shift
elif [ "$1" = "--unpack-meson" ]; then
    MESONVERSION=0.56.2
    #MESONVERSION=0.57.0
    "../tools/download_dep.py" --unpack "meson-$MESONVERSION.tar.gz"
    MESON="../tools/unpack-build-dep/meson-$MESONVERSION.tar.gz/meson-$MESONVERSION/meson.py"
    shift
fi

MESON_OPT=

MESON_OPT="$MESON_OPT -Dboost_include_path="
MESON_OPT="$MESON_OPT -Dlapacke_path="

FLAGS=
if [ "$1" = "--verbose" ]; then
    shift
    FLAGS=-v
fi

if [ "$1" != "${1#--hdf5-path=}" ]; then
    ASSIGN1="-Dhdf5_path=${1#--hdf5-path=}"
    shift
else
    ASSIGN1="-Dhdf5_path="
fi

if [ "$1" != "${1#--additional-licenses-file=}" ]; then
    ASSIGN3="-Dadditional_licenses_file=${1#--additional-licenses-file=}"
    shift
else
    ASSIGN3="-Dadditional_licenses_file="
fi

if [ "$1" = "--disable-help-browser" ]; then
    MESON_OPT="$MESON_OPT -Dhelp_browser=disabled"
    shift
else
    MESON_OPT="$MESON_OPT -Dhelp_browser=enabled"
fi

if [ "$1" = "--disable-hdf5" ]; then
    MESON_OPT="$MESON_OPT -Dhdf5=disabled"
    shift
else
    MESON_OPT="$MESON_OPT -Dhdf5=enabled"
fi

if [ "$1" = "--disable-lapack" ]; then
    MESON_OPT="$MESON_OPT -Dlapack=disabled"
    shift
else
    MESON_OPT="$MESON_OPT -Dlapack=enabled"
fi

if [ "$1" = "--only-lib" ]; then
    MESON_OPT="$MESON_OPT -Dlib=enabled -Dlibvoxieclient=enabled -Dlibvoxiebackend=enabled -Dlibvoxie=disabled -Dmain=disabled -Dplugins=disabled -Dext=disabled -Dextra=disabled -Dtest=disabled"
    shift
elif [ "$1" = "--only-manual" ]; then
    MESON_OPT="$MESON_OPT -Dlib=disabled -Dlibvoxieclient=disabled -Dlibvoxiebackend=disabled -Dlibvoxie=disabled -Dmain=disabled -Dplugins=disabled -Dext=disabled -Dextra=enabled -Dtest=disabled"
    shift
else
    MESON_OPT="$MESON_OPT -Dlib=enabled -Dlibvoxieclient=enabled -Dlibvoxiebackend=enabled -Dlibvoxie=enabled -Dmain=enabled -Dplugins=enabled -Dext=enabled -Dextra=enabled -Dtest=enabled"
fi

if [ "$1" = "--no-intern" ]; then
    MESON_OPT="$MESON_OPT -Dintern=disabled"
    shift
else
    MESON_OPT="$MESON_OPT -Dintern=enabled"
fi

MESON_RECONF=--reconfigure
if [ ! -f "build.ninja" ]; then
    MESON_RECONF=
fi
"$MESON" .. $MESON_RECONF $MESON_OPT "$ASSIGN1" $ASSIGN2 "$ASSIGN3"
ninja $FLAGS "$@"
