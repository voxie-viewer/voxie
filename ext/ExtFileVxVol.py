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
import voxie.json_util
import voxie.data_properties

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

        if 'de.uni_stuttgart.Voxie.VolumeDataBlockJpeg' in data.SupportedInterfaces:
            vty = 'BlockJpeg'
            data = data.CastTo('de.uni_stuttgart.Voxie.VolumeDataBlockJpeg')
        elif 'de.uni_stuttgart.Voxie.VolumeDataVoxel' in data.SupportedInterfaces:
            vty = 'Voxel'
            data = data.CastTo('de.uni_stuttgart.Voxie.VolumeDataVoxel')
        else:
            # TODO: More volume types?
            raise Exception('Cannot save unknown volume type, available interfaces are: {}'.format(data.SupportedInterfaces))

        baseFilename = filename
        if baseFilename.endswith('.json'):
            baseFilename = baseFilename[:-len('.json')]
        dataFilename = baseFilename + '.dat'
        dataFilenameBase = os.path.basename(dataFilename)
        lenFilename = baseFilename + '.len'
        lenFilenameBase = os.path.basename(lenFilename)

        # print('Export %s %s' % (filename, data))

        shape = data.ArrayShape
        spacing = data.GridSpacing
        origin = data.VolumeOrigin

        if vty == 'Voxel':
            ty = data.DataType

            fileType = ty
            if fileType[1] % 8 != 0:
                raise Exception('Data type size is not multiple of 8')

            # TODO: Create wrapper function in voxie namespace for this
            fileDtype = voxie.buffer.endianMap[fileType[2]] + voxie.buffer.typeNameMap[fileType[0]] + str(fileType[1] // 8)

            expectedCount = fileType[1] // 8 * shape[0] * shape[1]

            result = {
                'Type': 'de.uni_stuttgart.Voxie.FileFormat.Volume.VxVol',
                'VolumeType': vty,
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
        elif vty == 'BlockJpeg':
            blockShape = data.BlockShape

            blockCount = data.BlockCount
            blockCount2 = [None, None, None]
            for i in range(3):
                blockCount2[i] = (shape[i] + blockShape[i] - 1) // blockShape[i]
            blockCount2 = tuple(blockCount2)
            if blockCount != blockCount2:
                raise Exception('blockCount != blockCount2')

            result = {
                'Type': 'de.uni_stuttgart.Voxie.FileFormat.Volume.VxVol',
                'VolumeType': vty,
                'ArrayShape': shape,
                'BlockShape': blockShape,
                'GridSpacing': spacing,
                'VolumeOrigin': origin,
                'BlockJpegType': '2D_XYSlices',
                'StorageType': 'VLQLengthsAndBitstream',
                'JpegInfo': {
                    'ValueOffset': data.ValueOffset,
                    'ValueScalingFactor': data.ValueScalingFactor,
                    'SamplePrecision': data.SamplePrecision,
                    'HuffmanTableDC': data.HuffmanTableDC,
                    'HuffmanTableAC': data.HuffmanTableAC,
                    'QuantizationTable': data.QuantizationTable,
                },
                'VLQLengthFilename': lenFilenameBase,
                'BitstreamFilename': dataFilenameBase,
            }
        voxie.data_properties.save_properties(data, result)
        # print(result)

        # TODO: Support scaling and different data types?

        progressMax = 0
        progressSave = 1

        if vty == 'Voxel':
            with data.GetBufferReadonly() as inputArray:
                with open(dataFilename, 'wb') as file:
                    # TODO: Avoid doing copy? Might cause problems if e.g. voxie no longer uses continuous arrays
                    img = np.zeros((shape[1], shape[0]), dtype=fileDtype)
                    imgT = img.T
                    for z in range(shape[2]):
                        op.ThrowIfCancelled()
                        op.SetProgress(progressMax + progressSave * z / shape[2])
                        imgT[:, :] = inputArray[:, :, z]
                        count = file.write(img)
                        if count != expectedCount:
                            raise Exception('count != expectedCount')
                op.SetProgress(progressMax + progressSave)
        elif vty == 'BlockJpeg':
            maxBlockCount = 128 * 1024

            overallBlocks = blockCount[0] * blockCount[1] * blockCount[2]

            if maxBlockCount > overallBlocks:
                maxBlockCount = overallBlocks

            dataBufferSize = 16 * 1024 * 1024

            blockIDBT = instance.Components.GetComponent('de.uni_stuttgart.Voxie.ComponentType.BufferType', 'de.uni_stuttgart.Voxie.VolumeDataBlock.BlockID').CastTo('de.uni_stuttgart.Voxie.BufferType')
            blockSizeBT = instance.Components.GetComponent('de.uni_stuttgart.Voxie.ComponentType.BufferType', 'de.uni_stuttgart.Voxie.VolumeDataBlockJpeg.BlockSize').CastTo('de.uni_stuttgart.Voxie.BufferType')

            with instance.CreateBuffer(0, ['array', [maxBlockCount], [blockIDBT.SizeBytes], blockIDBT.Type]) as blockIDsBuffer, instance.CreateBuffer(0, ['array', [dataBufferSize], [1], ['primitive', 'uint', 8, 'none']]) as dataBuffer, instance.CreateBuffer(0, ['array', [maxBlockCount], [blockSizeBT.SizeBytes], blockSizeBT.Type]) as blockSizeBuffer:
                with open(dataFilename, 'wb') as file, open(lenFilename, 'wb') as lenFile:
                    pos = 0
                    while pos < overallBlocks:
                        op.ThrowIfCancelled()
                        op.SetProgress(progressMax + progressSave * pos / overallBlocks)
                        count = overallBlocks - pos
                        if count > maxBlockCount:
                            count = maxBlockCount

                        # Fill with value causing error
                        # TODO: Don't do this always?
                        blockIDsBuffer[:] = 2 ** 64 - 1

                        # Calculate block IDs
                        linIDs = pos + np.arange(count, dtype=np.uint64)
                        blockIDsBuffer[:count, 0] = linIDs % blockCount[0]
                        linIDs //= blockCount[0]
                        blockIDsBuffer[:count, 1] = linIDs % blockCount[1]
                        linIDs //= blockCount[1]
                        blockIDsBuffer[:count, 2] = linIDs % blockCount[2]

                        # Get compressed data
                        actualBlocks, actualBytes = data.GetCompressedData(count, blockIDsBuffer, 0, dataBuffer, 0, dataBufferSize, blockSizeBuffer, 0)

                        if actualBlocks == 0:
                            raise Exception('Failed to get compressed data for block {} with {} bytes buffer size'.format(blockIDsBuffer[0], dataBufferSize))

                        # Write actual data
                        # print('Writing {}'.format(len(dataBuffer[:actualBytes])))
                        file.write(dataBuffer[:actualBytes].tobytes())

                        # Write lengths
                        lengths = blockSizeBuffer[:actualBlocks]
                        # TODO: Avoid doing this in python?
                        # See e.g. https://en.wikipedia.org/wiki/Variable-length_quantity
                        # print('A', flush=True)
                        res = []
                        for value in lengths:
                            value = int(value)
                            # print(value)
                            if value < 128:
                                res.append(value)
                            else:
                                d = [value & 0x7f]
                                value >>= 7
                                while value > 0:
                                    d.append((value & 0x7f) | 0x80)
                                    value >>= 7
                                res += reversed(d)
                        # print('B', flush=True)
                        lenFile.write(bytes(res))
                        # print('C', flush=True)

                        # Continue
                        # TODO: Reuse part of the blockIDsBuffer if not all blocks were read?
                        pos += actualBlocks
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

        if voxie.json_util.expect_string(jsonData['Type']) != 'de.uni_stuttgart.Voxie.FileFormat.Volume.VxVol':
            raise Exception('Expected type %s, got %s' % (
                repr('de.uni_stuttgart.Voxie.FileFormat.Volume.VxVol'), repr(jsonData['Type'])))

        if voxie.json_util.expect_string(jsonData['VolumeType']) == 'Voxel':
            throwOnUnexpectedMember(jsonData, 'JSON file', ('Type', 'VolumeType', 'DataType', 'ArrayShape', 'GridSpacing', 'VolumeOrigin', 'DataSourceType', 'DataSource', 'DataProperties'))

            dataType = (voxie.json_util.expect_string(jsonData['DataType'][0]), voxie.json_util.expect_int(jsonData['DataType'][1]), voxie.json_util.expect_string(jsonData['DataType'][2]))
            arrayShape = voxie.json_util.expect_int3(jsonData['ArrayShape'])
            gridSpacing = voxie.json_util.expect_float3(jsonData['GridSpacing'])
            volumeOrigin = voxie.json_util.expect_float3(jsonData['VolumeOrigin'])

            dataSourceType = jsonData['DataSourceType']

            with data_source.get(filename, dataSourceType, jsonData['DataSource'], array_shape=arrayShape, data_type=dataType) as source:
                with instance.CreateVolumeDataVoxel(arrayShape, source.memType, volumeOrigin, gridSpacing) as resultData:
                    with resultData.CreateUpdate() as update, resultData.GetBufferWritable(update) as buffer:
                        voxie.data_properties.load_properties(instance, resultData, update, jsonData)

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
        elif voxie.json_util.expect_string(jsonData['VolumeType']) == 'BlockJpeg':
            if voxie.json_util.expect_string(jsonData['BlockJpegType']) != '2D_XYSlices':
                raise Exception('Expected BlockJpeg type %s, got %s' % (
                    repr(['2D_XYSlices']), repr(jsonData['BlockJpegType'])))

            if voxie.json_util.expect_string(jsonData['StorageType']) != 'VLQLengthsAndBitstream':
                raise Exception('Expected BlockJpeg storage type %s, got %s' % (
                    repr(['VLQLengthsAndBitstream']), repr(jsonData['StorageType'])))

            throwOnUnexpectedMember(jsonData, 'JSON file', ('Type', 'VolumeType', 'ArrayShape', 'BlockShape', 'GridSpacing', 'VolumeOrigin', 'BlockJpegType', 'JpegInfo', 'StorageType', 'BitstreamFilename', 'VLQLengthFilename', 'DataProperties'))
            jpegInfo = voxie.json_util.expect_object(jsonData['JpegInfo'])
            throwOnUnexpectedMember(jpegInfo, 'JpegInfo', ('ValueOffset', 'ValueScalingFactor', 'SamplePrecision', 'HuffmanTableDC', 'HuffmanTableAC', 'QuantizationTable'))

            arrayShape = voxie.json_util.expect_int3(jsonData['ArrayShape'])
            blockShape = voxie.json_util.expect_int3(jsonData['BlockShape'])
            gridSpacing = voxie.json_util.expect_float3(jsonData['GridSpacing'])
            volumeOrigin = voxie.json_util.expect_float3(jsonData['VolumeOrigin'])

            dataFilename = voxie.json_util.expect_string(jsonData['BitstreamFilename'])
            lenFilename = voxie.json_util.expect_string(jsonData['VLQLengthFilename'])

            dataFilename = os.path.join(os.path.dirname(filename), dataFilename)
            lenFilename = os.path.join(os.path.dirname(filename), lenFilename)

            valueScalingFactor = voxie.json_util.expect_float(jpegInfo['ValueScalingFactor'])
            valueOffset = voxie.json_util.expect_float(jpegInfo['ValueOffset'])
            samplePrecision = voxie.json_util.expect_int(jpegInfo['SamplePrecision'])
            huffmanTableDC = voxie.json_util.expect_array(jpegInfo['HuffmanTableDC'])
            for entry in huffmanTableDC:
                voxie.json_util.expect_array(entry)
                for entry2 in entry:
                    voxie.json_util.expect_int(entry2)
            huffmanTableAC = voxie.json_util.expect_array(jpegInfo['HuffmanTableAC'])
            for entry in huffmanTableAC:
                voxie.json_util.expect_array(entry)
                for entry2 in entry:
                    voxie.json_util.expect_int(entry2)
            quantizationTable = voxie.json_util.expect_array(jpegInfo['QuantizationTable'])
            for entry in quantizationTable:
                voxie.json_util.expect_array(entry)
                for entry2 in entry:
                    voxie.json_util.expect_int(entry2)

            blockCount = [None, None, None]
            for i in range(3):
                blockCount[i] = (arrayShape[i] + blockShape[i] - 1) // blockShape[i]
            blockCount = tuple(blockCount)

            overallBlocks = blockCount[0] * blockCount[1] * blockCount[2]

            maxBlockCount = 128 * 1024

            if maxBlockCount > overallBlocks:
                maxBlockCount = overallBlocks

            dataBufferSize = 16 * 1024 * 1024

            lenBufferSize = 128 * 1024

            blockIDBT = instance.Components.GetComponent('de.uni_stuttgart.Voxie.ComponentType.BufferType', 'de.uni_stuttgart.Voxie.VolumeDataBlock.BlockID').CastTo('de.uni_stuttgart.Voxie.BufferType')
            blockSizeBT = instance.Components.GetComponent('de.uni_stuttgart.Voxie.ComponentType.BufferType', 'de.uni_stuttgart.Voxie.VolumeDataBlockJpeg.BlockSize').CastTo('de.uni_stuttgart.Voxie.BufferType')

            progressMax = 0
            progressSave = 1

            with instance.CreateBuffer(0, ['array', [maxBlockCount], [blockIDBT.SizeBytes], blockIDBT.Type]) as blockIDsBuffer, instance.CreateBuffer(0, ['array', [dataBufferSize], [1], ['primitive', 'uint', 8, 'none']]) as dataBuffer, instance.CreateBuffer(0, ['array', [maxBlockCount], [blockSizeBT.SizeBytes], blockSizeBT.Type]) as blockSizeBuffer:
                with open(dataFilename, 'rb') as file, open(lenFilename, 'rb') as lenFile:
                    with instance.CreateVolumeDataBlockJpeg(arrayShape, blockShape, volumeOrigin, gridSpacing, valueOffset, valueScalingFactor, samplePrecision, huffmanTableDC, huffmanTableAC, quantizationTable) as resultData:
                        if resultData.BlockCount != blockCount:
                            raise Exception('resultData.BlockCount != blockCount')
                        pendingLen = None
                        lenData = b''
                        lenDataPos = 0
                        with resultData.CreateUpdate() as update:
                            voxie.data_properties.load_properties(instance, resultData, update, jsonData)

                            pos = 0
                            while pos < overallBlocks:
                                op.ThrowIfCancelled()
                                op.SetProgress(progressMax + progressSave * pos / overallBlocks)
                                count = overallBlocks - pos
                                if count > maxBlockCount:
                                    count = maxBlockCount

                                # Read lengths
                                # TODO: Avoid doing this in python?
                                blockSizeBufferVals = blockSizeBuffer[:]
                                # blockSizeBufferVals = blockSizeBuffer
                                # See e.g. https://en.wikipedia.org/wiki/Variable-length_quantity
                                # print('A', flush=True)
                                lpos = 0
                                sum = 0
                                while lpos < count:
                                    val = 0
                                    if pendingLen is not None:
                                        val = pendingLen
                                        pendingLen = None
                                    else:
                                        while True:
                                            # Read 1 byte
                                            if lenDataPos == len(lenData):
                                                # print('RA', flush=True)
                                                lenData = lenFile.read(lenBufferSize)
                                                lenDataPos = 0
                                                if len(lenData) == 0:
                                                    raise Exception('Got EOF in length file')
                                                # print('RB', flush=True)
                                            b = lenData[lenDataPos]
                                            lenDataPos += 1

                                            val |= b & 0x7f
                                            if (b & 0x80) != 0:
                                                val <<= 7
                                            else:
                                                break
                                    # print(val, flush=True)
                                    if sum + val > dataBufferSize:
                                        if lpos == 0:
                                            raise Exception('Got block with size {} which is longer than dataBufferSize {} at pos {}'.format(val, dataBufferSize, pos))
                                        else:
                                            pendingLen = val
                                            break
                                    blockSizeBufferVals[lpos] = val
                                    sum += val
                                    lpos += 1
                                count = lpos
                                # print('B', flush=True)

                                # Read actual data
                                # print('Reading {}'.format(sum))
                                dataBuffer[:sum] = np.frombuffer(file.read(sum), dtype=np.uint8)

                                # Fill with value causing error
                                # TODO: Don't do this always?
                                blockIDsBuffer[:] = 2 ** 64 - 1

                                # Calculate block IDs
                                linIDs = pos + np.arange(count, dtype=np.uint64)
                                blockIDsBuffer[:count, 0] = linIDs % blockCount[0]
                                linIDs //= blockCount[0]
                                blockIDsBuffer[:count, 1] = linIDs % blockCount[1]
                                linIDs //= blockCount[1]
                                blockIDsBuffer[:count, 2] = linIDs % blockCount[2]

                                # Set compressed data
                                resultData.SetCompressedData(count, blockIDsBuffer, 0, dataBuffer, 0, dataBufferSize, blockSizeBuffer, 0)

                                # Continue
                                # TODO: Reuse part of the blockIDsBuffer?
                                pos += count
                            if pendingLen or lenFile.read(1) != b'':
                                raise Exception('Got garabge at end of length file')
                            if file.read(1) != b'':
                                raise Exception('Got garabge at end of data file')
                            op.SetProgress(progressMax + progressSave)
                            version = update.Finish()
                        with version:
                            op.Finish(resultData, version)
        else:
            raise Exception('Expected volume type %s, got %s' % (
                repr(['Voxel', 'BlockJpeg']), repr(jsonData['VolumeType'])))

context.client.destroy()
