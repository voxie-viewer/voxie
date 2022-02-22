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

__author__ = "Alex Zeising"

import dbus

import numpy as np

import voxie

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()


def mse(arr1: np.ndarray, arr2: np.ndarray) -> float:
    return float(np.mean((arr1 - arr2) ** 2))


if args.voxie_action != 'RunFilter':
    raise Exception('Invalid operation: ' + args.voxie_action)


VOXIE_RUN_FILTER = 'de.uni_stuttgart.Voxie.ExternalOperationRunFilter'
FILTER_NAME = 'de.uni_stuttgart.Voxie.Filter.MeanSquaredError'
VOLUME_1 = FILTER_NAME + '.V1'
VOLUME_2 = FILTER_NAME + '.V2'
VOLUME_DATA_VOXEL = 'de.uni_stuttgart.Voxie.VolumeDataVoxel'


with context.makeObject(context.bus, context.busName, args.voxie_operation,
                        [VOXIE_RUN_FILTER]).ClaimOperationAndCatch() as op:
    # check if both inputs are available
    inputPath1 = op.Properties[VOLUME_1].getValue(
        'o')
    if inputPath1 == dbus.ObjectPath('/'):
        raise Exception('No input for volume 1 connected')
    inputPath2 = op.Properties[VOLUME_2].getValue(
        'o')
    if inputPath2 == dbus.ObjectPath('/'):
        raise Exception('No input for volume 2 connected')

    input1 = op.GetInputData(VOLUME_1).CastTo(VOLUME_DATA_VOXEL)
    input2 = op.GetInputData(VOLUME_2).CastTo(VOLUME_DATA_VOXEL)

    with input1.GetBufferReadonly() as b_v1:
        with input2.GetBufferReadonly() as b_v2:
            v1 = b_v1[:]
            v2 = b_v2[:]

            if v1.shape != v2.shape:
                raise ValueError('Volumes need to be of equal dimensions!')

            print(f'MSE: {mse(v1, v2)}')

            op.Finish({})
            instance._context.client.destroy()
