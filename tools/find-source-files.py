#!/usr/bin/python3
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

import sys
import os
import pathlib


if len(sys.argv) != 3:
    print('Usage: find-source-files.py <proj> <extension>')
    sys.exit(1)
proj = sys.argv[1]
ext = sys.argv[2]

os.chdir(os.path.join(os.path.dirname(sys.argv[0]), '..'))

dirs = {}

pdir = pathlib.Path('src/' + proj)
for path in pdir.rglob('*.' + ext):
    parts = path.parts[len(pdir.parts):]
    name = parts[-1]
    if '.List.' in name:
        continue
    dir = '/'.join(parts[:-1])
    if dir not in dirs:
        dirs[dir] = []
    dirs[dir].append(name)

for dir in sorted(dirs):
    # print(dir)
    for name in sorted(dirs[dir]):
        fn = dir + ('' if dir == '' else '/') + name
        print("    '%s'," % (fn,))
    print()
