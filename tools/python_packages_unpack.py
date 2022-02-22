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
import argparse
import shutil
import tarfile
import zipfile
import io
import json

import python_packages
import download_dep

downloadSource = False


def unpack(base, output_dir, *, python_tag, abi_tag, platform_tags, license_output_dir=None, license_only=False):
    copyingTempl = ''
    for pkg in python_packages.list_packages(base=base, python_tag=python_tag, abi_tag=abi_tag, platform_tags=platform_tags):
        package = pkg['Info']
        repl = pkg['Replacements']
        fn = pkg['Filename']
        sfn = pkg['SourceFilename']
        isPurePython = pkg['IsPurePython']

        if fn is not None:
            download_dep.download(fn, base=base)
        if downloadSource or (license_output_dir is not None and 'LicenseFilenameSource' in package) or isPurePython:
            download_dep.download(sfn, base=base)
        if not license_only:
            if isPurePython:
                print('Unpacking %s...' % (sfn,), end='', flush=True)
                # shutil.unpack_archive(base + 'tools/build-dep/' + sfn, output_dir)
                with tarfile.open(base + 'tools/build-dep/' + sfn, 'r') as zip:
                    with zip.extractfile('{SOURCE_NAME}-{VERSION}/{NAME}.egg-info/top_level.txt'.format(**repl)) as file:
                        lines = [line.decode('utf-8').strip() for line in file.readlines()]
                    members = []
                    prefix = '{SOURCE_NAME}-{VERSION}/'.format(**repl)
                    for mb in zip.getmembers():
                        for line in lines:
                            if mb.name.startswith(prefix + line):
                                mb.name = mb.name[len(prefix):]
                                members.append(mb)
                                break
                    zip.extractall(path=output_dir, members=members)
                print(flush=True)
            elif fn is not None:
                print('Unpacking %s...' % (fn,), end='', flush=True)
                with zipfile.ZipFile(base + 'tools/build-dep/' + fn, 'r') as zip_ref:
                    zip_ref.extractall(output_dir)
                print(flush=True)
                # print(license_output_dir)
        if license_output_dir is not None:
            if 'LicenseFilenameSource' in package:
                license_file = package['LicenseFilenameSource'].format(**repl)
                with tarfile.open(base + 'tools/build-dep/' + sfn, 'r') as zip:
                    with zip.extractfile(license_file) as file:
                        license = file.read()
            else:
                license_file = package['LicenseFilename'].format(**repl)
                # with open(output_dir + '/{NAME}-{VERSION}.dist-info/'.format(**repl) + license_file, 'rb') as file:
                #     license = file.read()

                # TODO: Read without extracting?
                dir = download_dep.unpack(fn)
                with open(dir + '/{NAME}-{VERSION}.dist-info/'.format(**repl) + license_file, 'rb') as file:
                    license = file.read()
            # print(len(license), b'\r' in license)
            license = license.replace(b'\r', b'')
            # print(len(license), b'\r' in license)
            if not os.path.exists(license_output_dir):
                os.mkdir(license_output_dir)
            licenseFileBase = 'python-{NAME}.txt'.format(**repl)
            with open(license_output_dir + '/' + licenseFileBase, 'wb') as file:
                file.write(license)
            copyingTempl += 'The copyright information for the python library \'{NAME}\' is in python-{NAME}.txt, you can download the source code from {ORIG_SRC_URL}\n'.format(**repl)

            entry = {
                'Name': 'python-' + repl['NAME'],
                'Description': 'Python package ' + repl['NAME'],
                'LicenseFilename': licenseFileBase,
            }
            listFilename = license_output_dir + '/list.jsonl'
            f = io.StringIO()
            json.dump(entry, f, allow_nan=False, sort_keys=True, ensure_ascii=False)
            s = bytes(f.getvalue(), 'utf-8')
            if s.find(b'\n') != -1 or s.find(b'\r') != -1:
                raise Exception("s.find(b'\n') != -1 or s.find(b'\r') != -1")
            s = s + b'\n'
            with open(listFilename, 'ab') as file:
                file.write(s)
    if license_output_dir is not None:
        # TODO: Remove python-packages.tmpl
        with open(license_output_dir + '/python-packages.tmpl', 'w') as file:
            file.write(copyingTempl)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--python-tag', type=str, required=True)
    parser.add_argument('--abi-tag', type=str, required=True)
    parser.add_argument('--platform-tag', type=str, required=True, action='append')
    parser.add_argument('output_directory', type=str)
    parser.add_argument('--license-output-dir', type=str)
    parser.add_argument('--license-only', action='store_true')
    args = parser.parse_args()

    os.chdir(os.path.join(os.path.dirname(__file__), '..'))

    unpack('', args.output_directory, python_tag=args.python_tag, abi_tag=args.abi_tag, platform_tags=args.platform_tag, license_output_dir=args.license_output_dir, license_only=args.license_only)


if __name__ == '__main__':
    main()
