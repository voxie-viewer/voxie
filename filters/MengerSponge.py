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


# Returns list of values where the bits show in which iteration the value is removed from the set
def create_cantor_function(n):
    data = np.array([0], dtype=np.uint32)
    for i in range(n):
        data = np.concatenate([data * 2, data * 2 + 1, data * 2])
    return data


args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

if args.voxie_action != 'RunFilter':
    raise Exception('Invalid operation: ' + repr(args.voxie_action))

with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationRunFilter']).ClaimOperationAndCatch() as op:
    output_path = op.Properties['de.uni_stuttgart.Voxie.Output'].getValue('o')

    n = op.Properties['de.uni_stuttgart.Voxie.Filter.MengerSponge.Iterations'].getValue('x')
    count = 3 ** n
    spacing = 1 / count
    origin = -0.5
    value = (27 / 20) ** n

    cantor_1d = create_cantor_function(n)
    # The Sierpiński carpet is where no bit is set in cantor_2d_and
    cantor_2d_and = cantor_1d[:, np.newaxis] & cantor_1d[np.newaxis, :]
    cantor_2d_or = cantor_1d[:, np.newaxis] | cantor_1d[np.newaxis, :]
    # The Menger sponge is where no bit is set in at least two out of the three dimensions.

    with instance.CreateVolumeDataVoxel((count, count, count), ('float', 32, 'native'), (origin, origin, origin), (spacing, spacing, spacing)) as data:
        with data.CreateUpdate() as update:
            with data.GetBufferWritable(update) as buffer:
                for z in range(0, count):
                    op.ThrowIfCancelled()
                    buffer[:, :, z] = ((cantor_2d_and | cantor_2d_or & cantor_1d[z]) == 0) * value
                    op.SetProgress((z + 1) / count)
            with update.Finish() as version:
                result = {}
                result[output_path] = {
                    'Data': voxie.Variant('o', data._objectPath),
                    'DataVersion': voxie.Variant('o', version._objectPath),
                }
                op.Finish(result)

    instance._context.client.destroy()
