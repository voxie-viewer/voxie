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
import voxie.json_dbus
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
        data = op.Data.CastTo('de.uni_stuttgart.Voxie.TableData')

        # print('Export %s %s' % (filename, data))

        result = {}

        result['Type'] = 'de.uni_stuttgart.Voxie.FileFormat.Table.Json'

        columns = data.Columns
        # print(columns)
        resultColumns = []
        result['Columns'] = resultColumns
        colNr = None
        sigs = []
        for i in range(len(columns)):
            col = {}
            resultColumns.append(col)
            col['Name'] = columns[i][0]
            typePath = columns[i][1]
            typeObj = context.makeObject(data._connection, data._busName, typePath, [
                                         'de.uni_stuttgart.Voxie.PropertyType'])
            col['Type'] = typeObj.Name
            sigs.append(typeObj.DBusSignature)
            col['DisplayName'] = columns[i][2]

            col['Metadata'] = {}
            for key in columns[i][3]:
                value = columns[i][3][key]
                col['Metadata'][key] = voxie.json_dbus.dbus_to_json(value)

            for opt in columns[i][4]:
                print('ExtFileTable: Warning: Unknown column option: ' + repr(opt))

        op.SetProgress(0.1)
        resData = []
        result['Data'] = resData
        # TODO: Get in multiple steps to avoid problems with DBus maximum message size
        for row in data.GetRows(0, 2**64 - 1):
            rowData = []
            resData.append(rowData)
            for j, val in enumerate(row[1]):
                if val.signature != sigs[j]:
                    raise Exception('val.signature != sigs[j]')
                # TODO: This might depend on the actual column type
                rowData.append(voxie.dbus_as_json.encode_dbus_as_json(sigs[j], val.value))
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

        if jsonData['Type'] != 'de.uni_stuttgart.Voxie.FileFormat.Table.Json':
            raise Exception('Expected type %s, got %s' % (
                repr('de.uni_stuttgart.Voxie.FileFormat.Table.Json'), repr(jsonData['Type'])))

        # print('Import %s %s' % (filename, data))

        result = {}

        columns = []
        sigs = []
        for col in jsonData['Columns']:
            metadata = {}
            for key in col['Metadata']:
                value = col['Metadata'][key]
                if 'DBusSignature' in value and 'Value' in value:
                    # Old files
                    metadata[key] = voxie.Variant(value['DBusSignature'], voxie.dbus_as_json.decode_dbus_as_json(
                        value['DBusSignature'], value['Value']))
                else:
                    metadata[key] = voxie.json_dbus.json_to_dbus(value)
            opt = {}  # TODO: options?
            typeObj = instance.Components.GetComponent(
                'de.uni_stuttgart.Voxie.ComponentType.PropertyType', col['Type']).CastTo('de.uni_stuttgart.Voxie.PropertyType')
            c = (col['Name'], typeObj._objectPath,
                 col['DisplayName'], metadata, opt)
            columns.append(c)
            sigs.append(typeObj.DBusSignature)

        with instance.CreateTableData(columns) as resultData:
            with resultData.CreateUpdate() as update:
                op.SetProgress(0.5)

                # TODO: progress bar
                for entry in jsonData['Data']:
                    row = []
                    for j, value in enumerate(entry):
                        row.append(voxie.Variant(
                            sigs[j], voxie.dbus_as_json.decode_dbus_as_json(sigs[j], value)))
                    resultData.AddRow(update, row)
                version = update.Finish()
            with version:
                op.Finish(resultData, version)

context.client.destroy()
