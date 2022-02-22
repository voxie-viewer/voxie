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

# Assumes 'tools/download_dep.py --unpack "$DBUS_VERSION.tar.gz"' has already been called

if [ "$#" != 1 ]; then
    echo "Usage: $0 <dbus-version>"
    exit 1
fi

DBUS_VERSION="$1"
shift

if [ ! -f "build/dbus.stamp" ]; then
    echo "Building dbus..."
    rm -rf build/dbus.stamp build/dbus build/dbus-install
    mkdir build/dbus
    cd build/dbus
    "../../tools/unpack-build-dep/$DBUS_VERSION.tar.gz/$DBUS_VERSION/configure" --prefix="$(pwd)/../dbus-install" --with-launchd-agent-dir="$(pwd)/../dbus-install/Library/LaunchAgents"
    make -j16
    make install
    cd ../..
    # Set library name to 'libdbus-1.dylib'
    install_name_tool -id '@rpath/libdbus-1.dylib' build/dbus-install/lib/libdbus-1.dylib
    echo "Building dbus done"
    touch build/dbus.stamp
else
    echo "dbus already built"
fi
