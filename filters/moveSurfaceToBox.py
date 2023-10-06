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

    box = properties['de.uni_stuttgart.Voxie.Filter.MoveSurfaceToBox.Box'].getValue('((ddd)(ddd))')
    doStretch = properties['de.uni_stuttgart.Voxie.Filter.MoveSurfaceToBox.Stretch'].getValue('b')

    # TODO
    if doStretch:
        raise Exception('Stretching not implemented')

    # TODO: Center object by default instead of putting it to the lower left corner?

    triangles = voxie.Buffer(srfT.GetTrianglesReadonly(), 2, False)
    vertices = voxie.Buffer(srfT.GetVerticesReadonly(), 2, False)

    min_val = np.min(vertices, axis=0)
    max_val = np.max(vertices, axis=0)
    # print(min_val, max_val)

    box_min = np.asarray(box[0])
    box_size = np.asarray(box[1]) - box[0]
    scale = np.min(box_size / (max_val - min_val))

    srf2 = instance.CreateSurfaceDataTriangleIndexed(
        triangles[:].shape[0], vertices[:].shape[0], srfT, False)
    with srf2.CreateUpdate() as update:
        vertices_out = voxie.Buffer(srf2.GetVerticesWritable(update), 2, True)
        vertices_out[:] = (vertices - min_val) * scale + box_min
        version = update.Finish()

    result = {}
    result[outputPath] = {
        'Data': voxie.Variant('o', srf2._objectPath),
        'DataVersion': voxie.Variant('o', version._objectPath),
    }
    op.Finish(result)
