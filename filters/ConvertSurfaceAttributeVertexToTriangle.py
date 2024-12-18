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
    attribute_name = properties['de.uni_stuttgart.Voxie.Filter.ConvertSurfaceAttributeVertexToTriangle.AttributeName'].getValue('s')

    srfT = voxie.cast(inputData, 'de.uni_stuttgart.Voxie.SurfaceDataTriangleIndexed')

    attribute_list = srfT.Attributes

    found = False
    result_attribute_list = []
    for attribute in attribute_list:
        name = attribute[0]
        if name != attribute_name:
            result_attribute_list.append(attribute)
            continue
        kind = attribute[1]
        if kind != 'de.uni_stuttgart.Voxie.SurfaceAttributeKind.Vertex':
            raise Exception('Input attribute is not a vertex attribute')
        attribute = attribute[0:1] + ('de.uni_stuttgart.Voxie.SurfaceAttributeKind.Triangle',) + attribute[2:]
        result_attribute_list.append(attribute)
        found = True
    if not found:
        raise Exception('Cannot find attribute {!r}'.format(attribute_name))

    triangles = voxie.Buffer(srfT.GetTrianglesReadonly(), 2, False)
    vertices = voxie.Buffer(srfT.GetVerticesReadonly(), 2, False)

    srf2 = instance.CreateSurfaceDataTriangleIndexed(triangles[:].shape[0], vertices[:].shape[0], srfT, False, {'Attributes': voxie.Variant('a(sst(sus)sa{sv}a{sv})', result_attribute_list)})
    with srf2.CreateUpdate() as update:
        voxie.Buffer(srf2.GetVerticesWritable(update), 2, True)[:] = vertices[:]

        for attribute in attribute_list:
            name = attribute[0]
            buffer_old = voxie.Buffer(srfT.GetAttributeReadonly(name), 2, False)
            buffer_new = voxie.Buffer(srf2.GetAttributeWritable(update, name), 2, True)
            if name != attribute_name:
                buffer_new[:] = buffer_old[:]
                continue
            # TODO: Progress bar, do in steps?
            # buffer_new[:] = 0.42
            # Get values for each of the corners of the triangle
            value_0 = buffer_old[triangles[:, 0], :]
            value_1 = buffer_old[triangles[:, 1], :]
            value_2 = buffer_old[triangles[:, 2], :]
            # print(buffer_old[:].shape, buffer_new[:].shape, value_1.shape)
            print(np.min(buffer_old), np.max(buffer_old))
            print(np.min(value_1), np.max(value_1))
            # Calculate the average
            buffer_new[:] = (value_0 + value_1 + value_2) / 3

        version = update.Finish()

    result = {}
    result[outputPath] = {
        'Data': voxie.Variant('o', srf2._objectPath),
        'DataVersion': voxie.Variant('o', version._objectPath),
    }
    op.Finish(result)
