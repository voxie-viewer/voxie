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
import dbus

import voxie

args = voxie.parser.parse_args()

instance = voxie.Voxie(args)

with instance.createClient() as client:
    with instance.createVolumeDataVoxel(client, (3, 3, 3), {}) as data, data.getDataWritable() as buffer:
        array = buffer.array
        array[:] = 0
        array[1, 1, 1] = 1

        dataSet = instance.createVolumeObject('TestData', data)

    slicePlugin = instance.getPlugin('Vis3D')
    visualizerPrototype = slicePlugin.getMemberDBus(
        'de.uni_stuttgart.Voxie.VisualizerPrototype', 'IsosurfaceMetaVisualizer')
    visualizerPrototype.Create([dataSet.path], dbus.Array(
        signature='o'), voxie.emptyOptions)
