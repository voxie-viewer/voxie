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
import numpy as np


def get_operation_from_string(operation_string):
    if operation_string == 'de.uni_stuttgart.Voxie.Filter.UnaryOperation.Operation.Negate':
        return np.negative
    if operation_string == 'de.uni_stuttgart.Voxie.Filter.UnaryOperation.Operation.LogicalNot':
        return np.logical_not
    raise Exception('Unknown unary operation: ' + repr(operation_string))


args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

if args.voxie_action != 'RunFilter':
    raise Exception('Invalid operation: ' + args.voxie_action)

with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationRunFilter']).ClaimOperationAndCatch() as op:
    inputData = op.GetInputData('de.uni_stuttgart.Voxie.Input').CastTo(
        'de.uni_stuttgart.Voxie.VolumeData')
    outputPath = op.Properties['de.uni_stuttgart.Voxie.Output'].getValue('o')

    inputDataVoxel = inputData.CastTo('de.uni_stuttgart.Voxie.VolumeDataVoxel')

    operation = get_operation_from_string(
        op.Properties['de.uni_stuttgart.Voxie.Filter.UnaryOperation.Operation'].getValue('s'))

    with inputDataVoxel.GetBufferReadonly() as inputArray:
        with op.GetOutputVolumeDataVoxelLike(outputPath, inputDataVoxel) as data:
            with data.CreateUpdate() as update, data.GetBufferWritable(update) as outputBuffer:
                zCount = data[:].shape[2]
                for z in range(0, zCount):
                    op.ThrowIfCancelled()
                    operation(inputArray[:, :, z], outputBuffer.array[:, :, z])
                    op.SetProgress((z + 1) / zCount)
                version = update.Finish()

            result = {}
            result[outputPath] = {
                'Data': voxie.Variant('o', data._objectPath),
                'DataVersion': voxie.Variant('o', version._objectPath),
            }
            op.Finish(result)
            version._referenceCountingObject.destroy()
