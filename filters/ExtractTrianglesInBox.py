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
    raise Exception('Invalid operation: ' + repr(args.voxie_action))

with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationRunFilter']).ClaimOperationAndCatch() as op:
    inputData = op.GetInputData('de.uni_stuttgart.Voxie.Input').CastTo('de.uni_stuttgart.Voxie.SurfaceDataTriangleIndexed')
    outputPath = op.Properties['de.uni_stuttgart.Voxie.Output'].getValue('o')
    bbData = op.GetInputData('de.uni_stuttgart.Voxie.BoundingBoxData').CastTo('de.uni_stuttgart.Voxie.GeometricPrimitiveData')

    # TODO: rotation etc. of input?

    # TODO: option to also extract triangles partially in the box?

    # TODO: option to close the resulting surface at the box?

    pointType = instance.Components.GetComponent(
        'de.uni_stuttgart.Voxie.ComponentType.GeometricPrimitiveType', 'de.uni_stuttgart.Voxie.GeometricPrimitive.Point').CastTo('de.uni_stuttgart.Voxie.GeometricPrimitiveType')
    points = []
    for primitive in bbData.GetPrimitives(0, 2**64 - 1):
        type = primitive[1]
        primitiveValues = primitive[3]
        if type != pointType._objectPath:
            print('Warning: Unknown primitive:', type, file=sys.stderr)
            continue
        position = primitiveValues['Position'].getValue('(ddd)')
        points.append(np.array(position))

    posmin = posmax = None
    if len(points) == 0:
        raise Exception('Got a bounding box input but no points in it')
    for point in points:
        # cpos = rotationPlane.inverse * (point - translationVolume)
        cpos = point
        if posmin is None:
            posmin = cpos
        if posmax is None:
            posmax = cpos
        posmin = np.minimum(posmin, cpos)
        posmax = np.maximum(posmax, cpos)
        # print (cpos)
    # print (posmin)
    # print (posmax)

    triangles = voxie.Buffer(inputData.GetTrianglesReadonly(), 2, False)
    vertices = voxie.Buffer(inputData.GetVerticesReadonly(), 2, False)

    # TODO: Also copy attributes?

    verticesIsInBox = np.min((vertices >= posmin) * (vertices <= posmax), axis=1)
    verticesIdx = np.nonzero(verticesIsInBox)[0]
    verticesMap = np.full(verticesIsInBox.shape[0], -1, dtype=np.uint32)
    verticesMap[verticesIdx] = np.arange(verticesIdx.shape[0], dtype=np.uint32)
    verticesOut = vertices[verticesIdx]

    print(len(verticesIsInBox))
    print(verticesIdx)

    trianglesIsInBox = np.min(verticesIsInBox[triangles], axis=1)
    print(len(trianglesIsInBox))
    print(np.count_nonzero(trianglesIsInBox))
    trianglesOut = triangles[trianglesIsInBox]
    print(trianglesOut.shape)

    trianglesOut = verticesMap[trianglesOut]
    print(trianglesOut.shape)

    # TODO: Remove vertices

    srf1 = instance.CreateSurfaceDataTriangleIndexed(trianglesOut[:].shape[0], 0, None, True)
    with srf1.CreateUpdate() as update:
        triangles_out = voxie.Buffer(srf1.GetTrianglesWritable(update), 2, True)
        triangles_out[:] = trianglesOut

    srf2 = instance.CreateSurfaceDataTriangleIndexed(trianglesOut[:].shape[0], verticesOut[:].shape[0], srf1, False)
    with srf2.CreateUpdate() as update:
        vertices_out = voxie.Buffer(srf2.GetVerticesWritable(update), 2, True)
        vertices_out[:] = verticesOut
        version = update.Finish()

    result = {}
    result[outputPath] = {
        'Data': voxie.Variant('o', srf2._objectPath),
        'DataVersion': voxie.Variant('o', version._objectPath),
    }
    op.Finish(result)
