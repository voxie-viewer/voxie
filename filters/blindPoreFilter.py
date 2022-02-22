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
from skimage.segmentation import clear_border
import numpy as np
import voxie

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

if args.voxie_action != 'RunFilter':
    raise Exception('Invalid operation: ' + args.voxie_action)

with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationRunFilter']).ClaimOperationAndCatch() as op:
    inputData = op.GetInputData('de.uni_stuttgart.Voxie.Input').CastTo(
        'de.uni_stuttgart.Voxie.VolumeData')
    outputPath = op.Properties['de.uni_stuttgart.Voxie.Output'].getValue('o')
    method = op.Properties['de.uni_stuttgart.Voxie.Filter.BlindPoreFilter.Mode'].getValue(
        's')

    inputDataVoxel = inputData.CastTo('de.uni_stuttgart.Voxie.VolumeDataVoxel')

    with inputDataVoxel.GetBufferReadonly() as inputArray:
        with op.GetOutputVolumeDataVoxelLike(outputPath, inputDataVoxel, type=None) as data:
            with data.CreateUpdate() as update, data.GetBufferWritable(update) as outputBuffer:

                if method == 'de.uni_stuttgart.Voxie.Filter.BlindPoreFilter.Mode.Extract':
                    outputBuffer.array[:] = clear_border(labels=inputArray[:])
                elif method == 'de.uni_stuttgart.Voxie.Filter.BlindPoreFilter.Mode.Remove':
                    blind_pores = clear_border(labels=inputArray[:])
                    outputBuffer.array[:] = np.logical_xor(
                        blind_pores, inputArray[:])
                else:
                    raise Exception('Unknown mode: ' + repr(method))

                version = update.Finish()

            result = {}
            result[outputPath] = {
                'Data': voxie.Variant('o', data._objectPath),
                'DataVersion': voxie.Variant('o', version._objectPath),
            }
            op.Finish(result)
            version._referenceCountingObject.destroy()

context.client.destroy()
