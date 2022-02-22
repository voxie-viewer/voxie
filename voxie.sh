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

BUILD_FLAGS=
if [ "$1" = "--meson" ]; then
    shift
fi

VOXIE_SUFFIX=/src

BUILD_FLAGS="$BUILD_FLAGS --meson"
if [ "$1" = "--debug" ]; then
    shift
    : "${VOXIE_BUILD_DIR:=$POS/build-debug}"
    BUILD_FLAGS="$BUILD_FLAGS --debug"
else
    : "${VOXIE_BUILD_DIR:=$POS/build}"
fi
export VOXIE_BUILD_DIR

if [ "$1" != "${1#--build-dir=}" ]; then
    VOXIE_BUILD_DIR="${1#--build-dir=}"
    shift
fi

VOXIE_BUILD_SRC="$VOXIE_BUILD_DIR$VOXIE_SUFFIX"
EXE="$VOXIE_BUILD_SRC/Main/voxie"

# If the first parameter is --build, try to build voxie
if [ "$1" = "--build" ]; then
    shift
    echo "Building voxie..."
    "$POS/tools/build.sh" $BUILD_FLAGS
    echo "Building voxie finished."
elif [ ! -f "$EXE" ]; then
    echo "Could not find $EXE" >&2
    echo "Run 'tools/build.sh' or './voxie.sh --build' to build voxie" >&2
    exit 1
fi

GDB=
if [ "$1" = "--gdb" ]; then
    GDB="gdb --args"
    shift
elif [ "$1" = "--valgrind" ]; then
    GDB="valgrind"
    shift
elif [ "$1" != "${1#--valgrind=}" ]; then
    GDB="valgrind ${1#--valgrind=}"
    shift
fi

# Workaround for https://bugreports.qt.io/browse/QTBUG-87014
if [ "$OS" = "Darwin" ]; then
    export QT_MAC_WANTS_LAYER=1
fi

# Run voxie
exec  $GDB "$EXE" "$@"
