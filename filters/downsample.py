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

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

if args.voxie_action != 'RunFilter':
    raise Exception('Invalid operation: ' + repr(args.voxie_action))

with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationRunFilter']).ClaimOperationAndCatch() as op:
    filterPath = op.FilterObject
    pars = op.Parameters
    # print (pars)
    properties = pars[filterPath._objectPath]['Properties'].getValue('a{sv}')
    # print (properties)
    inputPath = properties['de.uni_stuttgart.Voxie.Input'].getValue('o')
    inputDataPath = pars[inputPath]['Data'].getValue('o')
    inputData = context.makeObject(context.bus, context.busName, inputDataPath, [
                                   'de.uni_stuttgart.Voxie.VolumeDataVoxel'])
    outputPath = properties['de.uni_stuttgart.Voxie.Output'].getValue('o')

    factor = properties['de.uni_stuttgart.Voxie.Filter.Downsample.Factor'].getValue(
        'x')

    origin = inputData.VolumeOrigin
    sizeOrig = inputData.ArrayShape
    spacingOrig = np.array(inputData.GridSpacing)
    # print (origin, sizeOrig, spacingOrig)

    # TODO: Don't cut away data at the end

    # size = ((int(sizeOrig[0]) + factor - 1) // factor,
    #        (int(sizeOrig[1]) + factor - 1) // factor,
    #        (int(sizeOrig[2]) + factor - 1) // factor)
    size = (int(sizeOrig[0]) // factor,
            int(sizeOrig[1]) // factor,
            int(sizeOrig[2]) // factor)
    spacing = spacingOrig * factor

    with inputData.GetBufferReadonly() as bufferOld:
        arrayOld = bufferOld.array

        arrayOld2 = arrayOld[:size[0] * factor,
                             :size[1] * factor, :size[2] * factor]

        arrayOld3 = arrayOld2.view()
        arrayOld3.shape = size[0], factor, size[1], factor, size[2], factor

        dataType = ('float', 32, 'native')  # TODO?

        with instance.CreateVolumeDataVoxel(size, dataType, origin, spacing) as data:
            with data.CreateUpdate() as update, data.GetBufferWritable(update) as buffer:
                buffer[:] = 0

                zCount = arrayOld3.shape[4]
                for z in range(zCount):
                    buffer[:, :, z] = np.mean(
                        arrayOld3[:, :, :, :, z, :], axis=(1, 3, 4))
                    op.SetProgress((z + 1) / zCount)
                version = update.Finish()

            result = {}
            result[outputPath] = {
                'Data': voxie.Variant('o', data._objectPath),
                'DataVersion': voxie.Variant('o', version._objectPath),
            }
            op.Finish(result)
