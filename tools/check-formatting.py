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
import subprocess

os.chdir(os.path.join(os.path.dirname(__file__), '..'))

haveError = False

res = subprocess.run(['git', 'ls-tree', '-r', 'HEAD', '-z',
                      '--name-only'], stdout=subprocess.PIPE, check=True)
allFiles = [str(s, 'utf-8') for s in res.stdout.split(b'\0') if s != b'']

textExtensions = [
    '.py',
    '.sh',
    '.ps1',
    '.mako',

    '.json',
    '.xml',
    '.yml',

    '.txt',
    '.md',
    '.html',
    '.tex',

    '.commit',
    '.url',
    '.sha512sum',

    '.c',
    '.h',
    '.cpp',
    '.hpp',
    '.cl',
    '.glsl',
    '.js',
    '.qml',

    '.ui',
    '.qrc',
    '.pro',
    '.pri',
    '.tmpl',
    '.cfg',
    '.plist',
]
textFilenames = [
    'README',
    '.clang-format',
    '.gitignore',
    'meson.build',
    'meson_options.txt',
    'meson-ext-template.build',
    'meson-plugin-template.build',
]
textIgnoreDirs = [
    'lib',
    'src/PluginHDF5/lib',
    'intern/old',
]
textIgnoreFiles = [
    'src/ExtFilePly/tinyply.h',
    'src/ExtFilterMarchingCubes33_by_DVega/MC33_LookUpTable.h',
    'src/ExtFilterMarchingCubes33_by_DVega/marchingcubes33.cpp',
    'src/VoxieBackend/lib/CL/cl-patched.hpp',
]

for file in allFiles:
    ignore = True
    for ext in textExtensions:
        if file.endswith(ext):
            ignore = False
            break
    for name in textFilenames:
        if file == name or file.endswith('/' + name):
            ignore = False
            break
    for ignoreVal in textIgnoreDirs:
        if file.startswith(ignoreVal + '/'):
            ignore = True
    if file in textIgnoreFiles:
        ignore = True
    if ignore:
        continue
    # print(file)
    with open(file, 'rb') as f:
        data = f.read()

    if len(data) > 0 and data[-1:] != b'\n':
        print('Text file %s does not end in a newline (<LF>)' % (file,))
        haveError = True

    if b'\r' in data:
        print('Text file %s contains <CR> (windows line ending) at byte %d' % (file, data.find(b'\r')))
        haveError = True

    if b'\t' in data:
        print('Text file %s contains Tab character at byte %d' % (file, data.find(b'\t')))
        haveError = True

    # TODO: start checking for this
    # if b' \n' in data:
    #     print('Text file %s contains trailing spaces at byte %d' % (file, data.find(b' \n')))
    #     haveError = True

    try:
        data.decode('utf-8', errors='strict')
    except Exception as e:
        print('Text file %s is not UTF-8: %s' % (file, e))
        haveError = True

dirs = [
    {'dir': 'ext'},
    {'dir': 'filters', 'recursive': False, 'exclude': {'tomopy_misc_phantom.py'}},
    {'dir': 'filters/digitalvolumecorrelation'},
    {'dir': 'pythonlib'},
    {'dir': 'scripts'},
    {'dir': 'tools'},
]
ignore = [
    'W503',  # line break before binary operator
    'W504',  # line break after binary operator
    'E501',  # line too long
]

for d in dirs:
    if 'recursive' not in d:
        d['recursive'] = True
recDir = [d['dir'] for d in dirs if d['recursive']]
nrecDir = [d['dir'] for d in dirs if not d['recursive']]

exclude = set()
for d in dirs:
    if 'exclude' in d:
        exclude = set.union(exclude, {d['dir'] + '/' + di for di in d['exclude']})

res = subprocess.run(['git', 'ls-tree', '-r', 'HEAD', '-z',
                      '--name-only'] + recDir, stdout=subprocess.PIPE, check=True)
files = [str(s, 'utf-8') for s in res.stdout.split(b'\0') if s != b'']
res = subprocess.run(['git', 'ls-tree', 'HEAD', '-z',
                      '--name-only'] + [d + '/' for d in nrecDir], stdout=subprocess.PIPE, check=True)
files += [str(s, 'utf-8') for s in res.stdout.split(b'\0') if s != b'']

files = [f for f in files if f not in exclude]
# print(files)

pyFiles = []
for file in files:
    if not file.endswith('.py'):
        continue
    # print(file)
    pyFiles.append(file)
    # res = subprocess.run(['python3', '-m', 'pycodestyle', file])
    # if res.returncode != 0:
    #     haveError = True
res = subprocess.run(['python3', '-m', 'pycodestyle',
                      '--ignore=' + ','.join(ignore)] + pyFiles)
if res.returncode != 0:
    haveError = True

if haveError:
    print('Got at least one failure while checking formatting')
    sys.exit(1)
