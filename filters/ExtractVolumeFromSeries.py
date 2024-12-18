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
import dbus

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

if args.voxie_action != 'RunFilter':
    raise Exception('Invalid operation: ' + repr(args.voxie_action))

with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationRunFilter']).ClaimOperationAndCatch() as op:
    inputData = op.GetInputData('de.uni_stuttgart.Voxie.Input').CastTo('de.uni_stuttgart.Voxie.VolumeSeriesData')

    isAutomaticFilterRun = op.IsAutomaticFilterRun

    outputPath = op.Properties['de.uni_stuttgart.Voxie.Output'].getValue('o')

    key_0 = op.Properties['de.uni_stuttgart.Voxie.Filter.ExtractVolumeFromSeries.Key'].getValue('x')
    key = (key_0,)

    volume = None
    if inputData.DimensionCount != 1:
        if not isAutomaticFilterRun:
            raise Exception('This filter only works for 1D series data')
        else:
            print('This filter only works for 1D series data')
    else:
        try:
            volume = inputData.LookupEntry(key)
        except Exception as e:
            if not isAutomaticFilterRun:
                raise
            volume = None
        if volume is None:
            length = inputData.Dimensions[0].Length
            if not isAutomaticFilterRun:
                raise Exception('Did not find volume {}, length is {}'.format(key_0, length))
            else:
                print('Did not find volume {}, length is {}'.format(key_0, length))

    result = {}
    if volume is not None:
        volume_version = volume.GetCurrentVersion()
        result[outputPath] = {
            'Data': voxie.Variant('o', volume._objectPath),
            'DataVersion': voxie.Variant('o', volume_version._objectPath),
        }
    op.Finish(result)

    if volume is not None:
        volume_version._referenceCountingObject.destroy()

context.client.destroy()
