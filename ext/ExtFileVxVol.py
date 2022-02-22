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
import dbus
import sys
import json
import codecs
import os

import numpy as np

# Copied from ctscripts
import imagearchive

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

if args.voxie_action != 'Import':
    raise Exception('Invalid operation: ' + args.voxie_action)

# TODO: Test whether the StorageOrder stuff work properly

# TODO: Check checksum in iai file?

with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationImport']).ClaimOperationAndCatch() as op:
    filename = op.Filename

    with open(filename, 'rb') as file:
        jsonData = json.load(codecs.getreader('utf-8')(file))

    if jsonData['Type'] != 'de.uni_stuttgart.Voxie.FileFormat.Volume.VxVol':
        raise Exception('Expected type %s, got %s' % (
            repr('de.uni_stuttgart.Voxie.FileFormat.Volume.VxVol'), repr(jsonData['Type'])))

    if jsonData['VolumeType'] != 'Voxel':
        raise Exception('Expected volume type %s, got %s' % (
            repr('Voxel'), repr(jsonData['VolumeType'])))

    dataType = (str(jsonData['DataType'][0]), int(jsonData['DataType'][1]), str(jsonData['DataType'][2]))
    arrayShape = [int(jsonData['ArrayShape'][i]) for i in range(3)]
    gridSpacing = [float(jsonData['GridSpacing'][i]) for i in range(3)]
    volumeOrigin = [float(jsonData['VolumeOrigin'][i]) for i in range(3)]

    dataSourceType = str(jsonData['DataSourceType'])
    if dataSourceType != 'ImageArchive':
        raise Exception('Unknown DataSourceType: %s' % (repr(dataSourceType)))

    dataFilename = str(jsonData['DataSource']['DataFilename'])
    storageOrder = [int(jsonData['DataSource']['StorageOrder'][i]) for i in range(3)]

    rawDataFilename = os.path.join(os.path.dirname(filename), dataFilename)
    # iaiFilename = rawDataFilename + '.iai'

    # print (dataType)
    # print (arrayShape)
    # print (gridSpacing)
    # print (volumeOrigin)
    # print (storageOrder)
    # print (rawDataFilename)
    # # print (iaiFilename)

    expectedSize = [None, None, None]
    for resDim in range(3):
        so = storageOrder[resDim]
        aso = abs(so)
        if aso not in [1, 2, 3]:
            raise Exception('Got invalid StorageOrder value: %s' % (so,))
        if expectedSize[aso - 1] is not None:
            raise Exception('Got StorageOrder value %s multiple time' % (aso,))
        expectedSize[aso - 1] = arrayShape[resDim]

    ia = imagearchive.ImageArchive(rawDataFilename)

    if ia.count != expectedSize[2]:
        raise Exception('ia.count != expectedSize[2]')
    for z in range(expectedSize[2]):
        info = ia.info[z]
        if info['TypeName'] != dataType[0]:
            raise Exception("info['TypeName'] != dataType[0]")
        if int(info['TypeSize']) != dataType[1]:
            raise Exception("int(info['TypeSize']) (%d) != dataType[1] (%d)" % (int(info['TypeSize']), dataType[1]))
        if info['TypeEndian'] != dataType[2]:
            raise Exception("info['TypeEndian'] (%s) != dataType[2] (%s)", (repr(info['TypeEndian']), repr(dataType[2])))
        if int(info['Width']) != expectedSize[0]:
            raise Exception("int(info['Width']) != expectedSize[0]")
        if int(info['Height']) != expectedSize[1]:
            raise Exception("int(info['Height']) != expectedSize[1]")

    dataType2 = (dataType[0], dataType[1], 'native')

    with instance.CreateVolumeDataVoxel(arrayShape, dataType2, volumeOrigin, gridSpacing) as resultData:
        with resultData.CreateUpdate() as update, resultData.GetBufferWritable(update) as buffer:
            outData = buffer[()]
            # Apply storageOrder inverted
            mirror = []
            dim = []
            for i in range(3):
                if storageOrder[i] < 0:
                    mirror.append(slice(None, None, -1))
                else:
                    mirror.append(slice(None, None))
                dim.append(abs(storageOrder[i]) - 1)
            outData = outData[tuple(mirror)].transpose(dim)
            if outData.shape != tuple(expectedSize):
                raise Exception('outData.shape != tuple(expectedSize)')

            op.ThrowIfCancelled()
            for z in range(expectedSize[2]):
                # TODO: Avoid memory allocations here?
                img = ia.readImage(z)
                outData[:, :, z] = img

                op.ThrowIfCancelled()
                op.SetProgress((z + 1) / expectedSize[2])

            version = update.Finish()
        with version:
            op.Finish(resultData, version)

context.client.destroy()
