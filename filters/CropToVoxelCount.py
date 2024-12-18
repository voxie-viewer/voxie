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
import sys

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

if args.voxie_action != 'RunFilter':
    raise Exception('Invalid operation: ' + repr(args.voxie_action))

with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationRunFilter']).ClaimOperationAndCatch() as op:
    inputData = op.GetInputData('de.uni_stuttgart.Voxie.Input').CastTo('de.uni_stuttgart.Voxie.VolumeData')
    outputPath = op.Properties['de.uni_stuttgart.Voxie.Output'].getValue('o')

    voxelCount = op.Properties['de.uni_stuttgart.Voxie.Filter.CropToVoxelCount.VoxelCount'].getValue('(ttt)')

    inputDataVoxel = inputData.CastTo('de.uni_stuttgart.Voxie.VolumeDataVoxel')

    for i in range(3):
        if inputDataVoxel.ArrayShape[i] < voxelCount[i]:
            raise Exception('Size of input data {} smaller than specified output data {}'.format(inputDataVoxel.ArrayShape[i], voxelCount[i]))

    with instance.CreateVolumeDataVoxel(voxelCount, inputDataVoxel.DataType, inputDataVoxel.VolumeOrigin, inputDataVoxel.GridSpacing) as data:
        with data.CreateUpdate() as update, data.GetBufferWritable(update) as outputBuffer:
            zCount = data[:].shape[2]
            for z in range(0, zCount):
                op.ThrowIfCancelled()
                outputBuffer.array[:, :, z] = inputDataVoxel[:voxelCount[0], :voxelCount[1], z]
                op.SetProgress((z + 1) / zCount)
            version = update.Finish()

        result = {}
        result[outputPath] = {
            'Data': voxie.Variant('o', data._objectPath),
            'DataVersion': voxie.Variant('o', version._objectPath),
        }
        op.Finish(result)
        version._referenceCountingObject.destroy()
