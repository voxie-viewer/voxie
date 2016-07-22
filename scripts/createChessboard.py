#!/usr/bin/python3

import sys
import os
import numpy
import time
import dbus

import voxie

args = voxie.parser.parse_args()

instance = voxie.Voxie(args)

fieldSize = 10
fields = 8
overallSize = fieldSize * fields

spacing = 1.0 / fieldSize
origin = -fields / 2.0

with instance.createClient() as client:
    with instance.createVoxelData(client, (overallSize, overallSize, overallSize), {'Origin': (origin, origin, origin), 'Spacing': (spacing, spacing, spacing)}) as data, data.getDataWritable() as buffer:
        array = buffer.array.reshape((fields, fieldSize, fields, fieldSize, fields, fieldSize))
        array[:] = 0
        array[::2,:,::2,:,::2,:] = 1
        array[1::2,:,1::2,:,::2,:] = 1
        array[1::2,:,::2,:,1::2,:] = 1
        array[::2,:,1::2,:,1::2,:] = 1

        dataSet = instance.createDataSet('Chessboard', data)

    slice = dataSet.createSlice()

    slicePlugin = instance.getPlugin('VisSlice')
    visualizerFactory = slicePlugin.getMemberDBus('de.uni_stuttgart.Voxie.VisualizerFactory', 'SliceMetaVisualizer')
    visualizerFactory.Create(dbus.Array(signature='o'), [slice.path], voxie.emptyOptions)
