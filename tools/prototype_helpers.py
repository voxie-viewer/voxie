import glob
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
import sys


def add_arguments(parser):
    parser.add_argument('--glob-pattern', help='glob pattern for finding prototype JSON files', type=str)


def find_json_files(args):
    if args.glob_pattern is None:
        os.chdir(os.path.join(os.path.dirname(sys.argv[0]), '..'))

        files = [
            glob.glob('src/Voxie/Data/Prototypes/*.json'),
            glob.glob('src/Voxie/PropertyObjects/Prototypes/*.json'),
            glob.glob('src/Voxie/Node/Prototypes/*.json'),
            glob.glob('src/PluginFilter/Prototypes/*.json'),
            glob.glob('src/PluginVisSlice/Prototypes/*.json'),
            glob.glob('src/PluginVisRaw/Prototypes/*.json'),
            glob.glob('src/PluginVisTable/Prototypes/*.json'),
            glob.glob('src/PluginVis3D/Prototypes/*.json'),
            glob.glob('src/PluginFitting/Prototypes/*.json'),
            glob.glob('src/PluginExample/Prototypes/*.json'),
            glob.glob('src/Ext*/*.json'),
            glob.glob('ext/*.json'),
            glob.glob('filters/*.json'),
        ]
        files = [item for subl in files for item in subl]
    else:
        files = glob.glob(args.glob_pattern)
    return files
