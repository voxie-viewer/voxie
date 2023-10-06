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

import tomopy_misc_phantom

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
    outputPathX = properties['de.uni_stuttgart.Voxie.Example.Filter.CreateExampleShiftVolumes.OutputX'].getValue('o')
    outputPathY = properties['de.uni_stuttgart.Voxie.Example.Filter.CreateExampleShiftVolumes.OutputY'].getValue('o')
    outputPathZ = properties['de.uni_stuttgart.Voxie.Example.Filter.CreateExampleShiftVolumes.OutputZ'].getValue('o')

    voxelCount = (129, 129, 129)
    gridSpacing = (1e-3, 1e-3, 1e-3)
    gridOrigin = tuple(-np.array(voxelCount) * gridSpacing / 2)
    volX = instance.CreateVolumeDataVoxel(
        voxelCount, ('float', 32, 'native'), gridOrigin, gridSpacing)
    volY = instance.CreateVolumeDataVoxel(
        voxelCount, ('float', 32, 'native'), gridOrigin, gridSpacing)
    volZ = instance.CreateVolumeDataVoxel(
        voxelCount, ('float', 32, 'native'), gridOrigin, gridSpacing)
    with volX.CreateUpdate() as updateX, volY.CreateUpdate() as updateY, volZ.CreateUpdate() as updateZ:
        bufferX = volX.GetBufferWritable(updateX)
        bufferY = volY.GetBufferWritable(updateY)
        bufferZ = volZ.GetBufferWritable(updateZ)
        bufferX[:] = 0
        bufferY[:] = 0
        bufferZ[:] = 0

        bufferX[30:80, 30:80, 30:80] = 1.2 * gridSpacing[0]
        # change here to affect shifts in specific directions
        # bufferY[60:70, 60:70, 60:70] = 1 * gridSpacing[1]
        # bufferZ[30:80, 60:70, 60:70] = 1 * gridSpacing[2]

        versionX = updateX.Finish()
        versionY = updateY.Finish()
        versionZ = updateZ.Finish()

    result = {}
    result[outputPathX] = {
        'Data': voxie.Variant('o', volX._objectPath),
        'DataVersion': voxie.Variant('o', versionX._objectPath),
    }
    result[outputPathY] = {
        'Data': voxie.Variant('o', volY._objectPath),
        'DataVersion': voxie.Variant('o', versionY._objectPath),
    }
    result[outputPathZ] = {
        'Data': voxie.Variant('o', volZ._objectPath),
        'DataVersion': voxie.Variant('o', versionZ._objectPath),
    }
    op.Finish(result)
