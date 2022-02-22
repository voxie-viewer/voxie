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
import numpy
import time
import scipy.misc

import voxie

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

sliceVis = instance.Gui.SelectedObjects[0]
origin = sliceVis.GetProperty(
    'de.uni_stuttgart.Voxie.Visualizer.Slice.Plane.Origin').getValue('(ddd)')
orientation = voxie.Rotation(sliceVis.GetProperty(
    'de.uni_stuttgart.Voxie.Visualizer.Slice.Plane.Orientation').getValue('(dddd)'))

# print (origin, orientation)

volumePath = sliceVis.GetProperty(
    'de.uni_stuttgart.Voxie.Visualizer.Slice.Volume').getValue('o')
volume = context.makeObject(context.bus, context.busName, volumePath, [
                            'de.uni_stuttgart.Voxie.DataObject'])
data = volume.Data.CastTo('de.uni_stuttgart.Voxie.VolumeDataVoxel')

pixSize = 100e-6
slicethick = pixSize / 2
# slicethick = pixSize * 10

size = numpy.array(data.ArrayShape, dtype='uint64')
dorigin = numpy.array(data.VolumeOrigin, dtype=numpy.double)
dspacing = numpy.array(data.GridSpacing, dtype=numpy.double)
posmin = posmax = None
for corner in [[0, 0, 0], [0, 0, 1], [0, 1, 0], [0, 1, 1], [1, 0, 0], [1, 0, 1], [1, 1, 0], [1, 1, 1]]:
    cpos = numpy.array(corner, dtype=numpy.double) * size
    cpos = orientation.inverse * (dorigin + dspacing * cpos)
    if posmin is None:
        posmin = cpos
    if posmax is None:
        posmax = cpos
    posmin = numpy.minimum(posmin, cpos)
    posmax = numpy.maximum(posmax, cpos)
    # print (cpos)
# print (posmin)
# print (posmax)

posmin[2] = 0
posmax[2] = 0.004
# posmin[0] = posmin[1] = 0
# posmax[0] = posmax[1] = 0.01

width = int((posmax[0] - posmin[0]) / pixSize + 1)
height = int((posmax[1] - posmin[1]) / pixSize + 1)
with instance.CreateImage((width, height), 1, ('float', 32, 'native')) as image, image.GetBufferReadonly() as buffer:
    oarray = buffer.array[:, :, 0]

    avg = []

    options = {}
    options['Interpolation'] = voxie.Variant('s', 'NearestNeighbor')
    options['Interpolation'] = voxie.Variant('s', 'Linear')

    # pos0 = (posmin[0], posmin[1], (orientation.inverse * origin)[2])
    print(int((posmax[2] - posmin[2]) / slicethick + 1))
    # for i in range (int ((posmax[2] - posmin[2]) / slicethick + 1)):
    for i in [int((posmax[2] - posmin[2]) / slicethick + 1) // 2]:
        print(i)
        pos0 = (posmin[0], posmin[1], posmin[2] + slicethick * i)

        # print (orientation)
        # print (orientation.inverse)
        # print (pos0)
        pos = orientation * pos0
        instance.Utilities.ExtractSlice(
            data, pos, orientation.quaternion.value, (width, height), (pixSize, pixSize), image, options)

        array = oarray

        array = numpy.array(array)
        array[numpy.isnan(array)] = 0

        cutoff = 40
        array[array > cutoff] = cutoff
        avg.append(numpy.mean(array))

        # print (i)
        # continue

        # array = array[::-1, ::-1]
        array = array.transpose()

        # print (array)
        # print (array.shape)
        # print (array.strides)
        array = array[:, :, numpy.newaxis] * \
            numpy.ones(4)[numpy.newaxis, numpy.newaxis, :]
        array[:, :, 3] = (1 - numpy.isnan(oarray).transpose()) * 40
        fn = '/tmp/out-%04d.png' % i
        scipy.misc.toimage(array, cmin=0.0, cmax=40.0).save(fn)
        # print (array[1,0:100])

os.system('mdbus2 ' + context.busName + ' | grep Image')

print(avg)

os.system('mdbus2 ' + context.busName + ' | grep Client')

os.system('ls -l /proc/$PPID/fd/')
