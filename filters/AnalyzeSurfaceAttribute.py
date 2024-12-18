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
    output_path = properties['de.uni_stuttgart.Voxie.TextOutput'].getValue('o')
    attribute_name = properties['de.uni_stuttgart.Voxie.Filter.AnalyzeSurfaceAttribute.AttributeName'].getValue('s')

    srfT = voxie.cast(inputData, 'de.uni_stuttgart.Voxie.SurfaceDataTriangleIndexed')

    attribute_list = srfT.Attributes

    attr = None
    for attribute in attribute_list:
        name = attribute[0]
        if name != attribute_name:
            continue
        if attr is not None:
            raise Exception('attr is not None')
        attr = attribute
    if attr is None:
        raise Exception('Cannot find attribute {!r}'.format(attribute_name))

    result_text = ''

    result_text += 'Attribute name: {!r}\n'.format(attribute[0])
    result_text += 'Attribute kind: {!r}\n'.format(attribute[1])
    result_text += 'Component count: {}\n'.format(attribute[2])
    result_text += 'Value type: {}\n'.format(attribute[3])
    result_text += 'Display name: {!r}\n'.format(attribute[4])
    result_text += 'Metadata: {!r}\n'.format(attribute[5])
    result_text += 'Options: {!r}\n'.format(attribute[6])
    result_text += '\n'

    buffer = voxie.Buffer(srfT.GetAttributeReadonly(name), 2, False)

    result_text += 'Minimum value: {!r}\n'.format(np.min(buffer))
    result_text += 'Maximum value: {!r}\n'.format(np.max(buffer))
    result_text += 'Average value (non-weighted): {!r}\n'.format(np.mean(buffer))
    # TODO: Weighted average(s)

    result_text_bin = (result_text).encode('utf-8')

    with instance.CreateFileDataByteStream('text/plain; charset=UTF-8', len(result_text_bin)) as output:
        with output.CreateUpdate() as update:
            with output.GetBufferWritable(update) as buffer:
                buffer[:] = bytearray(result_text_bin)

            with update.Finish() as version:
                result = {}
                result[output_path] = {
                    'Data': voxie.Variant('o', output._objectPath),
                    'DataVersion': voxie.Variant('o', version._objectPath),
                }
                op.Finish(result)

    instance._context.client.destroy()
