#!/usr/bin/python3
#
# Copyright (c) 2014-2023 The Voxie Authors
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
import dbus
import json
import io
import os
import hashlib
import codecs

import voxie.dbus_as_json

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

# TODO: Should this depend on the property type? Currently mostly used for ChemicalComposition, which contains JSON data.
style = voxie.dbus_as_json.DBusAsJSONStyle()
style.variant_style = voxie.dbus_as_json.VariantStyle.JsonVariant

if args.voxie_action == 'Export':
    with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationExport']).ClaimOperationAndCatch() as op:
        filename = op.Filename
        series_data = op.Data.CastTo('de.uni_stuttgart.Voxie.VolumeSeriesData')
        keys = series_data.ListKeys()

        volume_exporter = instance.Components.GetComponent('de.uni_stuttgart.Voxie.Exporter', 'de.uni_stuttgart.Voxie.FileFormat.Volume.VxVol.Export').CastTo('de.uni_stuttgart.Voxie.Exporter')

        result = {}

        result['Type'] = "de.uni_stuttgart.Voxie.FileFormat.VolumeSeries.VxVolSer"

        result['Data'] = []
        result['Dimensions'] = []

        result['VolumeOrigin'] = series_data.VolumeOrigin
        result['VolumeSize'] = series_data.VolumeSize

        base_filename = filename
        if base_filename.endswith('.json'):
            base_filename = base_filename[:-5]
        # if base_filename.endswith('.vxvolser'):
        #     base_filename = base_filename[:-9]

        for dim in series_data.Dimensions:
            prop = dim.Property
            prop_ty = prop.Type
            ty_sig = prop_ty.DBusSignature
            entries_json = voxie.dbus_as_json.encode_dbus_as_json('a' + ty_sig, dim.ListEntries().getValue('a' + ty_sig), style=style)
            dim_data = {
                'Name': prop.Name,
                'Entries': entries_json,
            }
            prop_json = prop.PropertyDefinition
            for key in dim_data:
                if key in prop_json:
                    raise Exception('Duplicate key: {!r}'.format(key))
            dim_data.update(prop_json)
            result['Dimensions'].append(dim_data)

        for pos, key in enumerate(keys):
            # TODO: Forward cancellation to child exporter?
            op.ThrowIfCancelled()

            key_str = '_'.join(['entry'] + [str(i) for i in key])
            value_filename = base_filename + '.' + key_str + '.vxvol.json'
            # TODO: Allow embedding .vxvol.json data in .vxvolser.json file?
            result['Data'].append({
                'Key': key,
                'ValueFilename': os.path.basename(value_filename),
            })

            value = series_data.LookupEntry(key)
            if value is None:
                raise Exception('value is None')

            with volume_exporter.StartExport(value, value_filename) as exp_result:
                exp_result.Operation.WaitFor()

            op.SetProgress((pos + 1) / len(keys))

        f = io.StringIO()
        json.dump(result, f, allow_nan=False, sort_keys=True,
                  ensure_ascii=False, indent=2)
        s = bytes(f.getvalue() + '\n', 'utf-8')
        with open(filename, 'wb') as file:
            file.write(s)

        op.Finish()

elif args.voxie_action == 'Import':
    with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationImport']).ClaimOperationAndCatch() as op:
        filename = op.Filename

        with open(filename, 'rb') as file:
            jsonData = json.load(codecs.getreader('utf-8')(file))

        if jsonData['Type'] != "de.uni_stuttgart.Voxie.FileFormat.VolumeSeries.VxVolSer":
            raise Exception('Expected type %s, got %s' % (repr(
                "de.uni_stuttgart.Voxie.FileFormat.VolumeSeries.VxVolSer"), repr(jsonData['Type'])))

        volumeOrigin = [float(jsonData['VolumeOrigin'][i]) for i in range(3)]
        volumeSize = [float(jsonData['VolumeSize'][i]) for i in range(3)]

        base_name = filename
        if 'BaseFilename' in jsonData:
            base_name = os.path.join(os.path.dirname(base_name), jsonData['BaseFilename'])

        # TODO: Clean up
        dims = []
        dimProps = []
        dimLens = []
        dimValues = []
        dimTypes = []
        try:
            for dimData in jsonData['Dimensions']:
                dim_name = dimData['Name']
                # dim_displayName = dimData['DisplayName']
                # dim_type = instance.Components.GetComponent('de.uni_stuttgart.Voxie.ComponentType.PropertyType', dimData['Type']).CastTo('de.uni_stuttgart.Voxie.PropertyType')
                dim_values = dimData['Entries']

                dim_prop_data = dict(dimData)
                del dim_prop_data['Name']
                del dim_prop_data['Entries']
                dim_prop = instance.CreateDataProperty(dim_name, dim_prop_data)
                dim_type = dim_prop.Type

                dim = instance.CreateSeriesDimension(dim_prop, voxie.Variant('a' + dim_type.DBusSignature, voxie.dbus_as_json.decode_dbus_as_json('a' + dim_type.DBusSignature, dim_values, style=style)))
                dims.append(dim)
                dimProps.append(dim_prop)
                dimLens.append(dim.Length)
                dimValues.append(dim_values)
                dimTypes.append(dim_type)

            foundKeys = set()
            dataEntries = []
            for dat in jsonData['Data']:
                key = dat['Key']
                fn = dat['ValueFilename']

                if type(key) != list:
                    raise Exception('type(key) != list')
                if len(key) != len(dimLens):
                    raise Exception('len(key) != len(dimLens)')
                for i in range(len(key)):
                    if type(key[i]) != int:
                        raise Exception('type(key[i]) != int')
                    if key[i] < 0:
                        raise Exception('key[i] < 0')
                    if key[i] >= dimLens[i]:
                        raise Exception('key[i] >= dimLens[i]')

                if tuple(key) in foundKeys:
                    raise Exception('Duplicate key: ' + str(key))
                foundKeys.add(tuple(key))

                if type(fn) != str:
                    raise Exception('type(fn) != str')
                full_fn = os.path.join(os.path.dirname(base_name), fn)
                dataEntries.append((key, full_fn))

            op.ThrowIfCancelled()
            with instance.CreateVolumeSeriesData(dims, volumeOrigin, volumeSize) as resultData:
                with resultData.CreateUpdate() as update:
                    op.SetProgress(0)
                    i = 0
                    for key, full_fn in dataEntries:
                        # TODO: Forward cancellation to child importer?
                        op.ThrowIfCancelled()
                        # TODO: Progress update during import
                        with instance.Import(full_fn) as data:
                            # Update data properties of loaded data
                            with data.CreateUpdate() as entry_update:
                                for j, val in enumerate(key):
                                    prop = dimProps[j]
                                    prop_value = dimValues[j][val]
                                    prop_type = dimTypes[j]
                                    sig = prop_type.DBusSignature
                                    data.SetProperty(entry_update, prop, voxie.Variant(sig, voxie.dbus_as_json.decode_dbus_as_json(sig, prop_value, style=style)), {
                                        'ReplaceMode': voxie.Variant('s', 'de.uni_stuttgart.Voxie.ReplaceMode.InsertOrSame'),
                                    })

                                with entry_update.Finish() as version:
                                    pass

                            resultData.SetEntry(update, key, data, {
                                'ReplaceMode': voxie.Variant('s', 'de.uni_stuttgart.Voxie.ReplaceMode.Insert'),
                            })

                        i = i + 1
                        op.SetProgress(i / len(dataEntries))

                    with update.Finish() as version:
                        op.Finish(resultData, version)
        finally:
            for dim in dims:
                with dim:
                    pass

else:
    raise Exception('Invalid operation: ' + repr(args.voxie_action))


context.client.destroy()
