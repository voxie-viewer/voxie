#!/usr/bin/python3

# The data used for creating src/Main/icons-voxie/voxel-data-483.png
# (using a cuberille isosurface)

import sys
import os
import numpy
import time
import dbus

import voxie

args = voxie.parser.parse_args()

instance = voxie.Voxie(args)

with instance.createClient() as client:
    with instance.createVoxelData(client, (2, 2, 2), {}) as data, data.getDataWritable() as buffer:
        array = buffer.array
        array[:] = 0
        array[0,0,0] = 1
        array[1,1,0] = 1
        array[0,1,1] = 1

        dataSet = instance.createDataSet('IconData', data)

    slicePlugin = instance.getPlugin('Vis3D')
    visualizerFactory = slicePlugin.getMemberDBus('de.uni_stuttgart.Voxie.VisualizerFactory', 'IsosurfaceMetaVisualizer')
    visualizerFactory.Create([dataSet.path], dbus.Array(signature='o'), voxie.emptyOptions)
