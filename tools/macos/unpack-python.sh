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
#set -x

if [ "$#" != 1 ]; then
    echo "Usage: $0 <python-version>"
    exit 1
fi

PYTHON_VERSION="$1"
shift

VERS="${PYTHON_VERSION#*-}"
VERS="${VERS%%-*}"
VERS_MAJ="${VERS%%.*}"
VERS="${VERS#$VERS_MAJ.}"
VERS_MIN="${VERS%%.*}"
VERS="$VERS_MAJ.$VERS_MIN"

if [ ! -f "build/python.stamp" ]; then
    echo "Unpacking python (version '$VERS' from '$PYTHON_VERSION')..."
    rm -rf build/python.stamp build/python build/python-install

    tools/download_dep.py "$PYTHON_VERSION.pkg"

    mkdir build/python
    pkgutil --expand "tools/build-dep/$PYTHON_VERSION.pkg" build/python/pkg
    mkdir build/python-install
    mkdir build/python-install/Python.framework
    cd build/python-install/Python.framework
    tar xzf "../../python/pkg/Python_Framework.pkg/Payload"
    cd ..
    echo "Making python relocatable..."
    install_name_tool -id "@rpath/Python.framework/Versions/$VERS/Python" "Python.framework/Versions/$VERS/Python"
    chmod u+w "Python.framework/Versions/$VERS/lib"/*.dylib # Make sure libtk8.6.dylib is writable
    for file in "Python.framework/Versions/$VERS/bin/python$VERS" "Python.framework/Versions/$VERS/Resources/Python.app/Contents/MacOS/Python" "Python.framework/Versions/$VERS/lib"/*.dylib "Python.framework/Versions/$VERS/lib/python$VERS/lib-dynload"/*.so; do
        cp "$file" "$file.orig"
        install_name_tool -change "/Library/Frameworks/Python.framework/Versions/$VERS/Python" "@rpath/Python.framework/Versions/$VERS/Python" "$file"
        for lib in "Python.framework/Versions/$VERS/lib"/*.dylib; do
            install_name_tool -change "/Library/Frameworks/$lib" "@rpath/$lib" "$file"
        done
    done
    install_name_tool -add_rpath "@loader_path/../../../.." "Python.framework/Versions/$VERS/bin/python$VERS"
    install_name_tool -add_rpath "@loader_path/../../../../../../.." "Python.framework/Versions/$VERS/Resources/Python.app/Contents/MacOS/Python"
    cd ../..
    echo "Unpacking python done"
    touch build/python.stamp
else
    echo "python already unpacked"
fi
