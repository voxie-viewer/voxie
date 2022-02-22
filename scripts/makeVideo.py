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
import numpy as np
import time
import scipy.misc
import subprocess
import argparse

import voxie

parser = voxie.parser

parser.add_argument('--rotation')
parser.add_argument('--minpos')
parser.add_argument('--maxpos')
parser.add_argument('--maxval')
parser.add_argument('--fps')
parser.add_argument('--slice-factor')
parser.add_argument('--output-file')

args = parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

sliceVis = instance.Gui.SelectedObjects[0]

# print (origin, orientation)

volumePath = sliceVis.GetProperty(
    'de.uni_stuttgart.Voxie.Visualizer.Slice.Volume').getValue('o')
volume = context.makeObject(context.bus, context.busName, volumePath, [
                            'de.uni_stuttgart.Voxie.DataObject'])
data = volume.Data.CastTo('de.uni_stuttgart.Voxie.VolumeDataVoxel')

# pixSize = 100e-6
pixSize = np.mean(data.GridSpacing)
# slicethick = pixSize / 10
# slicethick = pixSize * 10
slicethick = pixSize
if args.slice_factor is not None:
    slicethick = pixSize / float(args.slice_factor)

valMin = numpy.min(data)
valMax = numpy.max(data)

if args.maxval is not None:
    valMax = float(args.maxval)

if args.rotation is not None:
    orientation = np.fromstring(args.rotation, dtype=float, sep=',')
    if orientation.shape != (4,):
        raise Exception('Invalid --rotation value: ' + str(orientation))
else:
    origin = sliceVis.GetProperty(
        'de.uni_stuttgart.Voxie.Visualizer.Slice.Plane.Origin').getValue('(ddd)')
    orientation = sliceVis.GetProperty(
        'de.uni_stuttgart.Voxie.Visualizer.Slice.Plane.Orientation').getValue('(dddd)')
rotation = voxie.Rotation(orientation)

size = numpy.array(data.ArrayShape, dtype='uint64')
dorigin = numpy.array(data.VolumeOrigin, dtype=numpy.double)
dspacing = numpy.array(data.GridSpacing, dtype=numpy.double)
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

# print ('posmin[2] = %f, posmax[2] = %f' % (posmin[2], posmax[2]))
if args.minpos is not None:
    posmin[2] = float(args.minpos)
if args.maxpos is not None:
    posmax[2] = float(args.maxpos)
# print ('posmin[2] = %f, posmax[2] = %f' % (posmin[2], posmax[2]))

width = int((posmax[0] - posmin[0]) / pixSize + 1)
height = int((posmax[1] - posmin[1]) / pixSize + 1)
with instance.CreateImage((width, height), 1, ('float', 32, 'native')) as image, image.GetBufferReadonly() as buffer:
    oarray = buffer.array[:, :, 0]

    options = {}
    options['Interpolation'] = voxie.Variant('s', 'Linear')

    fps = 10
    if args.fps is not None:
        fps = float(args.fps)
    outputFile = args.output_file
    process = subprocess.Popen(['ffmpeg', '-v', 'quiet', '-f', 'rawvideo', '-vcodec', 'rawvideo', '-s', '%dx%d' % (width, height),
                                '-r', '%f' % fps, '-pix_fmt', 'rgb24', '-i', '-', '-codec', 'mjpeg', '-qscale', '1', '-y', outputFile], stdin=subprocess.PIPE)

    imgCount = int((posmax[2] - posmin[2]) / slicethick + 1)
    for i in range(imgCount):
        pos0 = (posmin[0], posmin[1], posmin[2] + slicethick * i)

        pos = rotation * pos0
        instance.Utilities.ExtractSlice(
            data, pos, orientation, (width, height), (pixSize, pixSize), image, options)

        array = oarray

        array = numpy.array(array)
        array[numpy.isnan(array)] = 0

        array = array[:, ::-1]
        array = array.transpose()

        array = array[:, :, numpy.newaxis] * \
            numpy.ones(3)[numpy.newaxis, numpy.newaxis, :]

        array = array[:, :, 0:3]
        array = array * (255.0 / valMax)
        array = np.fmin(255, np.fmax(0, array))
        array = np.array(array, dtype=np.uint8, order='C')
        process.stdin.write(array.data)
        print('Wrote image %d / %d' % (i, imgCount))
