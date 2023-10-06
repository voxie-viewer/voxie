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
import io

import numpy as np

import data_source

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

if args.voxie_action != 'Export' and args.voxie_action != 'Import':
    raise Exception('Invalid operation: ' + repr(args.voxie_action))


def throwOnUnexpectedMember(data, desc, expected):
    expected = set(expected)
    optional = set()
    if 'OptionalMembers' in data:
        optional = set(data['OptionalMembers'])
    for key in data:
        if key not in expected and key not in optional:
            raise Exception('Unexpected member {!r} in {}'.format(key, desc))


if args.voxie_action == 'Export':
    with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationExport']).ClaimOperationAndCatch() as op:
        filename = op.Filename
        data = op.Data.CastTo('de.uni_stuttgart.Voxie.VolumeData')
        # TODO: Non-voxel data types?
        data = data.CastTo('de.uni_stuttgart.Voxie.VolumeDataVoxel')

        baseFilename = filename
        if baseFilename.endswith('.json'):
            baseFilename = baseFilename[:-len('.json')]
        dataFilename = baseFilename + '.dat'
        dataFilenameBase = os.path.basename(dataFilename)

        # print('Export %s %s' % (filename, data))

        shape = data.ArrayShape
        spacing = data.GridSpacing
        origin = data.VolumeOrigin
        ty = data.DataType

        fileType = ty
        if fileType[1] % 8 != 0:
            raise Exception('Data type size is not multiple of 8')

        # TODO: Create wrapper function in voxie namespace for this
        fileDtype = voxie.buffer.endianMap[fileType[2]] + voxie.buffer.typeNameMap[fileType[0]] + str(fileType[1] // 8)

        expectedCount = fileType[1] // 8 * shape[0] * shape[1]

        result = {
            'Type': 'de.uni_stuttgart.Voxie.FileFormat.Volume.VxVol',
            'VolumeType': 'Voxel',
            'DataType': fileType,
            'ArrayShape': shape,
            'GridSpacing': spacing,
            'VolumeOrigin': origin,
            'DataSourceType': 'DenseBinaryFile',
            'DataSource': {
                'DataFilename': dataFilenameBase,
                # 'Offset': 0,
                # 'StorageOrder': [1, 2, 3],
                # 'ValueOffset': 0,
                # 'ValueScalingFactor': 1.0,
            },
        }
        # print(result)

        # TODO: Support scaling and different data types?

        with data.GetBufferReadonly() as inputArray:
            progressMax = 0
            progressSave = 1

            with open(dataFilename, 'wb') as file:
                # TODO: Avoid doing copy? Might cause problems if e.g. voxie no longer uses continuous arrays
                img = np.zeros((shape[1], shape[0]), dtype=fileDtype)
                imgT = img.T
                for z in range(shape[2]):
                    op.SetProgress(progressMax + progressSave * z / shape[2])
                    imgT[:, :] = inputArray[:, :, z]
                    count = file.write(img)
                    if count != expectedCount:
                        raise Exception('count != expectedCount')
            op.SetProgress(progressMax + progressSave)

        f = io.StringIO()
        json.dump(result, f, allow_nan=False, sort_keys=True,
                  ensure_ascii=False, indent=2)
        s = bytes(f.getvalue() + '\n', 'utf-8')
        with open(filename, 'wb') as file:
            file.write(s)

        op.Finish()
else:
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

        throwOnUnexpectedMember(jsonData, 'JSON file', ('Type', 'VolumeType', 'DataType', 'ArrayShape', 'GridSpacing', 'VolumeOrigin', 'DataSourceType', 'DataSource'))

        dataType = (str(jsonData['DataType'][0]), int(jsonData['DataType'][1]), str(jsonData['DataType'][2]))
        arrayShape = [int(jsonData['ArrayShape'][i]) for i in range(3)]
        gridSpacing = [float(jsonData['GridSpacing'][i]) for i in range(3)]
        volumeOrigin = [float(jsonData['VolumeOrigin'][i]) for i in range(3)]

        dataSourceType = jsonData['DataSourceType']

        with data_source.get(filename, dataSourceType, jsonData['DataSource'], array_shape=arrayShape, data_type=dataType) as source:
            with instance.CreateVolumeDataVoxel(arrayShape, source.memType, volumeOrigin, gridSpacing) as resultData:
                with resultData.CreateUpdate() as update, resultData.GetBufferWritable(update) as buffer:
                    outData = buffer[()]
                    # Apply storageOrder
                    outData = source.with_storage_order_output(outData)

                    op.ThrowIfCancelled()
                    for z in range(source.diskSize[2]):
                        outData[:, :, z] = source.read_image_disk_coord(z)

                        op.ThrowIfCancelled()
                        op.SetProgress((z + 1) / source.diskSize[2])

                    version = update.Finish()
                with version:
                    op.Finish(resultData, version)

context.client.destroy()
