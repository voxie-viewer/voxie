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
import tarfile
import zipfile

import python_packages
import download_dep

tags = [
    {
        'python_tag': 'cp37',
        'abi_tag': 'cp37m',
        'platform_tags': ['win_amd64'],
    },
    {
        'python_tag': 'cp39',
        'abi_tag': 'cp39',
        'platform_tags': ['macosx_10_9_x86_64', 'macosx_10_10_x86_64'],
    },
]


def main():
    os.chdir(os.path.join(os.path.dirname(__file__), '..'))

    for tag in tags:
        for pkg in python_packages.list_packages(base='', **tag):
            package = pkg['Info']
            repl = pkg['Replacements']
            fn = pkg['Filename']
            sfn = pkg['SourceFilename']
            isPurePython = pkg['IsPurePython']

            if fn is not None:
                download_dep.download(fn)
            download_dep.download(sfn)

            if 'LicenseFilenameSource' in package:
                license_file = package['LicenseFilenameSource'].format(**repl)
                with tarfile.open('tools/build-dep/' + sfn, 'r') as zip:
                    with zip.extractfile(license_file) as file:
                        license = file.read()
            else:
                license_file = package['LicenseFilename'].format(**repl)
                with zipfile.ZipFile('tools/build-dep/' + fn, 'r') as zip:
                    with zip.open('{NAME}-{VERSION}.dist-info/'.format(**repl) + license_file, 'r') as file:
                        license = file.read()
            # print(type(license), len(license))


if __name__ == '__main__':
    main()
