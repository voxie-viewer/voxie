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

import numpy as np
import voxie
import dbus
import sys
import struct
import collections
import dataclasses
import typing

# See https://bamresearch.github.io/aRTist-handbook/bamct_file_format.html

bamFields = [
    ('12s', 'Filename'),
    ('I', 'Rows'),
    ('I', 'Columns'),
    ('I', 'AngularSteps'),
    ('i', 'AngularSteps180'),
    ('I', 'Slices'),
    ('I', 'NumberOfTranslations'),
    ('I', 'NumberOfIntermediateAngles'),
    ('I', 'NumberOfMarginPoints'),
    ('I', 'NumberOfDetectors'),
    ('I', 'BytesPerPixel'),
    ('I', 'NumberOfDiodesPerDetector'),
    ('24x', None),
    ('f', 'MinimumAttenuationCoefficient'),
    ('f', 'MaximumAttenuationCoefficient'),
    ('f', 'TotalNumberOfPhotons'),
    ('f', 'MeasurementTimePerPoint'),
    ('f', 'VelocityNumber'),
    ('f', 'StartAngle'),
    ('f', 'ScanCentrePoint'),
    ('f', 'ScanLengthWithoutRamp'),
    ('f', 'VoxelSize'),
    ('f', 'StageElevation'),
    ('f', 'ElevationIncrement'),
    ('f', 'SOD'),
    ('f', 'SDD'),
    ('f', 'SourceElevation'),
    ('f', 'SourceCentre'),
    ('f', 'SourceDistance'),
    ('f', 'DetectorElevation'),
    ('f', 'DetectorCentre'),
    ('f', 'DetectorDistance'),
    ('f', 'SpacerElevation'),
    ('f', 'ObjectWeight'),
    ('f', 'BeamElevation'),
    ('f', 'CollimatorWidth'),
    ('f', 'CollimatorHeight'),
    ('f', 'AngularStepSizeBetweenImages'),
    ('f', 'PCDClearTimePerPoint'),
    ('f', 'DensityCorrectionFactor'),
    ('f', 'ROICentre'),
    ('f', 'ROIDistance'),
    ('4x', None),
    ('8s', 'SourceType'),
    ('8s', 'SourceEnergy'),
    ('8s', 'SourceIntensity'),
    ('8s', 'DetectorType'),
    ('80s', 'SampleName'),
    ('4s', 'ProgramID'),
    ('16s', 'MeasurementStartTime'),
    ('16s', 'MeasurementStopTime'),
    ('16s', 'TimeAndDateOfLastEdit'),
    ('12s', 'LookUpTableFile1'),
    ('12s', 'LookUpTableFile2'),
    ('12s', 'LookUpTableFile3'),
    ('12s', 'TubeFilter'),
    ('96s', 'ProcessingSteps'),
    ('4x', None),
]

structFormat = '@'
fieldNames = ''
fieldDefaults = []
fields = []
offset = 0
for ty, name in bamFields:
    structFormat += ty
    # print(name, offset, struct.calcsize(ty))
    offset += struct.calcsize(ty)
    if name is not None:
        defVal = struct.unpack(ty, struct.calcsize(ty) * b'\0')[0]
        fieldNames += name + ' '
        fieldDefaults.append(defVal)
        fields.append((name, typing.Any, dataclasses.field(default=defVal)))
bamHeaderStruct = struct.Struct(structFormat)
# BamHeader = collections.namedtuple('BamHeader', fieldNames, defaults=fieldDefaults)
BamHeader = dataclasses.make_dataclass('BamHeader', fields)
if bamHeaderStruct.size != 512:
    raise Exception('bamHeaderStruct.size != 512')


imageKind = {
}


class TomographyRawData2DAccessorOperationsImpl(voxie.DBusExportObject):
    def __init__(self, conn, *, imageKind, metadata, geometries, imageCount, imageShape, file, offset, header):
        voxie.DBusExportObject.__init__(
            self, ['de.uni_stuttgart.Voxie.TomographyRawData2DAccessorOperations'], context=context)
        self.path = context.nextId(
            '/de/uni_stuttgart/Voxie/TomographyRawData2DAccessorOperationsImpl')
        self.refCount = 0
        self.imageKind = imageKind
        self.metadata = metadata
        self.geometries = geometries
        self.imageCount = imageCount
        self.imageShape = imageShape
        self.file = file
        self.offset = offset
        self.header = header
        self.geometryNames = [geom['Name'] for geom in geometries]
        self.geometryByName = {geom['Name']: geom['Data'] for geom in geometries}

        self.add_to_connection(conn, self.path)

    def GetNumberOfImages(self, stream, options):
        if stream != '':
            raise Exception('Invalid stream name: ' + repr(stream))
        return self.imageCount

    def GetMetadata(self, options):
        return self.metadata

    def GetAvailableImageKinds(self, options):
        return [self.imageKind]

    def GetAvailableStreams(self, options):
        return ['']

    def GetAvailableGeometryTypes(self, options):
        return self.geometryNames

    def GetGeometryData(self, geometryType, options):
        if geometryType not in self.geometryNames:
            raise Exception('Invalid geometry type: ' + repr(geometryType))
        return self.geometryByName[geometryType]

    def GetImageShape(self, stream, id, options):
        if stream != '':
            raise Exception('Invalid stream name: ' + repr(stream))
        if id < 0 or id >= self.imageCount:
            raise Exception('Invalid image ID: ' + id)
        return self.imageShape

    def GetPerImageMetadata(self, stream, id, options):
        if stream != '':
            raise Exception('Invalid stream name: ' + repr(stream))
        if id < 0 or id >= self.imageCount:
            raise Exception('Invalid image ID: ' + id)
        # TODO: ?
        return {}

    def ReadImages(self, imageKind, images, inputRegionStart, rawData, firstOutputImageId, outputRegionStart, regionSize, options):
        firstOutputImageId = int(firstOutputImageId)
        count = len(images)
        for stream, id in images:
            if stream != '':
                raise Exception('Invalid stream name: ' + repr(stream))
            if id < 0 or id >= self.imageCount:
                raise Exception('Invalid image ID: ' + id)

        if rawData._busName != context.busName:
            raise Exception('rawData._busName != context.busName')

        with rawData.CreateUpdate() as update, rawData.GetBufferWritable(update) as buffer:
            for i in range(count):
                stream, id = images[i]
                region = buffer[outputRegionStart[0]:outputRegionStart[0] + regionSize[0],
                                outputRegionStart[1]:outputRegionStart[1] + regionSize[1],
                                firstOutputImageId + i]
                pos = self.offset + self.imageShape[1] * self.imageShape[0] * id
                file.seek(pos)
                data = self.file.read(self.imageShape[1] * self.imageShape[0] * self.header.BytesPerPixel)
                # TODO: Is this correct?
                # TODO: Mirror images? (in particular y direction)
                img = np.frombuffer(data, dtype=dtype).reshape((self.imageShape[1], self.imageShape[0])).transpose()
                region[:] = img[inputRegionStart[0]:inputRegionStart[0] + regionSize[0],
                                inputRegionStart[1]:inputRegionStart[1] + regionSize[1]]

            version = update.Finish()

        return version.VersionString

    def incRefCount(self):
        self.refCount += 1

    def decRefCount(self):
        self.refCount -= 1


# print(bamHeaderStruct.size)
# data = open('/tmp/New.bd', 'rb').read(bamHeaderStruct.size)
# print(data)
# h = BamHeader(*bamHeaderStruct.unpack(data))
# print(h)

args = voxie.parser.parse_args()
# context = voxie.VoxieContext(args)
context = voxie.VoxieContext(args, enableService=True)
instance = context.createInstance()

if args.voxie_action != 'Import':
    raise Exception('Invalid operation: ' + args.voxie_action)

provider = None

with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationImport']).ClaimOperationAndCatch() as op:
    filename = op.Filename

    # with open(filename, 'rb') as file:
    # TODO: Close file for volume files?
    file = open(filename, 'rb')
    if True:
        header_data = file.read(bamHeaderStruct.size)
        header = BamHeader(*bamHeaderStruct.unpack(header_data))
        # print(header, flush=True)

        if header.Filename[7:8] != b'.':
            raise Exception("header.Filename[7:8] != b'.'")

        ty = header.Filename[10:11]
        if ty == b'c':
            dataType = ('uint', 8)
        elif ty == b's':
            dataType = ('uint', 16)
        elif ty == b'i':
            dataType = ('uint', 32)
        elif ty == b'e':
            dataType = ('float', 32)
        else:
            raise Exception('Got unknown data type {!r}'.format(ty))

        endian = header.Filename[11:12]
        # TODO: consider endianess for header values
        if dataType[1] == 8:
            dataType += ('none',)
        elif endian == b's':
            dataType += ('little',)
        elif endian == b'x':
            dataType += ('big',)
        else:
            raise Exception('Got unknown data byte order {!r}'.format(endian))

        dtype_str = voxie.buffer.endianMap[dataType[2]] + voxie.buffer.typeNameMap[dataType[0]] + str(int(dataType[1]) // 8)
        dtype = np.dtype(dtype_str)

        dataType2 = (dataType[0], dataType[1], 'native')
        # dataType2 = ('float', 32, 'native')

        content_type = header.Filename[8:9]
        if content_type == b'b':  # tomogram
            arrayShape = [header.Rows, header.Columns, header.Slices]
            expectedSize = arrayShape

            gridSpacing = [header.VoxelSize * 1e-3, header.VoxelSize * 1e-3, header.VoxelSize * 1e-3]

            # TODO: ?
            volumeOrigin = [gridSpacing[0] * arrayShape[0], gridSpacing[1] * arrayShape[1], gridSpacing[2] * arrayShape[2]]

            if header.BytesPerPixel * 8 != dataType[1]:
                raise Exception('header.BytesPerPixel * 8 != dataType[1]')

            rowSize = header.Columns * header.BytesPerPixel
            offset = (512 + rowSize - 1) // rowSize * rowSize
            expectedFileSize = offset + expectedSize[0] * expectedSize[1] * expectedSize[2] * header.BytesPerPixel
            fileSize = file.seek(0, 2)
            if expectedFileSize != fileSize:
                raise Exception('Expected file size {!r}, got file size {!r}'.format(expectedFileSize, fileSize))

            # TODO: .vgi file contains "Mirror = 0 1 0 ". Also mirror some axis?
            # TODO: Is the order of axes correct?
            with instance.CreateVolumeDataVoxel(arrayShape, dataType2, volumeOrigin, gridSpacing) as resultData:
                with resultData.CreateUpdate() as update, resultData.GetBufferWritable(update) as buffer:
                    outData = buffer

                    op.ThrowIfCancelled()
                    for z in range(expectedSize[2]):
                        # TODO: Avoid memory allocations here?
                        pos = offset + arrayShape[0] * arrayShape[1] * header.BytesPerPixel * z
                        file.seek(pos)
                        data = file.read(arrayShape[0] * arrayShape[1] * header.BytesPerPixel)
                        img = np.frombuffer(data, dtype=dtype).reshape((arrayShape[0], arrayShape[1]))
                        outData[:, :, z] = img

                        op.ThrowIfCancelled()
                        op.SetProgress((z + 1) / expectedSize[2])
                    # TODO: Rescale data to from min(outData), max(outData) to MinimumAttenuationCoefficient, MaximumAttenuationCoefficient? (Would need float data)

                    version = update.Finish()
                with version:
                    op.Finish(resultData, version)
        elif content_type == b'd':  # raw data
            clientManager = voxie.clientimpl.ClientManagerImpl(context, context.bus)

            imageCount = header.AngularSteps

            rowSize = header.Columns * header.BytesPerPixel
            if header.Rows % imageCount != 0:
                raise Exception('header.Rows % imageCount != 0')
            imageShape = (header.Columns, header.Rows // imageCount)
            offset = (512 + rowSize - 1) // rowSize * rowSize
            expectedFileSize = offset + imageShape[0] * imageShape[1] * imageCount * header.BytesPerPixel
            fileSize = file.seek(0, 2)
            if expectedFileSize != fileSize:
                raise Exception('Expected file size {!r}, got file size {!r}'.format(expectedFileSize, fileSize))

            # TODO: Produce metadata and geometries?
            metadata = {}
            geometries = []

            provider = TomographyRawData2DAccessorOperationsImpl(context.bus, imageKind=imageKind, metadata=metadata, geometries=geometries, imageCount=imageCount, imageShape=imageShape, file=file, offset=offset, header=header)
            clientManager.objects[provider.path] = provider
            providerTuple = (context.bus.get_unique_name() if not instance._context.isPeerToPeerConnection else '', dbus.ObjectPath(provider.path))
            with instance.CreateTomographyRawData2DAccessor(providerTuple, {}, DBusObject_handleMessages=True) as data:
                with data.GetCurrentVersion() as version:
                    op.Finish(data, version)
        else:
            raise Exception('Got content type {!r}, expected "b" for tomogram or "d" for raw data'.format(content_type))


# if provider is not None:
#     print('RefCount: %d' % provider.refCount, flush=True)
while provider is not None and provider.refCount > 0:
    context.iteration()
    # print('RefCount: %d' % provider.refCount, flush=True)
# print('Terminating', flush=True)

context.client.destroy()
