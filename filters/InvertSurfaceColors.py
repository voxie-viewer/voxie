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
    properties = pars[filterPath._objectPath]['Properties'].getValue('a{sv}')
    inputPath = properties['de.uni_stuttgart.Voxie.Input'].getValue('o')
    inputDataPath = pars[inputPath]['Data'].getValue('o')
    inputData = context.makeObject(context.bus, context.busName, inputDataPath, [
                                   'de.uni_stuttgart.Voxie.SurfaceData'])
    outputPath = properties['de.uni_stuttgart.Voxie.Output'].getValue('o')

    inputSurface = voxie.cast(
        inputData, 'de.uni_stuttgart.Voxie.SurfaceDataTriangleIndexed')

    attributes = inputSurface.Attributes
    print(attributes)

    outputSurface = instance.CreateSurfaceDataTriangleIndexed(inputSurface.TriangleCount, inputSurface.VertexCount, inputSurface, False, {
                                                              'Attributes': voxie.Variant('a(sst(sus)sa{sv}a{sv})', attributes)})
    with outputSurface.CreateUpdate() as update:
        voxie.Buffer(outputSurface.GetVerticesWritable(update), 2, True)[
            :] = voxie.Buffer(inputSurface.GetVerticesReadonly(), 2, False)
        for attr in attributes:
            name = attr[0]
            out = voxie.Buffer(
                outputSurface.GetAttributeWritable(update, name), 2, True)
            out[:] = voxie.Buffer(
                inputSurface.GetAttributeReadonly(name), 2, False)
            if name == 'de.uni_stuttgart.Voxie.SurfaceAttribute.Color' or name == 'de.uni_stuttgart.Voxie.SurfaceAttribute.ColorBackside':
                out[:, :3] = 1 - out[:, :3]  # Invert everything except alpha
            del out

        version = update.Finish()

    result = {}
    result[outputPath] = {
        'Data': voxie.Variant('o', outputSurface._objectPath),
        'DataVersion': voxie.Variant('o', version._objectPath),
    }
    op.Finish(result)
