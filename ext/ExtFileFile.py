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
import sys
import json
import io
import codecs
import os

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

if args.voxie_action != 'Export' and args.voxie_action != 'Import':
    raise Exception('Invalid operation: ' + repr(args.voxie_action))

if args.voxie_action == 'Export':
    with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationExport']).ClaimOperationAndCatch() as op:
        filename = op.Filename
        data = op.Data
        if data is None:
            raise Exception('Data node is empty')
        data = data.CastTo('de.uni_stuttgart.Voxie.FileData')
        data = data.CastTo('de.uni_stuttgart.Voxie.FileDataByteStream')

        # print('Export %s %s' % (filename, data))

        result = {}

        result['Type'] = 'de.uni_stuttgart.Voxie.FileFormat.File.Json'

        dataFilename = filename
        if dataFilename.endswith('.json'):
            dataFilename = dataFilename[:-5]
        dataFilename += '.dat'
        dataFilenameBase = os.path.basename(dataFilename)

        resData = []
        result['Filename'] = dataFilenameBase
        result['MediaType'] = data.MediaType

        buf = data[:]

        # TODO: Progress bar?
        with open(dataFilename, 'wb') as file:
            file.write(buf)
        op.SetProgress(0.9)

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

        op.SetProgress(0.1)

        if jsonData['Type'] != 'de.uni_stuttgart.Voxie.FileFormat.File.Json':
            raise Exception('Expected type %s, got %s' % (repr(
                'de.uni_stuttgart.Voxie.FileFormat.File.Json'), repr(jsonData['Type'])))

        # print('Import %s %s' % (filename, data))

        if type(jsonData['MediaType']) != str:
            raise Exception("type(jsonData['MediaType']) != str")
        media_type = jsonData['MediaType']

        if type(jsonData['Filename']) != str:
            raise Exception("type(jsonData['Filename']) != str")
        data_filename = jsonData['Filename']
        data_filename = os.path.join(os.path.dirname(filename), data_filename)

        with open(data_filename, 'rb') as file:
            data = file.read()

        op.SetProgress(0.8)

        with instance.CreateFileDataByteStream(media_type, len(data)) as resultData:
            with resultData.CreateUpdate() as update:
                with resultData.GetBufferWritable(update) as buffer:
                    buffer[:] = bytearray(data)

                version = update.Finish()
            with version:
                op.Finish(resultData, version)

context.client.destroy()
