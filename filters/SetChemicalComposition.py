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


args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

if args.voxie_action != 'RunFilter':
    raise Exception('Invalid operation: ' + repr(args.voxie_action))

with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationRunFilter']).ClaimOperationAndCatch() as op:
    inputData = op.GetInputData('de.uni_stuttgart.Voxie.Input')

    output_path = op.Properties['de.uni_stuttgart.Voxie.Output'].getValue('o')

    comp = voxie.json_dbus.dbus_to_json(op.Properties['de.uni_stuttgart.Voxie.Filter.SetChemicalComposition.Composition'].getValue('v'))

    # TODO: Avoid modifying input data
    outputData = inputData

    prop_data = {
        "Type": "de.uni_stuttgart.Voxie.PropertyType.ChemicalComposition",
    }
    prop = instance.CreateDataProperty('de.uni_stuttgart.Voxie.SeriesDimension.ChemicalComposition', prop_data)

    with outputData.CreateUpdate() as update:
        outputData.SetProperty(update, prop, voxie.Variant('v', voxie.json_dbus.json_to_dbus(comp)), {
            'ReplaceMode': voxie.Variant('s', 'de.uni_stuttgart.Voxie.ReplaceMode.InsertOrReplace'),
        })

        with update.Finish() as version:
            result = {}
            result[output_path] = {
                'Data': voxie.Variant('o', outputData._objectPath),
                'DataVersion': voxie.Variant('o', version._objectPath),
            }
            op.Finish(result)

    instance._context.client.destroy()
