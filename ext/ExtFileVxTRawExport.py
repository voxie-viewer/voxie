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
import os
import hashlib

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

if args.voxie_action != 'Export':
    raise Exception('Invalid operation: ' + args.voxie_action)

with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationExport']).ClaimOperationAndCatch() as op:
    filename = op.Filename
    data = op.Data.CastTo('de.uni_stuttgart.Voxie.TomographyRawData2DAccessor')

    # print('Export %s %s' % (filename, data))

    jsonData = {}

    # TODO: Allow selecting the image kind
    imageKind = data.GetAvailableImageKinds()[0]

    # TODO: Allow selecting the data type?
    dataType = ('float', 32, 'native')

    geometries = []
    for name in data.GetAvailableGeometryTypes():
        geometries.append({
            'Name': name,
            'Data': data.GetGeometryData(name),
        })

    jsonData['Type'] = 'de.uni_stuttgart.Voxie.FileFormat.TomographyRawData.VxTRaw'

    jsonData['ImageKind'] = imageKind
    jsonData['Metadata'] = data.GetMetadata()
    jsonData['Geometries'] = geometries

    streams = []
    streamId = 0
    streamDat = []
    overallCount = 0
    for streamName in data.GetAvailableStreams():
        imageCount = data.GetNumberOfImages(streamName)

        perImageMetadata = []
        for i in range(imageCount):
            perImageMetadata.append(data.GetPerImageMetadata(streamName, i))

        dataFilename = filename
        if dataFilename.endswith('.json'):
            dataFilename = dataFilename[:-5]
        dataFilename += '.stream-%04d.dat' % (streamId,)
        dataFilenameBase = os.path.basename(dataFilename)

        streams.append({
            'Name': streamName,
            'ImageCount': imageCount,
            'PerImageMetadata': perImageMetadata,
            'DataSourceType': 'ImageArchive',
            'DataSource': {
                'DataFilename': dataFilenameBase,
            },
        })
        streamDat.append({
            'Name': streamName,
            'Filename': dataFilename,
            'ImageCount': imageCount,
        })
        overallCount += imageCount
        streamId += 1
    jsonData['Streams'] = streams

    buffer = None
    bufferSize = None
    bufferType = None
    try:
        overallProgress = 0
        for stream in streamDat:
            streamName = stream['Name']
            imageCount = stream['ImageCount']
            dataFilename = stream['Filename']

            # print('Writing %d images to %s' % (imageCount, filename))
            iaiFilename = dataFilename + '.iai'
            with open(dataFilename, 'wb') as file:
                with open(iaiFilename, 'w') as fileIai:
                    fileIai.write('"ID","Offset","TypeName","TypeSize","TypeEndian","Width","Height","DataSHA512"\r\n')
                    offset = 0
                    overallSha = hashlib.sha512()

                    for i in range(imageCount):
                        op.ThrowIfCancelled()
                        op.SetProgress(overallProgress / overallCount)

                        size = data.GetImageShape(streamName, i)
                        if size != bufferSize:
                            if buffer is not None:
                                buffer.__exit__(None, None, None)
                                buffer = None
                            bufferSize = size
                            buffer = instance.CreateTomographyRawData2DRegular(size, 1, dataType)
                            bufferType = buffer.DataType

                        allowIncompleteData = True
                        if not allowIncompleteData:
                            data.ReadImages(imageKind, [(streamName, i)], (0, 0), (buffer._busName, buffer._objectPath), 0, (0, 0), size)
                        else:
                            data.ReadImages(imageKind, [(streamName, i)], (0, 0), (buffer._busName, buffer._objectPath), 0, (0, 0), size, {'AllowIncompleteData': voxie.Variant('b', True)})
                            while True:
                                with buffer.GetCurrentVersion() as version:
                                    stat = version.Metadata
                                    # print('Status:', stat, flush=True)
                                    if 'Status' not in stat:
                                        break
                                    if 'Error' in stat['Status']:
                                        raise Exception('Got error: ' + repr(stat['Status']['Error']))
                                    prog = float(stat['Status']['Progress'])
                                    if prog < 0 or prog > 1:
                                        raise Exception('prog < 0 or prog > 1')
                                    op.ThrowIfCancelled()
                                    op.SetProgress((overallProgress + prog) / overallCount)
                                    time.sleep(0.1)

                        buf = buffer.GetBufferReadonly()
                        cnt = size[0] * size[1] * (dataType[1] / 8)
                        dat = buf[:, ::-1].tobytes(order='F')
                        if len(dat) != cnt:
                            raise Exception('len(dat) != cnt')
                        sha512sum = hashlib.sha512(dat).hexdigest()
                        overallSha.update(dat)
                        file.write(dat)
                        fileIai.write('%d,%d,"%s",%d,"%s",%d,%d,"%s"\r\n' % (i, offset, bufferType[0], bufferType[1], bufferType[2], size[0], size[1], sha512sum))
                        offset += cnt
                        overallProgress += 1
                    fileIai.write(',%d,,,,,,"%s"' % (offset, overallSha.hexdigest()))
    finally:
        if buffer is not None:
            buffer.__exit__(None, None, None)

    if overallProgress != overallCount:
        raise Exception('overallProgress != overallCount')
    op.SetProgress(1.0)

    f = io.StringIO()
    json.dump(jsonData, f, allow_nan=False, sort_keys=True,
              ensure_ascii=False, indent=2)
    s = bytes(f.getvalue(), 'utf-8')
    with open(filename, 'wb') as file:
        file.write(s)

    op.Finish()

context.client.destroy()
