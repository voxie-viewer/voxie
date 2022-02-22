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

if [ "$#" != 1 ]; then
    echo "Usage: $0 <dbus-python-version>"
    exit 1
fi

DBUS_PYTHON_VERSION="$1"
shift

PYTHONVERS="$(pwd)/build/python-install/Python.framework/Versions/Current"
PYTHONINC="$(echo "$PYTHONVERS/include"/*)"

if [ ! -f "build/dbus-python.stamp" ]; then
    echo "Building dbus-python..."
    rm -rf build/dbus-python.stamp build/dbus-python build/dbus-python-install

    rm -f "tools/unpack-build-dep/$DBUS_PYTHON_VERSION.tar.gz.stamp"
    rm -rf "tools/unpack-build-dep/$DBUS_PYTHON_VERSION.tar.gz"
    tools/download_dep.py --unpack "$DBUS_PYTHON_VERSION.tar.gz"

    # Patch source to avoid building GLib bindings
    # Makefile.am has to be patched before Makefile.in, otherwise automake will complain if the timestamp of Makefile.am is newer (only seems to happen if the patching happens in different seconds):
    # .../voxie-intern/tools/unpack-build-dep/dbus-python-1.2.16.tar.gz/dbus-python-1.2.16/build-aux/missing: line 81: automake-1.16: command not found
    # WARNING: 'automake-1.16' is missing on your system.
    #          You should only need it if you modified 'Makefile.am' or
    #          'configure.ac' or m4 files included by 'configure.ac'.
    # ...
    sed -i.bak 's,noinst_LTLIBRARIES =.*,,;/^._dbus_glib_bindings.la/d;s/SUBDIRS = dbus-gmain/SUBDIRS =/' "tools/unpack-build-dep/$DBUS_PYTHON_VERSION.tar.gz/$DBUS_PYTHON_VERSION/Makefile.am"
    sed -i.bak 's,noinst_LTLIBRARIES =.*,,;/^._dbus_glib_bindings.la/d;s/SUBDIRS = dbus-gmain/SUBDIRS =/' "tools/unpack-build-dep/$DBUS_PYTHON_VERSION.tar.gz/$DBUS_PYTHON_VERSION/Makefile.in"

    mkdir build/dbus-python
    cd build/dbus-python
    # Set flags like PYTHON_EXTRA_LDFLAGS to avoid prevent (broken) autoconfiguration
    "../../tools/unpack-build-dep/$DBUS_PYTHON_VERSION.tar.gz/$DBUS_PYTHON_VERSION/configure" --prefix="$(pwd)/../dbus-python-install" PYTHON_CONFIG=":" PYTHON_EXTRA_LDFLAGS="-L." PYTHON="$PYTHONVERS/bin/python3" PYTHON_LIBS="-L$PYTHONVERS/lib -lpython" PYTHON_INCLUDES="-I$PYTHONINC" --enable-shared --disable-static GLIB_CFLAGS="-I." GLIB_LIBS="-L." DBUS_GLIB_CFLAGS="-I." DBUS_GLIB_LIBS="-L." DBUS_CFLAGS="-I$(pwd)/../dbus-install/include/dbus-1.0 -I$(pwd)/../dbus-install/lib/dbus-1.0/include" DBUS_LIBS="-L$(pwd)/../dbus-install/lib -ldbus-1"
    make -j16
    make install
    cd ../..
    echo "Building dbus-python done"
    touch build/dbus-python.stamp
else
    echo "dbus-python already built"
fi
