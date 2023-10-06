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
import sys
import json
import codecs
import os

import numpy as np

import data_source

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args, enableService=True)
instance = context.createInstance()

if args.voxie_action != 'Import':
    raise Exception('Invalid operation: ' + repr(args.voxie_action))

clientManager = voxie.clientimpl.ClientManagerImpl(context, context.bus)


class Stream:
    def __init__(self, *, name, imageCount, source, perImageMetadata):
        self.name = name
        self.imageCount = imageCount
        self.source = source
        self.perImageMetadata = perImageMetadata


class TomographyRawData2DAccessorOperationsImpl(voxie.DBusExportObject):
    def __init__(self, conn, *, imageKind, streams, metadata, geometries):
        voxie.DBusExportObject.__init__(
            self, ['de.uni_stuttgart.Voxie.TomographyRawData2DAccessorOperations'], context=context)
        self.path = context.nextId(
            '/de/uni_stuttgart/Voxie/TomographyRawData2DAccessorOperationsImpl')
        self.refCount = 0
        self.imageKind = imageKind
        self.streams = streams
        self.streamByName = {stream.name: stream for stream in streams}
        self.metadata = metadata
        self.geometries = geometries
        self.geometryNames = [geom['Name'] for geom in geometries]
        self.geometryByName = {geom['Name']: geom['Data'] for geom in geometries}

        self.add_to_connection(conn, self.path)

    def GetNumberOfImages(self, stream, options):
        if stream not in self.streamByName:
            raise Exception('Invalid stream name: ' + repr(stream))
        return self.streamByName[stream].imageCount

    def GetMetadata(self, options):
        return self.metadata

    def GetAvailableImageKinds(self, options):
        return [self.imageKind]

    def GetAvailableStreams(self, options):
        return [stream.name for stream in streams]

    def GetAvailableGeometryTypes(self, options):
        return self.geometryNames

    def GetGeometryData(self, geometryType, options):
        if geometryType not in self.geometryNames:
            raise Exception('Invalid geometry type: ' + repr(geometryType))
        return self.geometryByName[geometryType]

    def GetImageShape(self, stream, id, options):
        if stream not in self.streamByName:
            raise Exception('Invalid stream name: ' + repr(stream))
        streamData = self.streamByName[stream]
        if id < 0 or id >= streamData.imageCount:
            raise Exception('Invalid image ID: ' + id)
        return streamData.source.get_image_size(id)

    def GetPerImageMetadata(self, stream, id, options):
        if stream not in self.streamByName:
            raise Exception('Invalid stream name: ' + repr(stream))
        streamData = self.streamByName[stream]
        if id < 0 or id >= streamData.imageCount:
            raise Exception('Invalid image ID: ' + id)
        return streamData.perImageMetadata[id]

    def ReadImages(self, imageKind, images, inputRegionStart, rawData, firstOutputImageId, outputRegionStart, regionSize, options):
        firstOutputImageId = int(firstOutputImageId)
        count = len(images)
        for stream, id in images:
            if stream not in self.streamByName:
                raise Exception('Invalid stream name: ' + repr(stream))
            streamData = self.streamByName[stream]
            if id < 0 or id >= streamData.imageCount:
                raise Exception('Invalid image ID: ' + id)

        if rawData._busName != context.busName:
            raise Exception('rawData._busName != context.busName')

        with rawData.CreateUpdate() as update, rawData.GetBufferWritable(update) as buffer:
            for i in range(count):
                stream, id = images[i]
                streamData = self.streamByName[stream]
                region = buffer[outputRegionStart[0]:outputRegionStart[0] + regionSize[0],
                                outputRegionStart[1]:outputRegionStart[1] + regionSize[1],
                                firstOutputImageId + i]
                img = streamData.source.read_image(id)
                region[:] = img[inputRegionStart[0]:inputRegionStart[0] + regionSize[0],
                                inputRegionStart[1]:inputRegionStart[1] + regionSize[1]]

            with update.Finish() as version:
                return version.VersionString

    def incRefCount(self):
        self.refCount += 1

    def decRefCount(self):
        self.refCount -= 1


provider = None

with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationImport']).ClaimOperationAndCatch() as op:
    filename = op.Filename

    with open(filename, 'rb') as file:
        jsonData = json.load(codecs.getreader('utf-8')(file))

    if jsonData['Type'] != 'de.uni_stuttgart.Voxie.FileFormat.TomographyRawData.VxTRaw':
        raise Exception('Expected type %s, got %s' % (
            repr('de.uni_stuttgart.Voxie.FileFormat.TomographyRawData.VxTRaw'), repr(jsonData['Type'])))

    imageKind = jsonData['ImageKind']
    metadata = jsonData['Metadata']
    geometries = jsonData['Geometries']

    streams = []

    for streamData in jsonData['Streams']:
        name = streamData['Name']

        imageCount = int(streamData['ImageCount'])
        imageShape = None
        arrayShape = [None, None, imageCount]
        if 'ImageShape' in streamData:
            imageShape = [int(streamData['ImageShape'][i]) for i in range(2)]
            arrayShape = [imageShape[0], imageShape[1], imageCount]
        # TODO: gridSpacing / imageOrigin is not used currently. Should gridSpacing / imageOrigin be optional or removed? (It could be different per image)
        # gridSpacing = [float(streamData['GridSpacing'][i]) for i in range(2)]
        # imageOrigin = [float(streamData['ImageOrigin'][i]) for i in range(2)]

        if 'PerImageMetadata' in streamData:
            perImageMetadata = streamData['PerImageMetadata']
        else:
            perImageMetadata = [{}] * imageCount
        if len(perImageMetadata) != imageCount:
            raise Exception('Unexpected number of PerImageMetadata entries')

        source = data_source.get(filename, streamData['DataSourceType'], streamData['DataSource'], array_shape=arrayShape, data_type=None)

        streams.append(Stream(name=name, imageCount=imageCount, source=source, perImageMetadata=perImageMetadata))

    provider = TomographyRawData2DAccessorOperationsImpl(context.bus, imageKind=imageKind, streams=streams, metadata=metadata, geometries=geometries)
    clientManager.objects[provider.path] = provider
    providerTuple = (context.bus.get_unique_name() if not instance._context.isPeerToPeerConnection else '', dbus.ObjectPath(provider.path))
    with instance.CreateTomographyRawData2DAccessor(providerTuple, {}, DBusObject_handleMessages=True) as data:
        with data.GetCurrentVersion() as version:
            op.Finish(data, version)

# if provider is not None:
#     print('RefCount: %d' % provider.refCount, flush=True)
while provider is not None and provider.refCount > 0:
    context.iteration()
    # print('RefCount: %d' % provider.refCount, flush=True)
# print('Terminating', flush=True)
