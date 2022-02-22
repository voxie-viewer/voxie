#!/usr/bin/python
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
import dbus
import mmap
import numpy
import tempfile
import os

import zbar
import Image
import ImageDraw

import voxie

args = voxie.parser.parse_args()

instance = voxie.Voxie(args)

slice = instance.getSlice(args)

dataSet = slice.getVolumeObject()

data = dataSet.getVolumeDataVoxel()

with instance.createClient() as client:
    plane = slice.dbus_properties.Get('de.uni_stuttgart.Voxie.Slice', 'Plane')
    origin = numpy.array(plane[0])
    orientation = plane[1]
    rotation = voxie.Rotation(orientation)

    size = numpy.array(data.get('Size'), dtype='uint64')
    dorigin = numpy.array(data.get('Origin'), dtype=numpy.double)
    dspacing = numpy.array(data.get('Spacing'), dtype=numpy.double)
    posmin = posmax = None
    for corner in [[0, 0, 0], [0, 0, 1], [0, 1, 0], [0, 1, 1], [1, 0, 0], [1, 0, 1], [1, 1, 0], [1, 1, 1]]:
        cpos = numpy.array(corner, dtype=numpy.double) * size
        cpos = rotation * (dorigin + dspacing * cpos)
        if posmin is None:
            posmin = cpos
        if posmax is None:
            posmax = cpos
        posmin = numpy.minimum(posmin, cpos)
        posmax = numpy.maximum(posmax, cpos)
        # print (cpos)
    # print (posmin)
    # print (posmax)

    pixSize = numpy.min(dspacing)

    pos0 = (posmin[0], posmin[1], (rotation.inverse * origin)[2])

    pos = rotation * pos0

    width = int((posmax[0] - posmin[0]) / pixSize + 1)
    height = int((posmax[1] - posmin[1]) / pixSize + 1)

    options = {}
    options['Interpolation'] = 'NearestNeighbor'
    options['Interpolation'] = 'Linear'

    with instance.createImage(client, (width, height)) as image, image.getDataReadonly() as buffer:
        instance.Utilities.ExtractSlice(
            data, pos, orientation, (width, height), (pixSize, pixSize), image.path, options)  # TODO

        data = numpy.array(buffer.array)
    scanner = zbar.ImageScanner()

    allData = ""

    def scanImage(image):
        global allData
        zbarImage = zbar.Image(
            image.size[0], image.size[1], 'Y800', image.tobytes())
        scanner.scan(zbarImage)
        print(len(scanner.results))
        for result in scanner.results:
            data = '%s %s %s "%s"' % (
                result.type, result.quality, result.location, result.data)
            allData = allData + data + "\n"
            print(data)
            # im2 = image.copy ().convert ('RGB')
            # draw = ImageDraw.ImageDraw (im2)
            # line = list(result.location)
            # line.append (result.location[0])
            # draw.line (line, fill='blue')
            # im2.save ('/tmp/xx.png')

    # print (data.dtype)
    data[numpy.isnan(data)] = 0
    data -= numpy.min(data)
    data /= numpy.max(data)
    data *= 255

    image = Image.fromarray(data).convert('L')
    # image.save ('/tmp/qq.png')
    scanImage(image)

    image = image.transpose(Image.FLIP_LEFT_RIGHT)
    scanImage(image)

    import gtk
    md = gtk.MessageDialog(None,
                           gtk.DIALOG_DESTROY_WITH_PARENT, gtk.MESSAGE_INFO,
                           gtk.BUTTONS_CLOSE, "QT codes found:\n" + allData)
    md.run()
