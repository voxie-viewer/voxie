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
import hashlib
import urllib.request
import argparse
import subprocess
import shutil
import time
import json
import codecs
import io

DEP_SOURCE_URL = 'https://ipvs.informatik.uni-stuttgart.de/pas/src'
DEP_SOURCE_URL_PUB = DEP_SOURCE_URL


def sha512(filename):
    hash = hashlib.sha512()
    with open(filename, 'rb') as file:
        # Read and update hash string value in blocks of 4K
        for byte_block in iter(lambda: file.read(4096), b''):
            hash.update(byte_block)
    return hash.hexdigest().lower()


def download_file(url, filename, *, show_progress=True):
    if not show_progress:
        urllib.request.urlretrieve(url, filename)
        return

    # https://stackoverflow.com/questions/22676/how-do-i-download-a-file-over-http-using-python/22776#22776
    # https://stackoverflow.com/questions/22676/how-to-download-a-file-over-http/16518224#16518224

    last_progress = [None]

    def show_progress(done, overall, *, min_delay):
        now = time.monotonic()
        if min_delay is not None and last_progress[0] is not None:
            if last_progress[0] + min_delay > now:
                return

        status = "{0:16} bytes".format(file_size_dl)
        if file_size:
            col = 70  # TODO: Use COLUMNS environment variable?
            done = round(col * file_size_dl / file_size)
            if done > col:
                done = col
            rem = col - done
            s = '#' * done + '.' * rem
            status = '{1} [{0:6.2f}%]'.format(file_size_dl * 100 / file_size, s)
        status += chr(13)
        print(status, end='')

        last_progress[0] = now

    u = urllib.request.urlopen(url)
    with open(filename, 'wb') as f:
        meta = u.info()
        meta_length = meta.get_all("Content-Length")
        file_size = None
        if meta_length:
            file_size = int(meta_length[0])
        # print("Downloading: %s Bytes: %s" % (filename, file_size))

        file_size_dl = 0
        show_progress(file_size_dl, file_size, min_delay=None)
        block_sz = 8192
        while True:
            buffer = u.read(block_sz)
            if not buffer:
                break

            file_size_dl += len(buffer)
            f.write(buffer)

            show_progress(file_size_dl, file_size, min_delay=0.2)
        show_progress(file_size_dl, file_size_dl, min_delay=None)


def download(filename, *, base=None):
    # print(filename)

    url = DEP_SOURCE_URL + '/' + filename

    if base is None:
        base = os.path.join(os.path.dirname(__file__), '..') + '/'

    sha512RefFilename = base + 'tools/build-dep/' + filename + '.sha512sum'
    resultFilename = base + 'tools/build-dep/' + filename
    for f in [resultFilename + '.new', sha512RefFilename + '.new', sha512RefFilename + '.new.bak']:
        try:
            os.unlink(f)
        except FileNotFoundError:
            pass

    with open(sha512RefFilename, 'r') as file:
        sha512Ref = file.read().strip().split(' ')[0].strip().lower()
    if len(sha512Ref) != 128:
        print('File %s contains invalid sha512sum: %s' % (repr(sha512RefFilename), repr(sha512Ref)))
        sys.exit(1)

    sha512Act = 'missing'
    if os.path.exists(resultFilename):
        sha512Act = sha512(resultFilename)
    if sha512Ref != sha512Act:
        # print(sha512Ref + ' != ' + sha512Act)
        # print('Downloading %s...' % (filename,), end='', flush=True)
        print('Downloading %s...' % (filename,), flush=True)

        download_file(url, resultFilename + '.new', show_progress=True)

        print(flush=True)

        sha512Act = sha512(resultFilename + '.new')
        if sha512Ref != sha512Act:
            print('sha512sum mismatch for downloaded file %s: Should be %s, is %s' % (repr(filename), repr(sha512Ref), repr(sha512Act)))
            sys.exit(1)
        os.rename(resultFilename + '.new', resultFilename)


def store_license(filename, outputDirectory, *, base=None, unpack_program_7z=None):
    # print(filename)

    if base is None:
        base = os.path.join(os.path.dirname(__file__), '..') + '/'

    swFilename = base + 'tools/build-dep/' + filename + '.sw.json'

    with open(swFilename, 'rb') as file:
        swData = json.load(codecs.getreader('utf-8')(file))

    licenseFileBase = swData['Name'] + '.txt'
    licenseFile = outputDirectory + '/' + licenseFileBase
    listFilename = outputDirectory + '/list.jsonl'

    if 'SourceFilename' in swData:
        sfilename = swData['SourceFilename']
        sourceCodeUri = DEP_SOURCE_URL_PUB + '/' + sfilename
    else:
        sourceCodeUri = DEP_SOURCE_URL_PUB + '/' + filename

    if 'LicenseFilename' in swData:
        lfn = swData['LicenseFilename']
        unpackedFn = base + 'tools/build-dep/' + filename + '.unpack/' + lfn
        if os.path.exists(unpackedFn):
            # File is unpacked in repository
            shutil.copy(unpackedFn, licenseFile + '.new')
        else:
            download(filename, base=base)  # Needed when --license-only is set
            dir = unpack(filename, base=base, unpack_program_7z=unpack_program_7z)
            shutil.copy(dir + '/' + lfn, licenseFile + '.new')
    elif 'SourceFilename' in swData:
        sfilename = swData['SourceFilename']
        sswFilename = base + 'tools/build-dep/' + sfilename + '.sw.json'
        with open(sswFilename, 'rb') as file:
            sswData = json.load(codecs.getreader('utf-8')(file))
        if 'LicenseFilename' not in sswData:
            raise Exception('Could not find license information in source file ' + repr(sswFilename) + ' for ' + repr(swFilename))
        lfn = sswData['LicenseFilename']
        unpackedFn = base + 'tools/build-dep/' + sfilename + '.unpack/' + lfn
        if os.path.exists(unpackedFn):
            # File is unpacked in repository
            shutil.copy(unpackedFn, licenseFile + '.new')
        else:
            download(sfilename, base=base)
            dir = unpack(sfilename, base=base, unpack_program_7z=unpack_program_7z)
            shutil.copy(dir + '/' + lfn, licenseFile + '.new')
    else:
        raise Exception('Could not find license information in ' + repr(swFilename))
    os.rename(licenseFile + '.new', licenseFile)

    entry = {
        'Name': swData['Name'],
        'LicenseFilename': licenseFileBase,
        'SourceCodeURI': sourceCodeUri,
    }
    f = io.StringIO()
    json.dump(entry, f, allow_nan=False, sort_keys=True, ensure_ascii=False)
    s = bytes(f.getvalue(), 'utf-8')
    if s.find(b'\n') != -1 or s.find(b'\r') != -1:
        raise Exception("s.find(b'\n') != -1 or s.find(b'\r') != -1")
    s = s + b'\n'
    with open(listFilename, 'ab') as file:
        file.write(s)


def unpack(filename, base=None, *, unpack_program_7z=None):
    # print(filename)

    if base is None:
        base = os.path.join(os.path.dirname(__file__), '..') + '/'

    sha512RefFilename = base + 'tools/build-dep/' + filename + '.sha512sum'
    archiveFilename = base + 'tools/build-dep/' + filename
    outputParentDir = base + 'tools/unpack-build-dep'
    outputDir = outputParentDir + '/' + filename
    outputStamp = outputDir + '.stamp'

    with open(sha512RefFilename, 'r') as file:
        sha512Ref = file.read().strip().split(' ')[0].strip().lower()

    if os.path.exists(outputStamp):
        with open(outputStamp, 'r') as file:
            sha512Stamp = file.read().strip().split(' ')[0].strip().lower()
        if sha512Ref == sha512Stamp:
            return outputDir

    print('Unpacking {0}...'.format(filename), end='', flush=True)

    if os.path.exists(outputStamp):
        os.remove(outputStamp)
    if os.path.exists(outputStamp + '.new'):
        os.remove(outputStamp + '.new')
    if os.path.exists(outputDir):
        shutil.rmtree(outputDir)

    if not os.path.exists(outputParentDir):
        os.mkdir(outputParentDir)
    os.mkdir(outputDir)
    # TODO: Do unpacking in python?
    if unpack_program_7z is not None:
        # Use stdout=subprocess.PIPE to hide stdout
        subprocess.run([unpack_program_7z, '-aoa', '-o' + outputDir, 'x', archiveFilename], check=True, stdout=subprocess.PIPE)
        if filename.endswith('.tar.xz') or filename.endswith('.tar.bz2') or filename.endswith('.tar.gz') or filename.endswith('.tgz'):
            if filename.endswith('.tgz'):
                tarfile = outputDir + '/' + os.path.basename(filename[:-4] + '.tar')
            else:
                ext = filename.rfind('.')
                tarfile = outputDir + '/' + os.path.basename(filename[:ext])
            subprocess.run([unpack_program_7z, '-aoa', '-o' + outputDir, 'x', tarfile], check=True, stdout=subprocess.PIPE)
    elif filename.endswith('.zip') or filename.endswith('.whl'):
        # shutil.unpack_archive() does not set executable bits. TODO: fix this, use shutil.unpack_archive() everywhere
        if os.name == 'posix':
            subprocess.run(['unzip', '-q', '../../build-dep/' + filename], check=True, cwd=outputDir)
        else:
            shutil.unpack_archive(archiveFilename, outputDir, 'zip')
    else:
        subprocess.run(['tar', 'xf', archiveFilename, '-C', outputDir], check=True)

    shutil.copy(sha512RefFilename, outputStamp + '.new')
    os.rename(outputStamp + '.new', outputStamp)

    print(flush=True)

    return outputDir


def main():
    os.chdir(os.path.join(os.path.dirname(__file__), '..'))

    parser = argparse.ArgumentParser()
    parser.add_argument('filename', type=str, nargs='+')
    parser.add_argument('--license-output-dir', type=str)
    parser.add_argument('--unpack-program-7z', type=str)
    parser.add_argument('--unpack', action='store_true')
    parser.add_argument('--license-only', action='store_true')
    args = parser.parse_args()

    for filename in args.filename:
        if not args.license_only:
            download(filename, base='')
        if args.license_output_dir is not None:
            store_license(filename, args.license_output_dir, base='', unpack_program_7z=args.unpack_program_7z)
        if args.unpack:
            unpack(filename, base='', unpack_program_7z=args.unpack_program_7z)


if __name__ == '__main__':
    main()
