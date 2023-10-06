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
    inputData = op.GetInputData('de.uni_stuttgart.Voxie.Input').CastTo('de.uni_stuttgart.Voxie.ContainerData')

    outputPath = op.Properties['de.uni_stuttgart.Voxie.Output'].getValue('o')

    num = op.Properties['de.uni_stuttgart.Voxie.Filter.ExtractVolumeFromSet.SelectedVolume'].getValue('x')
    name = 'Volume_{}'.format(num)

    try:
        volume = inputData.GetElement(name)
    except dbus.exceptions.DBusException as e:
        # TODO: Rename error?
        if e._dbus_error_name != 'de.uni_stuttgart.Voxie.InvalidOperation':
            raise
        keys = inputData.GetKeys()
        nrs = []
        for key in keys:
            if not key.startswith('Volume_'):
                continue
            key2 = key[len('Volume_'):]
            try:
                i = int(key2)
            except Exception:
                continue
            if 'Volume_{}'.format(i) != key:
                continue
            nrs.append(i)
        nrs = sorted(nrs)
        raise Exception('Selected invalid volume {}, valid ones are: {}'.format(num, nrs))

    volume_version = volume.GetCurrentVersion()

    result = {}
    result[outputPath] = {
        'Data': voxie.Variant('o', volume._objectPath),
        'DataVersion': voxie.Variant('o', volume_version._objectPath),
    }
    op.Finish(result)

    volume_version._referenceCountingObject.destroy()

context.client.destroy()
