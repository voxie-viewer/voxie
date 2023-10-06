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
import scipy.ndimage
import numpy as np

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

if args.voxie_action != 'RunFilter':
    raise Exception('Invalid operation: ' + repr(args.voxie_action))

with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationRunFilter']).ClaimOperationAndCatch() as op:
    inputData = op.GetInputData('de.uni_stuttgart.Voxie.Input').CastTo(
        'de.uni_stuttgart.Voxie.VolumeData')
    outputPath = op.Properties['de.uni_stuttgart.Voxie.Output'].getValue('o')

    inputDataVoxel = inputData.CastTo('de.uni_stuttgart.Voxie.VolumeDataVoxel')

    # TODO: Progress during sobel?

    with inputDataVoxel.GetBufferReadonly() as inputArray:
        with op.GetOutputVolumeDataVoxelLike(outputPath, inputDataVoxel, type=('float', 32, 'native')) as data:
            with data.CreateUpdate() as update, data.GetBufferWritable(update) as outputBuffer:

                dir = op.Properties['de.uni_stuttgart.Voxie.Filter.Sobel.Direction'].getValue(
                    's')

                op.SetProgress(0.05)

                # outputBuffer.array[:] = gaussian_filter(inputDataVoxel[:].astype(float), sigma=sig)
                op.ThrowIfCancelled()
                if dir == 'de.uni_stuttgart.Voxie.Filter.Sobel.Direction.Magnitude':
                    # TODO: Reduce memory usage?
                    outputBuffer.array[:] = 0
                    op.ThrowIfCancelled()
                    res = np.zeros(outputBuffer.array.shape,
                                   dtype=outputBuffer.array.dtype)
                    for axis in range(3):
                        op.ThrowIfCancelled()
                        scipy.ndimage.sobel(
                            inputArray.array, axis=axis, output=res, mode='constant', cval=0.0)
                        op.ThrowIfCancelled()
                        res **= 2
                        op.ThrowIfCancelled()
                        outputBuffer.array[:] += res
                        op.ThrowIfCancelled()
                        op.SetProgress(0.05 + 0.87 / 3 * (axis + 1))
                    op.ThrowIfCancelled()
                    outputBuffer.array[:] **= 0.5
                else:
                    if dir == 'de.uni_stuttgart.Voxie.Filter.Sobel.Direction.X':
                        axis = 0
                    elif dir == 'de.uni_stuttgart.Voxie.Filter.Sobel.Direction.Y':
                        axis = 1
                    elif dir == 'de.uni_stuttgart.Voxie.Filter.Sobel.Direction.Z':
                        axis = 2
                    else:
                        raise Exception(
                            'Unknown direction value: ' + repr(dir))
                    scipy.ndimage.sobel(
                        inputArray.array, axis=axis, output=outputBuffer.array, mode='constant', cval=0.0)
                op.ThrowIfCancelled()

                op.SetProgress(0.95)

                version = update.Finish()

                result = {}
                result[outputPath] = {
                    'Data': voxie.Variant('o', data._objectPath),
                    'DataVersion': voxie.Variant('o', version._objectPath),
                }
                op.Finish(result)
                version._referenceCountingObject.destroy()

    context.client.destroy()
