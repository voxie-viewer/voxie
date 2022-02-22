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
import dbus.service
import time
import sys

sys.stderr = sys.stdout = open('/dev/tty', 'w')

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args, enableService=True)
instance = context.createInstance()

if args.voxie_action is not None and args.voxie_action != 'Import':
    raise Exception('Invalid operation: ' + args.voxie_action)

clientManager = voxie.clientimpl.ClientManagerImpl(context, context.bus)


class TomographyRawData2DAccessorOperationsImpl(voxie.DBusExportObject):
    def __init__(self, conn):
        voxie.DBusExportObject.__init__(
            self, ['de.uni_stuttgart.Voxie.TomographyRawData2DAccessorOperations'], context=context)
        self.path = context.nextId(
            '/de/uni_stuttgart/Voxie/Script/ImportRawTest/TomographyRawData2DAccessorOperationsImpl')
        self.refCount = 0
        self.count = 44
        self.add_to_connection(conn, self.path)

    def GetNumberOfImages(self, stream, options):
        if stream != '':
            raise Exception('Invalid stream name: ' + repr(stream))
        return self.count

    def GetMetadata(self, options):
        return {
            'Info': {
                'Geometry': {
                    'DistanceSourceDetector': 44.55
                }
            }
        }

    def GetAvailableImageKinds(self, options):
        return [{'Type': 'SomeType'}, {'Type': 'SomeOtherType'}]

    def GetAvailableStreams(self, options):
        return ['']

    def GetAvailableGeometryTypes(self, options):
        return ['ConeBeamCT']

    def GetGeometryData(self, geometryType, options):
        if geometryType != 'ConeBeamCT':
            raise Exception('Invalid geometry type: ' + repr(geometryType))
        return {'DistanceSourceDetector': 44.55}

    def GetImageShape(self, stream, id, options):
        if stream != '':
            raise Exception('Invalid stream name: ' + repr(stream))
        return (640, 480)

    def GetPerImageMetadata(self, stream, id, options):
        if stream != '':
            raise Exception('Invalid stream name: ' + repr(stream))
        return {'Angle': id * 5}

    def ReadImages(self, imageKind, images, inputRegionStart, rawData, firstOutputImageId, outputRegionStart, regionSize, options):
        firstOutputImageId = int(firstOutputImageId)
        count = len(images)
        for stream, id in images:
            if stream != '':
                raise Exception('Invalid stream name: ' + repr(stream))
            if id < 0 or id >= self.count:
                raise Exception('Image ID is out of range')

        if rawData._busName != context.busName:
            raise Exception('rawData._busName != context.busName')

        with rawData.CreateUpdate() as update, rawData.GetBufferWritable(update) as buffer:
            for i in range(count):
                stream, id = images[i]
                region = buffer[outputRegionStart[0]:outputRegionStart[0] + regionSize[0],
                                outputRegionStart[1]:outputRegionStart[1] + regionSize[1],
                                firstOutputImageId + i]
                print(buffer[()].shape)
                print(region[()].shape)
                region[:, :] = 0
                region[30:-30, 30:-30] = id

            version = update.Finish()

        return version.VersionString

    def incRefCount(self):
        self.refCount += 1

    def decRefCount(self):
        self.refCount -= 1


provider = None
with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationImport']).ClaimOperationAndCatch() as op:
    properties = op.Properties
    someValue = properties['de.uni_stuttgart.Voxie.Example.RawTestImporter.SomeValue'].getValue('x')
    print('SomeValue: ' + str(someValue))
    provider = TomographyRawData2DAccessorOperationsImpl(context.bus)
    clientManager.objects[provider.path] = provider
    providerTuple = (context.bus.get_unique_name(
    ) if not instance._context.isPeerToPeerConnection else '', dbus.ObjectPath(provider.path))
    with instance.CreateTomographyRawData2DAccessor(providerTuple, {}, DBusObject_handleMessages=True) as data:
        with data.GetCurrentVersion() as version:  # TODO: What should be done with versions here?
            op.Finish(data, version)

if provider is not None:
    print('RefCount: %d' % provider.refCount, flush=True)
while provider is not None and provider.refCount > 0:
    context.iteration()
    print('RefCount: %d' % provider.refCount, flush=True)
print('Terminating', flush=True)
