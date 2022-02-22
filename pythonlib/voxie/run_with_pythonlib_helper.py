import os
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
import importlib.util

# This is a helper which will launch the first argument as a python script while keeping the current sys.path


def main():
    if len(sys.argv) < 2:
        print(
            'Usage: python3 -m voxie.run_with_pythonlib_helper filename.py [...]', file=sys.stderr)
        sys.exit(1)

    # while len(sys.argv) > 1 and sys.argv[1].startswith('--additional-python-path='):
    #     sys.path.insert(0, sys.argv.pop(1)[len('--additional-python-path='):])

    # if len(sys.argv) > 1 and sys.argv[1] == '--':
    #     sys.argv.pop(1)

    sys.argv.pop(0)
    fn = sys.argv[0]

    sys.path.insert(0, os.path.dirname(fn))

    # https://docs.python.org/3/library/importlib.html#importing-a-source-file-directly
    spec = importlib.util.spec_from_file_location('__main__', fn)
    if spec is None:
        raise Exception('Unable to load python file ' + repr(fn))
    module = importlib.util.module_from_spec(spec)
    sys.modules['__main__'] = module
    spec.loader.exec_module(module)


if __name__ == '__main__':
    main()
