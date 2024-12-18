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
import math

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
    outputPath = properties['de.uni_stuttgart.Voxie.Output'].getValue('o')
    radius = properties['de.uni_stuttgart.Voxie.Filter.CreateSphereVolume.Radius'].getValue('d')
    sphere_value = properties['de.uni_stuttgart.Voxie.Filter.CreateSphereVolume.SphereValue'].getValue('d')
    grid_spacing = properties['de.uni_stuttgart.Voxie.Filter.CreateSphereVolume.GridSpacing'].getValue('d')

    voxel_count = math.ceil(radius * 2 * 1.1 / grid_spacing)
    vol_size = voxel_count * grid_spacing
    grid_origin = -vol_size / 2
    vol = instance.CreateVolumeDataVoxel((voxel_count, voxel_count, voxel_count), ('float', 32, 'native'), (grid_origin, grid_origin, grid_origin), (grid_spacing, grid_spacing, grid_spacing))
    with vol.CreateUpdate() as update:
        buffer = vol.GetBufferWritable(update)

        x_pos_2 = (np.linspace(0, voxel_count - 1, voxel_count)[:, np.newaxis] * grid_spacing + grid_origin) ** 2
        y_pos_2 = (np.linspace(0, voxel_count - 1, voxel_count)[np.newaxis, :] * grid_spacing + grid_origin) ** 2
        radius_2 = radius ** 2

        zCount = buffer[:].shape[2]
        for z in range(0, zCount):
            op.ThrowIfCancelled()

            z_pos_2 = (z * grid_spacing + grid_origin) ** 2
            buffer.array[:, :, z] = (x_pos_2 + y_pos_2 + z_pos_2 <= radius_2) * sphere_value

            op.SetProgress((z + 1) / zCount)

        version = update.Finish()

    result = {}
    result[outputPath] = {
        'Data': voxie.Variant('o', vol._objectPath),
        'DataVersion': voxie.Variant('o', version._objectPath),
    }
    op.Finish(result)
