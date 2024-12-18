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

start_time = time.monotonic()
with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationRunFilter']).ClaimOperationAndCatch() as op:
    filterPath = op.FilterObject
    pars = op.Parameters
    # print (pars)
    properties = pars[filterPath._objectPath]['Properties'].getValue('a{sv}')
    # print (properties)
    inputPath = properties['de.uni_stuttgart.Voxie.Input'].getValue('o')
    if inputPath == dbus.ObjectPath('/'):
        raise Exception('No input volume object connected')
    inputProperties = pars[inputPath]['Properties'].getValue('a{sv}')
    inputDataPath = pars[inputPath]['Data'].getValue('o')
    inputData = context.makeObject(context.bus, context.busName, inputDataPath, ['de.uni_stuttgart.Voxie.VolumeDataBlock'])

    outputPath = properties['de.uni_stuttgart.Voxie.Output'].getValue('o')

    arrayShape = inputData.ArrayShape
    blockShape = inputData.BlockShape
    dataType = ('float', 32, 'native')
    blockCount = inputData.BlockCount

    bufferType = instance.Components.GetComponent('de.uni_stuttgart.Voxie.ComponentType.BufferType', 'de.uni_stuttgart.Voxie.VolumeDataBlock.BlockOffsetEntry').CastTo('de.uni_stuttgart.Voxie.BufferType')

    bufferSize = blockCount[0] * blockCount[1]
    with instance.CreateBuffer(0, ['array', [bufferSize], [bufferType.SizeBytes], bufferType.Type]) as buffer:
        with instance.CreateVolumeDataVoxel(arrayShape, dataType, inputData.VolumeOrigin, inputData.GridSpacing) as data:
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

                    inputData.DecodeBlocks(data, update, bufferSize, buffer, 0)
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
