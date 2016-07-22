#!/usr/bin/python3

import sys
import os
import numpy
import time
import dbus

import voxie

args = voxie.parser.parse_args()

instance = voxie.Voxie(args)

with instance.createClient() as client:
    with instance.createVoxelData(client, (3,3,3), {}) as data, data.getDataWritable() as buffer:
        array = buffer.array
        array[:] = 0
        array[1,1,1] = 1

        dataSet = instance.createDataSet('TestData', data)

    slicePlugin = instance.getPlugin('Vis3D')
    visualizerFactory = slicePlugin.getMemberDBus('de.uni_stuttgart.Voxie.VisualizerFactory', 'IsosurfaceMetaVisualizer')
    visualizerFactory.Create([dataSet.path], dbus.Array(signature='o'), voxie.emptyOptions)
