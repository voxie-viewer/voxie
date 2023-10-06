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

from scipy import ndimage
import numpy as np
import voxie
import math

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

if args.voxie_action != 'RunFilter':
    raise Exception('Invalid operation: ' + repr(args.voxie_action))

with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationRunFilter']).ClaimOperationAndCatch() as op:
    inputData = op.GetInputData('de.uni_stuttgart.Voxie.Input').CastTo(
        'de.uni_stuttgart.Voxie.VolumeData')
    outputPath = op.Properties['de.uni_stuttgart.Voxie.Output'].getValue('o')

    operation = op.Properties['de.uni_stuttgart.Voxie.Filter.MorphologicalOperation.Mode'].getValue(
        's')
    shape = op.Properties['de.uni_stuttgart.Voxie.Filter.MorphologicalOperation.Shape'].getValue(
        's')
    structureSize = op.Properties['de.uni_stuttgart.Voxie.Filter.MorphologicalOperation.Size'].getValue(
        'x')
    isSegmented = op.Properties['de.uni_stuttgart.Voxie.Filter.MorphologicalOperation.Segmented'].getValue(
        'b')

    inputDataVoxel = inputData.CastTo('de.uni_stuttgart.Voxie.VolumeDataVoxel')

    with inputDataVoxel.GetBufferReadonly() as inputArray:
        with op.GetOutputVolumeDataVoxelLike(outputPath, inputDataVoxel, type=None) as data:
            with data.CreateUpdate() as update, data.GetBufferWritable(update) as outputBuffer:
                dimensionSize = int(structureSize) * 2 + 1
                center = int(dimensionSize / 2)
                struct = np.empty(
                    [dimensionSize, dimensionSize, dimensionSize], int)
                for x in range(dimensionSize):
                    for y in range(dimensionSize):
                        for z in range(dimensionSize):
                            if shape == 'de.uni_stuttgart.Voxie.Filter.MorphologicalOperation.Shape.Diamond':
                                # Manhattan distance
                                distance = abs(x - center) + \
                                    abs(y - center) + abs(z - center)
                            elif shape == 'de.uni_stuttgart.Voxie.Filter.MorphologicalOperation.Shape.Sphere':
                                # Euclid distance
                                distance = math.sqrt(
                                    (x - center) ** 2 + (y - center) ** 2 + (z - center) ** 2)
                            elif shape == 'de.uni_stuttgart.Voxie.Filter.MorphologicalOperation.Shape.Cube':
                                # Chessboard distance
                                distance = max(
                                    [abs(x - center), abs(y - center), abs(z - center)])
                            struct[x][y][z] = int(distance <= structureSize)

                if operation == 'de.uni_stuttgart.Voxie.Filter.MorphologicalOperation.Mode.Opening':
                    if isSegmented:
                        outputBuffer.array[:] = ndimage.morphology.binary_opening(
                            inputArray, structure=struct)
                    else:
                        outputBuffer.array[:] = ndimage.morphology.grey_opening(
                            inputArray, structure=struct)
                elif operation == 'de.uni_stuttgart.Voxie.Filter.MorphologicalOperation.Mode.Closing':
                    if isSegmented:
                        outputBuffer.array[:] = ndimage.morphology.binary_closing(
                            inputArray, structure=struct)
                    else:
                        outputBuffer.array[:] = ndimage.morphology.grey_closing(
                            inputArray, structure=struct)
                elif operation == 'de.uni_stuttgart.Voxie.Filter.MorphologicalOperation.Mode.Dilation':
                    if isSegmented:
                        outputBuffer.array[:] = ndimage.morphology.binary_dilation(
                            inputArray, structure=struct)
                    else:
                        outputBuffer.array[:] = ndimage.morphology.grey_dilation(
                            inputArray, structure=struct)
                elif operation == 'de.uni_stuttgart.Voxie.Filter.MorphologicalOperation.Mode.Erosion':
                    if isSegmented:
                        outputBuffer.array[:] = ndimage.morphology.binary_erosion(
                            inputArray, structure=struct)
                    else:
                        outputBuffer.array[:] = ndimage.morphology.grey_erosion(
                            inputArray, structure=struct)

                version = update.Finish()

            result = {}
            result[outputPath] = {
                'Data': voxie.Variant('o', data._objectPath),
                'DataVersion': voxie.Variant('o', version._objectPath),
            }
            op.Finish(result)
            version._referenceCountingObject.destroy()

context.client.destroy()
