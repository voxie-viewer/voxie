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
    raise Exception('Invalid operation: ' + args.voxie_action)

with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationRunFilter']).ClaimOperationAndCatch() as op:
    filterPath = op.FilterObject
    pars = op.Parameters
    # print (pars)
    properties = pars[filterPath._objectPath]['Properties'].getValue('a{sv}')
    # print (properties)
    outputPath = properties['de.uni_stuttgart.Voxie.Output'].getValue('o')
    size = properties['de.uni_stuttgart.Voxie.Filter.CreateSheppLogan3D.Size'].getValue(
        'x')
    scale = properties['de.uni_stuttgart.Voxie.Filter.CreateSheppLogan3D.Scale'].getValue(
        'd')

    volSize = (0.1, 0.1, 0.1)
    gridOrigin = (-0.05, -0.05, -0.05)
    gridSpacing = (volSize[0] / size, volSize[1] / size, volSize[2] / size)
    vol = instance.CreateVolumeDataVoxel(
        (size, size, size), ('float', 32, 'native'), gridOrigin, gridSpacing)
    with vol.CreateUpdate() as update:
        buffer = vol.GetBufferWritable(update)
        buffer[:] = 0

        shepp_params = tomopy_misc_phantom._array_to_params(
            tomopy_misc_phantom._get_shepp_array())
        coords = tomopy_misc_phantom._define_coords((size, size, size))

        # recursively add ellipsoids to cube
        i = 0
        op.SetProgress((1) / (len(shepp_params) + 2))
        for param in shepp_params:
            # print('X', flush=True)
            tomopy_misc_phantom._ellipsoid(param, out=buffer[:], coords=coords)
            op.SetProgress((i + 2) / (len(shepp_params) + 2))
            i = i + 1
            # print('Y', flush=True)

        buffer[:] *= scale
        op.SetProgress((len(shepp_params) + 2) / (len(shepp_params) + 2))

        version = update.Finish()

    result = {}
    result[outputPath] = {
        'Data': voxie.Variant('o', vol._objectPath),
        'DataVersion': voxie.Variant('o', version._objectPath),
    }
    op.Finish(result)
