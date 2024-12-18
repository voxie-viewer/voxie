#!/usr/bin/env python3

# Note: This uses '#!/usr/bin/env python3' instead of '#!/usr/bin/python3' to take venvs into account

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

# Note: This requires CadQuery available somewhere

import numpy as np
import voxie
import tempfile

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
    outputPath = properties['de.uni_stuttgart.Voxie.Output'].getValue('o')
    filename = properties['de.uni_stuttgart.Voxie.Filter.CreateSurfaceUsingCadQuery.ScriptFileName'].getValue('s')
    tolerance = properties['de.uni_stuttgart.Voxie.Filter.CreateSurfaceUsingCadQuery.Tolerance'].getValue('d')
    angular_tolerance = properties['de.uni_stuttgart.Voxie.Filter.CreateSurfaceUsingCadQuery.AngularTolerance'].getValue('d')
    unit = properties['de.uni_stuttgart.Voxie.Filter.CreateSurfaceUsingCadQuery.Unit'].getValue('d')

    op.ThrowIfCancelled()
    op.SetProgress(0.1)

    with open(filename, 'r', encoding='utf-8') as file:
        script_data = file.read()

    op.ThrowIfCancelled()
    op.SetProgress(0.2)

    import cadquery.cqgi

    op.ThrowIfCancelled()
    op.SetProgress(0.3)

    script = cadquery.cqgi.parse(script_data)

    op.ThrowIfCancelled()
    op.SetProgress(0.4)

    build_result = script.build()

    op.ThrowIfCancelled()
    op.SetProgress(0.7)

    if not build_result.success:
        raise Exception('CadQuery build failed')  # TODO: Message
    # print(dir(build_result))
    if len(build_result.results) == 0:
        raise Exception('Did not get any result from CadQuery')
    if len(build_result.results) > 1:
        raise Exception('Got multiple results from CadQuery')
    cq_surface = build_result.results[0]
    with tempfile.TemporaryDirectory(prefix='cq.') as tmp:
        fn = tmp + '/surface.stl'
        cadquery.exporters.export(cq_surface.shape, fn)

        op.SetProgress(0.8)

        data = instance.Import(fn)

    op.SetProgress(0.9)

    data = data.CastTo('de.uni_stuttgart.Voxie.SurfaceDataTriangleIndexed')

    with data.CreateUpdate() as update:
        voxie.Buffer(data.GetVerticesWritable(update), 2, True)[:] *= unit

    with data.GetCurrentVersion() as version:
        result = {}
        result[outputPath] = {
            'Data': voxie.Variant('o', data._objectPath),
            'DataVersion': voxie.Variant('o', version._objectPath),
        }
        op.Finish(result)
