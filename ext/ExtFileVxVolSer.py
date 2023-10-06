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


args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()


if args.voxie_action == 'Export':
    with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationExport']).ClaimOperationAndCatch() as op:
        raise Exception('TODO: Export')

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

        dims = []
        dimLens = []
        try:
            for dimData in jsonData['Dimensions']:
                dim_name = dimData['Name']
                dim_displayName = dimData['DisplayName']
                dim_type = instance.Components.GetComponent('de.uni_stuttgart.Voxie.ComponentType.PropertyType', dimData['Type']).CastTo('de.uni_stuttgart.Voxie.PropertyType')
                dim_values = dimData['Entries']
                dim = instance.CreateSeriesDimension(dim_name, dim_displayName, dim_type, voxie.Variant('a' + dim_type.DBusSignature, dim_values))
                dims.append(dim)
                dimLens.append(dim.Length)

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

            with instance.CreateVolumeSeriesData(dims, volumeOrigin, volumeSize) as resultData:
                with resultData.CreateUpdate() as update:
                    op.SetProgress(0)
                    i = 0
                    for key, full_fn in dataEntries:
                        # TODO: Progress update during import
                        with instance.Import(full_fn) as data:
                            resultData.AddEntry(update, key, data)

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
