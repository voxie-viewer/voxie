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

import numpy as np
import voxie
import dbus
import time

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

if args.voxie_action != 'RunFilter':
    raise Exception('Invalid operation: ' + repr(args.voxie_action))


# algorithm taken from libjpeg, jcparam.c
# TODO: This should probably also take the SamplePrecision into account and
# multiply everything with 2**(SamplePrecision - 8).
def convertQualityToScalingFactor(quality):
    if quality <= 0:
        quality = 1
    if quality > 100:
        quality = 100

    if quality < 50:
        factor = 5000 / quality
    else:
        factor = 200 - quality * 2

    return factor / 100.0


# Return the quantization matrix scaled with the scaling factor.
def scaleQuantizationTable(table, scalingFactor, samplePrecision):
    maxValue = 65535
    if samplePrecision <= 8:
        maxValue = 255

    res = []
    for row in table:
        resrow = []
        for value in row:
            newValue = round(value * scalingFactor)
            if newValue > maxValue:
                newValue = maxValue
            if newValue < 1:
                newValue = 1
            resrow.append(newValue)
        res.append(resrow)

    return res


def toZigZag(input):
    output = [None] * 64
    x = 0
    y = 0
    upwards = True
    for i in range(64):
        output[i] = input[y * 8 + x]
        if upwards:
            if x < 7:
                x += 1
                if y > 0:
                    y -= 1
                else:
                    upwards = False
            else:
                y += 1
                upwards = False
        else:
            if y < 7:
                y += 1
                if x > 0:
                    x -= 1
                else:
                    upwards = True
            else:
                x += 1
                upwards = True
    return output


huffmanTableDC = {}
huffmanTableAC = {}

# tables from K.3.2
huffmanTableDC[8] = [
    [], [0], [1, 2, 3, 4, 5], [6], [7], [8], [9], [10], [11], [], [], [], [], [], [], [],
]
huffmanTableAC[8] = [
    [],
    [0x01, 0x02],
    [0x03],
    [0x00, 0x04, 0x11],
    [0x05, 0x12, 0x21],
    [0x31, 0x41],
    [0x06, 0x13, 0x51, 0x61],
    [0x07, 0x22, 0x71],
    [0x14, 0x32, 0x81, 0x91, 0xa1],
    [0x08, 0x23, 0x42, 0xb1, 0xc1],
    [0x15, 0x52, 0xd1, 0xf0],
    [0x24, 0x33, 0x62, 0x72],
    [],
    [],
    [0x82],
    [0x09, 0x0a, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa],
]

# TODO: What tables should be used in this case?
huffmanTableDC[12] = [
    [], [0], [1, 2, 3, 4, 5], [6], [7], [8], [9], [10], [11], [12], [13], [14], [15], [], [], []
]
huffmanTableAC[12] = [
    [],
    [],
    [],
    [],
    [],
    [],
    [],
    # TODO: This is a rather broken table
    # TODO: Check which codes are actually needed
    [x for x in range(256) if (x % 16 != 0 and x % 16 <= 0xd) or x == 0x00 or x == 0xf0],
    [],
    [],
    [],
    [],
    [],
    [],
    [],
    [],
]

# TODO: Should this be different for 12-bit JPEG?
# table from K.1
quantizationTable = [
    [16, 11, 10, 16, 24, 40, 51, 61],
    [12, 12, 14, 19, 26, 58, 60, 55],
    [14, 13, 16, 24, 40, 57, 69, 56],
    [14, 17, 22, 29, 51, 87, 80, 62],
    [18, 22, 37, 56, 68, 109, 103, 77],
    [24, 35, 55, 64, 81, 104, 113, 92],
    [49, 64, 78, 87, 103, 121, 120, 101],
    [72, 92, 95, 98, 112, 100, 103, 99]
]

start_time = time.monotonic()
with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationRunFilter']).ClaimOperationAndCatch() as op:
    inputData = op.GetInputData('de.uni_stuttgart.Voxie.Input').CastTo('de.uni_stuttgart.Voxie.VolumeDataVoxel')

    outputPath = op.Properties['de.uni_stuttgart.Voxie.Output'].getValue('o')

    quality = op.Properties['de.uni_stuttgart.Voxie.Filter.CompressVolumeJpeg.Quality'].getValue('x')

    arrayShape = inputData.ArrayShape
    blockShape = (16, 16, 16)
    samplePrecision = 8

    scalingFactor = convertQualityToScalingFactor(quality)
    scaledQuantizationTable = scaleQuantizationTable(quantizationTable, scalingFactor=scalingFactor, samplePrecision=samplePrecision)

    # TODO: Use percentile?
    lowerMethod = op.Properties['de.uni_stuttgart.Voxie.Filter.CompressVolumeJpeg.RangeSelection.Lower'].getValue('s')
    upperMethod = op.Properties['de.uni_stuttgart.Voxie.Filter.CompressVolumeJpeg.RangeSelection.Upper'].getValue('s')
    print(lowerMethod, upperMethod)
    minimum = None
    if lowerMethod == 'de.uni_stuttgart.Voxie.Filter.CompressVolumeJpeg.RangeSelection.MinMax':
        minimum = np.min(inputData[:])
        lower = minimum
    elif lowerMethod == 'de.uni_stuttgart.Voxie.Filter.CompressVolumeJpeg.RangeSelection.Manual':
        lower = op.Properties['de.uni_stuttgart.Voxie.Filter.CompressVolumeJpeg.RangeSelection.Lower.ManualValue'].getValue('d')
    else:
        raise Exception('Unknown value for RangeSelection: {!r}'.format(lowerMethod))
    maximum = None
    if upperMethod == 'de.uni_stuttgart.Voxie.Filter.CompressVolumeJpeg.RangeSelection.MinMax':
        maximum = np.max(inputData[:])
        upper = maximum
    elif upperMethod == 'de.uni_stuttgart.Voxie.Filter.CompressVolumeJpeg.RangeSelection.Manual':
        upper = op.Properties['de.uni_stuttgart.Voxie.Filter.CompressVolumeJpeg.RangeSelection.Upper.ManualValue'].getValue('d')
    else:
        raise Exception('Unknown value for RangeSelection: {!r}'.format(upperMethod))
    print('min={}, max={}, lower={}, upper={}'.format(minimum, maximum, lower, upper), flush=True)
    dataRange = upper - lower
    valueScalingFactor = dataRange / (2 ** samplePrecision - 1)
    valueOffset = lower + (2 ** (samplePrecision - 1)) * valueScalingFactor
    print('valueOffset={}, valueScalingFactor={}'.format(valueOffset, valueScalingFactor), flush=True)

    bufferType = instance.Components.GetComponent('de.uni_stuttgart.Voxie.ComponentType.BufferType', 'de.uni_stuttgart.Voxie.VolumeDataBlock.BlockOffsetEntry').CastTo('de.uni_stuttgart.Voxie.BufferType')

    with instance.CreateVolumeDataBlockJpeg(arrayShape, blockShape, inputData.VolumeOrigin, inputData.GridSpacing, valueOffset, valueScalingFactor, samplePrecision, huffmanTableDC[samplePrecision], huffmanTableAC[samplePrecision], scaledQuantizationTable) as data:
        blockCount = data.BlockCount
        bufferSize = blockCount[0] * blockCount[1]
        with instance.CreateBuffer(0, ['array', [bufferSize], [bufferType.SizeBytes], bufferType.Type]) as buffer:
            with data.CreateUpdate() as update:
                for z in range(0, blockCount[2]):
                    op.ThrowIfCancelled()

                    # TODO: Clean up

                    blockIDsPos = np.arange(blockCount[0] * blockCount[1], dtype=np.uint64)
                    blockIDsX = blockIDsPos % blockCount[0]
                    blockIDsY = blockIDsPos // blockCount[0]

                    # Calculate block shapes
                    blockShapeX = np.full((blockCount[0], blockCount[1]), blockShape[0], dtype=np.uint64)
                    blockShapeY = np.full((blockCount[0], blockCount[1]), blockShape[1], dtype=np.uint64)
                    blockShapeZ = np.full((blockCount[0], blockCount[1]), blockShape[2], dtype=np.uint64)
                    blockShapeX[-1, :] = arrayShape[0] - (blockCount[0] - 1) * blockShape[0]
                    blockShapeY[:, -1] = arrayShape[1] - (blockCount[1] - 1) * blockShape[1]
                    if z + 1 == blockCount[2]:
                        blockShapeZ[:, :] = arrayShape[2] - (blockCount[2] - 1) * blockShape[2]

                    buffer[()].BlockID[:, 0] = blockIDsX
                    buffer[()].BlockID[:, 1] = blockIDsY
                    buffer[()].BlockID[:, 2] = z
                    buffer[()].Offset[:, 0] = blockIDsX * blockShape[0]
                    buffer[()].Offset[:, 1] = blockIDsY * blockShape[1]
                    buffer[()].Offset[:, 2] = z * blockShape[2]
                    buffer[()].BlockShape[:, 0] = blockShapeX[blockIDsX, blockIDsY]
                    buffer[()].BlockShape[:, 1] = blockShapeY[blockIDsX, blockIDsY]
                    buffer[()].BlockShape[:, 2] = blockShapeZ[blockIDsX, blockIDsY]

                    data.EncodeBlocks(update, inputData, bufferSize, buffer, 0)
                    op.SetProgress((z + 1) / blockCount[2])
                version = update.Finish()

        with version:
            result = {}
            result[outputPath] = {
                'Data': voxie.Variant('o', data._objectPath),
                'DataVersion': voxie.Variant('o', version._objectPath),
            }
            op.Finish(result)

context.client.destroy()
print('Wall clock time: {}, CPU time: {}'.format(time.monotonic() - start_time, time.process_time()))
