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

        filename = op.Filename
        containerData = op.Data.CastTo('de.uni_stuttgart.Voxie.ContainerData')
        keys = containerData.GetKeys()

        result = {}

        # TODO: This should be named "de.uni_stuttgart.Voxie.FileFormat.ContainerData.Json" or "de.uni_stuttgart.Voxie.FileFormat.ContainerData.VxCo" or something like that (the importer also has to be changed and has to allow the old names)
        result['Type'] = "de.uni_stuttgart.Voxie.FileFormat.ContainerData.Json.Export"
        result['Name'] = containerData.GetName()

        # Map which holds the keys and the file_names
        result['Elements'] = {}

        interfaces = []

        if filename.endswith('.json'):
            base_filename = filename[:-5]

        dataFilename = filename[:-5]
        for key in keys:
            value = containerData.GetElement(key)
            interfaces = value.SupportedInterfaces
            key_filepath = base_filename + "_" + key

            # get exporter object
            if "de.uni_stuttgart.Voxie.TableData" in interfaces:
                # table csv
                exporter_name = 'de.uni_stuttgart.Voxie.FileFormat.Table.Csv.Export'

                # table json
                exporter_name = 'de.uni_stuttgart.Voxie.FileFormat.Table.Json.Export'

            elif 'de.uni_stuttgart.Voxie.VolumeData' in interfaces:
                # hdf5 export
                exporter_name = 'de.uni_stuttgart.Voxie.FileFormat.Hdf5.ExportVolume'

            elif "de.uni_stuttgart.Voxie.EventListDataAccessor" in interfaces:
                # event list csv
                exporter_name = 'de.uni_stuttgart.Voxie.FileFormat.EventList.Csv.Export'

                # event list json
                exporter_name = 'de.uni_stuttgart.Voxie.FileFormat.EventList.Json.Export'

            elif "de.uni_stuttgart.Voxie.GeometricPrimitiveData" in interfaces:

                # geometri primitives json export
                exporter_name = 'de.uni_stuttgart.Voxie.FileFormat.GeometricPrimitive.Json.Export'

            elif "de.uni_stuttgart.Voxie.TomographyRawDataBase" in interfaces:

                # tomography raw
                exporter_name = 'de.uni_stuttgart.Voxie.FileFormat.TomographyRawData.VxTRaw.Export'

            elif "de.uni_stuttgart.Voxie.SurfaceData" in interfaces:
                # ply export
                exporter_name = 'de.uni_stuttgart.Voxie.FileFormat.Ply.Export'

            else:
                raise Exception(
                    "No known exporter for the interface(s): {}".format(interfaces))

            exporter = instance.Components.GetComponent('de.uni_stuttgart.Voxie.Exporter',
                                                        exporter_name).CastTo('de.uni_stuttgart.Voxie.Exporter')

            filename_export = exporter.FilterForceMatch(key_filepath)

            result['Elements'][key] = {
                "Path": os.path.basename(filename_export), "Interfaces": interfaces, "Exporter": exporter_name}

            with exporter.StartExport(value, filename_export) as exp_result:
                exp_result.Operation.WaitFor()

        # Is this needed?
        op.SetProgress(0.1)

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

        # Is this needed?
        op.SetProgress(0.2)

        if jsonData['Type'] != "de.uni_stuttgart.Voxie.FileFormat.ContainerData.Json.Export":
            raise Exception('Expected type %s, got %s' % (repr(
                "de.uni_stuttgart.Voxie.FileFormat.ContainerData.Json.Export"), repr(jsonData['Type'])))

        with instance.CreateContainerData(jsonData['Name']) as resultData:
            with resultData.CreateUpdate() as update:
                op.SetProgress(0.3)

                # iterate over all elements and start each importer.
                for key in jsonData['Elements']:
                    import_path = os.path.join(os.path.dirname(
                        filename), jsonData['Elements'][key]["Path"])

                    if os.path.exists(import_path):
                        object_data = instance.Import(import_path)
                        resultData.InsertElement(key, object_data, update)
                    else:
                        raise Exception('File %s, is not placed in same folder as %s --> can not be imported' % (repr(
                            jsonData['Elements'][key]["Path"]), repr(filename)))

                version = update.Finish()
                with version:
                    op.Finish(resultData, version)


else:
    raise Exception('Invalid operation: ' + repr(args.voxie_action))


context.client.destroy()
