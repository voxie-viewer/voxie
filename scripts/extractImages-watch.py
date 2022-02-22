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

instance = voxie.Voxie(args)

slice = instance.getSlice(args)

# dataSet = slice.getVolumeObject()
dataSet = instance.getVolumeObject()

# print (dataSet.get('DisplayName'))

data = dataSet.getVolumeDataVoxel()

with instance.createClient() as client:
    # pixSize = 100e-6
    pixSize = 30e-6
    slicethick = pixSize / 2
    # slicethick = pixSize * 10

    # plane = slice.dbus_properties.Get('de.uni_stuttgart.Voxie.Slice', 'Plane')
    plane = ([0.0038008843548595905, 0.019205857068300247, 0.01320459134876728], [
             0.790107250213623, -0.6124427914619446, 0.016235243529081345, 0.019515523687005043])
    # print([float(x) for x in plane[0]], [float(x) for x in plane[1]])
    origin = numpy.array(plane[0])
    orientation = plane[1]
    rotation = voxie.Rotation(orientation)

    size = numpy.array(data.get('Size'), dtype='uint64')
    dorigin = numpy.array(data.get('Origin'), dtype=numpy.double)
    dspacing = numpy.array(data.get('Spacing'), dtype=numpy.double)
    posmin = posmax = None
    for corner in [[0, 0, 0], [0, 0, 1], [0, 1, 0], [0, 1, 1], [1, 0, 0], [1, 0, 1], [1, 1, 0], [1, 1, 1]]:
        cpos = numpy.array(corner, dtype=numpy.double) * size
        cpos = rotation.inverse * (dorigin + dspacing * cpos)
        if posmin is None:
            posmin = cpos
        if posmax is None:
            posmax = cpos
        posmin = numpy.minimum(posmin, cpos)
        posmax = numpy.maximum(posmax, cpos)
        # print (cpos)
    # print (posmin)
    # print (posmax)

#    posmin[2] = 0
#    posmax[2] = 0.004
#    posmin[0] = posmin[1] = 0
    posmin[0] = -0.017
    posmin[1] = -0.027
    posmax[0] = 0.025
    posmax[1] = 0.012
    posmin[2] = posmax[2] = 0.0218979908616

    width = int((posmax[0] - posmin[0]) / pixSize + 1)
    height = int((posmax[1] - posmin[1]) / pixSize + 1)
    with instance.createImage(client, (width, height)) as image, image.getDataReadonly() as buffer:
        oarray = buffer.array

        avg = []

        options = {}
        options['Interpolation'] = 'NearestNeighbor'
        options['Interpolation'] = 'Linear'

        # pos0 = (posmin[0], posmin[1], (rotation.inverse * origin)[2])
        print(int((posmax[2] - posmin[2]) / slicethick + 1))
        # for i in range (int ((posmax[2] - posmin[2]) / slicethick + 1)):
        # for i in [int ((posmax[2] - posmin[2]) / slicethick + 1)//2]:
        i = 0
        for zPos in [0.0218979908616, 0.023216686311, 0.0234892180946]:
            print(i)
            # pos0 = (posmin[0], posmin[1], posmin[2] + slicethick * i)
            pos0 = (posmin[0], posmin[1], zPos)

            # print (rotation)
            # print (rotation.inverse)
            # print (pos0)
            pos = rotation * pos0
            instance.Utilities.ExtractSlice(
                data, pos, orientation, (width, height), (pixSize, pixSize), image.path, options)  # TODO

            array = oarray

            array = numpy.array(array)
            array[numpy.isnan(array)] = 0

            avg.append(numpy.mean(array))
            # maxVal = 1000
            maxVal = 5 * numpy.mean(array)
            cutoff = maxVal
            array[array > cutoff] = cutoff

            # print (i)
            # continue

            # array = array[::-1, ::-1]
            array = array.transpose()

            # print (array)
            # print (array.shape)
            # print (array.strides)
            array = array[:, :, numpy.newaxis] * \
                numpy.ones(4)[numpy.newaxis, numpy.newaxis, :]
            array[:, :, 3] = (1 - numpy.isnan(oarray).transpose()) * maxVal
            fn = '/tmp/out-%04d.png' % i
            scipy.misc.toimage(array, cmin=0.0, cmax=maxVal).save(fn)
            # print (array[1,0:100])
            i = i + 1

    os.system('mdbus2 ' + instance.bus_name + ' | grep Image')

print(avg)

os.system('mdbus2 ' + instance.bus_name + ' | grep Client')

os.system('ls -l /proc/$PPID/fd/')
