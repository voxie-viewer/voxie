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


import scipy as sp
import numpy as np
import scipy.ndimage as spim
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

    inputDataVoxel = inputData.CastTo('de.uni_stuttgart.Voxie.VolumeDataVoxel')

    xDimension = op.Properties['de.uni_stuttgart.Voxie.Filter.ChordFilter.xDimension'].getValue(
        'b')
    yDimension = op.Properties['de.uni_stuttgart.Voxie.Filter.ChordFilter.yDimension'].getValue(
        'b')
    zDimension = op.Properties['de.uni_stuttgart.Voxie.Filter.ChordFilter.zDimension'].getValue(
        'b')

    with inputDataVoxel.GetBufferReadonly() as inputArray:
        with op.GetOutputVolumeDataVoxelLike(outputPath, inputDataVoxel, type=('uint', 8, 'native')) as data:
            with data.CreateUpdate() as update, data.GetBufferWritable(update) as outputBuffer:

                spacing = op.Properties['de.uni_stuttgart.Voxie.Filter.ChordFilter.Spacing'].getValue(
                    'x') * 2

                outputBuffer.array[:] = sp.zeros_like(
                    inputDataVoxel[:].shape[0], dtype=int)

                if xDimension:
                    outputBuffer.array[:, ::4 + spacing, ::4 + spacing] = 1
                if yDimension:
                    outputBuffer.array[::4 + spacing, :, 2::4 + spacing] = 2
                if zDimension:
                    outputBuffer.array[2::4 + spacing, 2::4 + spacing, :] = 3

                outputBuffer.array[:] *= inputDataVoxel[:]

                version = update.Finish()

                result = {}
                result[outputPath] = {
                    'Data': voxie.Variant('o', data._objectPath),
                    'DataVersion': voxie.Variant('o', version._objectPath),
                }
                op.Finish(result)
                version._referenceCountingObject.destroy()

    context.client.destroy()