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

import numpy
import voxie
import dbus
import time
import sys
import json
import io
import math
import codecs
import voxie.dbus_as_json

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

# Note: The order of the entries is preserved, but the RowID is *not* stored

if args.voxie_action != 'Export' and args.voxie_action != 'Import':
    raise Exception('Invalid operation: ' + repr(args.voxie_action))

if args.voxie_action == 'Export':
    with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationExport']).ClaimOperationAndCatch() as op:
        filename = op.Filename
        data = op.Data.CastTo('de.uni_stuttgart.Voxie.GeometricPrimitiveData')

        # print('Export %s %s' % (filename, data))

        result = {}

        result['Type'] = 'de.uni_stuttgart.Voxie.FileFormat.GeometricPrimitive.Json'

        resData = []
        result['Data'] = resData
        # TODO: Get in multiple steps to avoid problems with DBus maximum message size
        for prim in data.GetPrimitives(0, 2**64 - 1):
            entry = {}
            resData.append(entry)

            # Don't save the ID

            # TODO: Cache this somewhere?
            ty = context.makeObject(data._connection, data._busName, prim[1], [
                                    'de.uni_stuttgart.Voxie.GeometricPrimitiveType'])
            tyName = ty.Name
            tyTypes = ty.ValueDBusSignatures

            entry['PrimitiveType'] = tyName
            entry['DisplayName'] = prim[2]
            values = prim[3]
            entry['PrimitiveValues'] = {key: voxie.dbus_as_json.encode_dbus_as_json_variant(
                tyTypes[key], values[key]) for key in values}
            for opt in prim[4]:
                print(
                    'ExtFileGeometricPrimitive: Warning: Unknown primitive option: ' + repr(opt))
        op.SetProgress(0.5)

        f = io.StringIO()
        json.dump(result, f, allow_nan=False, sort_keys=True,
                  ensure_ascii=False, indent=2)
        s = bytes(f.getvalue() + '\n', 'utf-8')
        with open(filename, 'wb') as file:
            file.write(s)

        op.Finish()
else:
    with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationImport']).ClaimOperationAndCatch() as op:
        filename = op.Filename

        with open(filename, 'rb') as file:
            jsonData = json.load(codecs.getreader('utf-8')(file))

        op.SetProgress(0.2)

        if jsonData['Type'] != 'de.uni_stuttgart.Voxie.FileFormat.GeometricPrimitive.Json':
            raise Exception('Expected type %s, got %s' % (repr(
                'de.uni_stuttgart.Voxie.FileFormat.GeometricPrimitive.Json'), repr(jsonData['Type'])))

        # print('Import %s %s' % (filename, data))

        with instance.CreateGeometricPrimitiveData() as resultData:
            with resultData.CreateUpdate() as update:
                op.SetProgress(0.3)

                # TODO: progress bar
                for row in jsonData['Data']:
                    typeName = row['PrimitiveType']
                    displayName = row['DisplayName']
                    valuesJson = row['PrimitiveValues']

                    # TODO: Cache this somewhere?
                    ty = instance.Components.GetComponent(
                        'de.uni_stuttgart.Voxie.ComponentType.GeometricPrimitiveType', typeName).CastTo('de.uni_stuttgart.Voxie.GeometricPrimitiveType')
                    tyTypes = ty.ValueDBusSignatures

                    valuesDBus = {key: voxie.Variant(tyTypes[key], voxie.dbus_as_json.decode_dbus_as_json(
                        tyTypes[key], valuesJson[key])) for key in valuesJson}

                    resultData.AddPrimitive(
                        update, ty, displayName, valuesDBus)
                version = update.Finish()
            with version:
                op.Finish(resultData, version)

context.client.destroy()
