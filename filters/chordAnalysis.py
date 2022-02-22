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

import voxie
from voxie.table import *

import numpy
import dbus
from skimage import measure as skmeasure
from scipy import ndimage, nan, inf


args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationRunFilter']).ClaimOperationAndCatch() as op:
    filterPath = op.FilterObject
    pars = op.Parameters
    properties = pars[filterPath._objectPath]['Properties'].getValue('a{sv}')

    labelPath = properties['de.uni_stuttgart.Voxie.Filter.ChordAnalysis.LabelVolume'].getValue(
        'o')
    if labelPath == dbus.ObjectPath('/'):
        raise Exception('No label volume specified')
    labelProperties = pars[labelPath]['Properties'].getValue('a{sv}')
    labelDataPath = pars[labelPath]['Data'].getValue('o')
    labelData = context.makeObject(context.bus, context.busName, labelDataPath, [
                                   'de.uni_stuttgart.Voxie.Data']).CastTo('de.uni_stuttgart.Voxie.VolumeDataVoxel')[:]

    volumePath = properties['de.uni_stuttgart.Voxie.Filter.ChordAnalysis.Volume'].getValue(
        'o')
    if volumePath == dbus.ObjectPath('/'):
        print('Information: Without the original VolumeObject not all values can be calculated!')
        volumeData = None
    else:
        volumeProperties = pars[volumePath]['Properties'].getValue('a{sv}')
        volumeDataPath = pars[volumePath]['Data'].getValue('o')
        volumeData = context.makeObject(context.bus, context.busName, volumeDataPath, [
                                        'de.uni_stuttgart.Voxie.Data']).CastTo('de.uni_stuttgart.Voxie.VolumeDataVoxel')[:]

    # Ensure that volume data has the same dimensions as the label data
    if volumeData is not None and volumeData.shape != labelData.shape:
        print('Warning: Volume object dimensions do not match up with Label object dimensions. Values that depend on the voxel data values will not be calculated.')
        volumeData = None

    outputPath = properties['de.uni_stuttgart.Voxie.Output'].getValue('o')

    # Define columns for table
    columns = [
        TableColumn.int('ChordID', 'Chord ID'),
        TableColumn.vec3('StartPosition', 'Start Position'),
        TableColumn.vec3('EndPosition', 'End Position'),
        TableColumn.float('Length', 'Chord Length'),
    ]

    # Show additional columns when volume dataset is available
    if volumeData is not None:
        columns += [
            TableColumn.int('Axis', 'Chord Axis'),
        ]

    with instance.CreateTableData(createColumnDefinition(instance, columns)) as resultData:
        with resultData.CreateUpdate() as update:

            # Flatten label data to enable voxel counting
            flatLabelData = labelData.reshape(-1)
            labelVoxelCounts = numpy.bincount(flatLabelData)
            if volumeData is not None:
                labelVoxelSums = numpy.bincount(
                    flatLabelData, weights=volumeData.reshape(-1))

            # Shift label IDs by 1 to work around SciPy ignoring zero-labeled
            # regions
            regions = ndimage.find_objects(labelData + 1)

            for labelID, slices in enumerate(regions):
                if slices is not None and labelID != 0:

                    chordID = labelID
                    startPosition = [slice.start for slice in slices]
                    endPosition = [slice.stop - 1 for slice in slices]
                    chordLength = numpy.sum(
                        [slice.stop - slice.start - 1 for slice in slices]) + 1
                    if volumeData is not None:
                        axis = volumeData[startPosition[0],
                                          startPosition[1], startPosition[2]]
                    else:
                        axis = -1

                    # Assign region data to row dict
                    row = {
                        'ChordID': chordID,
                        'StartPosition': startPosition,
                        'EndPosition': endPosition,
                        'Length': chordLength,
                        'Axis': axis,
                    }
                    resultData.AddRow(update, createRow(columns, row))

            version = update.Finish()
        result = {}
        result[outputPath] = {
            'Data': voxie.Variant('o', resultData._objectPath),
            'DataVersion': voxie.Variant('o', version._objectPath),
        }
        op.Finish(result)
