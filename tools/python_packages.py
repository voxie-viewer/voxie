import sys
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
import os
import json
import codecs


def list_packages(base, *, python_tag, abi_tag, platform_tags):
    with open(base + 'tools/python-packages.json', 'rb') as file:
        data = json.load(codecs.getreader('utf-8')(file))

    result = []
    # print(data)
    for package in data:
        name = package['Name']
        version = package['Version']
        if isinstance(version, dict):
            found = False
            for platform_tag in platform_tags:
                if platform_tag in version:
                    version = version[platform_tag]
                    found = True
                    break
            if not found:
                version = version['']
        sourceVersion = version
        if 'SourceVersion' in package:
            sourceVersion = package['SourceVersion']
        sourceExt = 'tar.gz'
        if 'SourceExtension' in package:
            sourceExt = package['SourceExtension']
        isPurePython = False
        if 'IsPurePython' in package:
            isPurePython = package['IsPurePython']
        repl = {
            'NAME': name,
            'SOURCE_NAME': name.replace('_', '-'),
            'VERSION': version,
            'SOURCE_VERSION': sourceVersion,
            'SOURCE_EXT': sourceExt,
        }
        sfn = '{SOURCE_NAME}-{VERSION}.{SOURCE_EXT}'
        if 'SourceFilename' in package:
            sfn = package['SourceFilename']
        sfn = sfn.format(**repl)
        if isPurePython:
            fn = None
            platform_tag = 'none'
            repl['PYTHON_TAG'] = python_tag
            repl['ABI_TAG'] = abi_tag
            repl['PLATFORM_TAG'] = platform_tag
        else:
            found = False
            for actual_python_tag in [python_tag, 'py3']:
                for actual_abi_tag in [abi_tag, 'none']:
                    for platform_tag in platform_tags + ['any']:
                        repl['PYTHON_TAG'] = actual_python_tag
                        repl['ABI_TAG'] = actual_abi_tag
                        repl['PLATFORM_TAG'] = platform_tag
                        # https://www.python.org/dev/peps/pep-0427/
                        fn = '{NAME}-{VERSION}-{PYTHON_TAG}-{ABI_TAG}-{PLATFORM_TAG}.whl'
                        if 'Filename' in package and platform_tag in package['Filename']:
                            fn = package['Filename'][platform_tag]
                        if fn is not None:
                            fn = fn.format(**repl)
                        # print(fn)
                        if fn is not None and not os.path.exists(base + 'tools/build-dep/' + fn + '.sha512sum'):
                            continue
                        found = True
                        break
                    if found:
                        break
                if found:
                    break
            if not found:
                print('Missing file ' + fn, file=sys.stderr)
                sys.exit(1)
            if fn is None:
                continue  # TODO: Add license information if fn is None?
        if not os.path.exists(base + 'tools/build-dep/' + sfn + '.sha512sum'):
            print('Missing source file ' + sfn, file=sys.stderr)
            sys.exit(1)
        with open(base + 'tools/build-dep/' + sfn + '.url', 'r') as file:
            origSrcUrl = file.read().strip()
        repl['ORIG_SRC_URL'] = origSrcUrl

        result.append({
            'Info': package,
            'Replacements': repl,
            'Filename': fn,
            'SourceFilename': sfn,
            'IsPurePython': isPurePython,
        })
    return result
