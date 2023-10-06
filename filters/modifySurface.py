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
                                   'de.uni_stuttgart.Voxie.SurfaceData'])
    outputPath = properties['de.uni_stuttgart.Voxie.Output'].getValue('o')

    srfT = voxie.cast(
        inputData, 'de.uni_stuttgart.Voxie.SurfaceDataTriangleIndexed')

    triangles = voxie.Buffer(srfT.GetTrianglesReadonly(), 2, False)
    vertices = voxie.Buffer(srfT.GetVerticesReadonly(), 2, False)

    # print (triangles[:].shape)
    # print (triangles[:])
    # print (vertices[:].shape)
    # print (vertices[:])

    # with srfT.CreateUpdate() as update:
    #    verticesW = voxie.Buffer(srfT.GetVerticesWritable(update), 2, True)
    #    verticesW[:] = -verticesW[:]
    #    update.Finish()

    triangles2 = triangles[:triangles[:].shape[0] // 2, :]
    # vertices2 = -vertices[:]
    vertices2 = vertices[:] + 0.02

    srf2t = instance.CreateSurfaceDataTriangleIndexed(
        triangles2.shape[0], 0, None, True)
    with srf2t.CreateUpdate() as update:
        voxie.Buffer(srf2t.GetTrianglesWritable(
            update), 2, True)[:] = triangles2
        update.Finish()

    srf2 = instance.CreateSurfaceDataTriangleIndexed(
        triangles2.shape[0], vertices2.shape[0], srf2t, False)
    with srf2.CreateUpdate() as update:
        voxie.Buffer(srf2.GetVerticesWritable(update), 2, True)[:] = vertices2
        version = update.Finish()

    # srf2Obj = instance.GetPrototype('de.uni_stuttgart.Voxie.SurfaceObject').CreateObject({}, {'Data': voxie.Variant('o', srf2), 'ManualDisplayName': voxie.Variant('(bs)', (True, 'Modified ' + srfObjName))})

    result = {}
    result[outputPath] = {
        'Data': voxie.Variant('o', srf2._objectPath),
        'DataVersion': voxie.Variant('o', version._objectPath),
    }
    op.Finish(result)
