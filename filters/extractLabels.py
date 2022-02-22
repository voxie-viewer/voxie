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

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

if args.voxie_action != 'RunFilter':
    raise Exception('Invalid operation: ' + args.voxie_action)

with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationRunFilter']).ClaimOperationAndCatch() as op:

    try:
        compoundData = op.GetInputData('de.uni_stuttgart.Voxie.ContainerNode').CastTo(
            'de.uni_stuttgart.Voxie.ContainerData')
    except Exception:
        raise Exception(
            "A Container object with a LabelVolume inside needs to be connected!")

    try:
        inputVolume = op.GetInputData('de.uni_stuttgart.Voxie.Input').CastTo(
            'de.uni_stuttgart.Voxie.VolumeDataVoxel')
    except Exception:
        raise Exception("An input volume needs to be connected!")

    outputPathVolume = op.Properties['de.uni_stuttgart.Voxie.Output'].getValue(
        'o')

    labelVolume = compoundData.GetElement("labelVolume").CastTo(
        'de.uni_stuttgart.Voxie.VolumeDataVoxel')
    labelTable = compoundData.GetElement("labelTable").CastTo(
        'de.uni_stuttgart.Voxie.TableData')

    labelIDs = op.Properties["de.uni_stuttgart.Voxie.LabelIDs"].getValue(
        'at')

    shapeInput = inputVolume.GetBufferReadonly().array.shape
    shapeLabelVolume = labelVolume.GetBufferReadonly().array.shape

    if (shapeInput == shapeLabelVolume):

        with op.GetOutputVolumeDataVoxelLike(outputPathVolume, inputVolume) as data:
            with data.CreateUpdate() as update, data.GetBufferWritable(update) as outputBuffer:

                outputBuffer.array[np.isin(
                    labelVolume, labelIDs)] = inputVolume[np.isin(labelVolume, labelIDs)]

                version = update.Finish()

                result = {}
                result[outputPathVolume] = {
                    'Data': voxie.Variant('o', data._objectPath),
                    'DataVersion': voxie.Variant('o', version._objectPath),
                }
                op.Finish(result)
                version._referenceCountingObject.destroy()

    else:
        raise Exception('Input Volume shape: {} and LabelVolume shape {} must be equal.'.format(
            shapeInput, shapeLabelVolume))

context.client.destroy()
