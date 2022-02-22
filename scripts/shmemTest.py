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

import numpy
import voxie

# TODO: Update to new DBus interface

args = voxie.parser.parse_args()

instance = voxie.Voxie(args)

slice = instance.getSlice()

dataSet = slice.getVolumeObject()

data = dataSet.getVolumeDataVoxel()

# array = data.getDataReadonly().array
with data.CreateUpdate() as update:
    array = data.getDataWritable().array
    print(numpy.mean(array))
    print(numpy.mean(array[43:86, 43:86, 43:86]))
    # array[43:86,43:86,43:86] = 0
    array[43:86, 43:86, 43:86] = 1 - array[43:86, 43:86, 43:86]
    print(numpy.mean(array[43:86, 43:86, 43:86]))

    update.Finish()
