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
    inputPath = properties['de.uni_stuttgart.Voxie.Input'].getValue('o')
    if inputPath == dbus.ObjectPath('/'):
        raise Exception('No input volume object connected')
    inputProperties = pars[inputPath]['Properties'].getValue('a{sv}')
    inputDataPath = pars[inputPath]['Data'].getValue('o')
    inputData = context.makeObject(context.bus, context.busName, inputDataPath, [
                                   'de.uni_stuttgart.Voxie.VolumeData'])
    outputPath = properties['de.uni_stuttgart.Voxie.Output'].getValue('o')
    # print(inputProperties["de.uni_stuttgart.Voxie.MovableDataNode.Translation"].getValue("(ddd)"))
    # print(inputProperties["de.uni_stuttgart.Voxie.MovableDataNode.Rotation"].getValue("(dddd)"))

    translationVolume = np.array(
        inputProperties["de.uni_stuttgart.Voxie.MovableDataNode.Translation"].getValue("(ddd)"))
    rotationVolume = voxie.Rotation(
        inputProperties["de.uni_stuttgart.Voxie.MovableDataNode.Rotation"].getValue("(dddd)"))

    origin = inputData.VolumeOrigin
    size = inputData.VolumeSize

    pointType = instance.Components.GetComponent(
        'de.uni_stuttgart.Voxie.ComponentType.GeometricPrimitiveType', 'de.uni_stuttgart.Voxie.GeometricPrimitive.Point').CastTo('de.uni_stuttgart.Voxie.GeometricPrimitiveType')
    with instance.CreateGeometricPrimitiveData() as data:
        with data.CreateUpdate() as update:
            for corner in [[0, 0, 0], [0, 0, 1], [0, 1, 0], [
                    0, 1, 1], [1, 0, 0], [1, 0, 1], [1, 1, 0], [1, 1, 1]]:
                cpos = origin + size * np.array(corner, dtype=np.double)
                cpos = rotationVolume * cpos + translationVolume
                # print (cpos)
                data.AddPrimitive(update, pointType, 'Point %s' % (corner,), {
                                  'Position': voxie.Variant('(ddd)', tuple(map(float, cpos)))})

            with update.Finish() as version:
                result = {}
                result[outputPath] = {
                    'Data': voxie.Variant('o', data._objectPath),
                    'DataVersion': voxie.Variant('o', version._objectPath),
                }
                op.Finish(result)
