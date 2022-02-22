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
import random
import scipy.ndimage

detShiftXMinPix = -30.0
detShiftXMaxPix = 30.0
detShiftYMinPix = -30.0
detShiftYMaxPix = 30.0
borderCutoff = 50

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args, enableService=True)
instance = context.createInstance()

if args.voxie_action != 'RunFilter':
    raise Exception('Invalid operation: ' + args.voxie_action)

clientManager = voxie.clientimpl.ClientManagerImpl(context, context.bus)


class TomographyRawData2DAccessorOperationsImpl(voxie.DBusExportObject):
    def __init__(self, conn, *, backend, seed, shiftFirstImage):
        voxie.DBusExportObject.__init__(
            self, ['de.uni_stuttgart.Voxie.TomographyRawData2DAccessorOperations'], context=context)
        self.path = context.nextId(
            '/de/uni_stuttgart/Voxie/TomographyRawData2DAccessorOperationsImpl')
        self.refCount = 0
        self.backend = backend
        self.imageKind = {
            "Corrections": {
                "BadPixel": True,
                "FluxNormalization": False,
                "NormalizeLevel": True,
                "Shading": True,
            },
            "Description": "Intensity image with bad pixel and shading corrected",
            "Dimension": "Intensity"
        }
        self.imageShape = self.backend.GetImageShape('', 0)
        self.bufferData = instance.CreateTomographyRawData2DRegular(self.imageShape, 1, ('float', 32, 'native'))
        self.buffer = self.bufferData.GetBufferReadonly()

        self.movementReferenceData = self.backend.GetGeometryData('MovementReferenceData')
        self.count = self.backend.GetNumberOfImages('')
        self.refIDs = []
        self.refPositions = []
        self.seed = seed
        self.shiftFirstImage = shiftFirstImage
        random.seed(seed)
        doShiftImage = shiftFirstImage
        for seq in self.movementReferenceData['Sequences']:
            for image in seq['Images']:
                if image['ImageReference']['Stream'] != '':
                    raise Exception("image['ImageReference']['Stream'] != ''")
                id = image['ImageReference']['ImageID']
                if doShiftImage:
                    posX = (detShiftXMinPix + (detShiftXMaxPix - detShiftXMinPix) * random.random())
                    posY = (detShiftYMinPix + (detShiftYMaxPix - detShiftYMinPix) * random.random())
                else:
                    posX = 0.0
                    posY = 0.0
                self.refIDs.append(id)
                self.refPositions.append((posX, posY))
                doShiftImage = True
        self.refIDs = np.array(self.refIDs)
        self.refPositions = np.array(self.refPositions)
        # print(self.refIDs, flush=True)
        # print(self.refPositions, flush=True)

        self.add_to_connection(conn, self.path)

    def GetNumberOfImages(self, stream, options):
        return self.backend.GetNumberOfImages(stream, options)

    def GetMetadata(self, options):
        metadata = self.backend.GetMetadata(options)
        metadata['Info']['DetectorPixels'][0] -= 2 * borderCutoff
        metadata['Info']['DetectorPixels'][1] -= 2 * borderCutoff
        if 'AppliedChanges' not in metadata['Info']:
            metadata['Info']['AppliedChanges'] = {}
        metadata['Info']['AppliedChanges']['ArtificalShift'] = {
            'Settings': {
                'Seed': self.seed,
                'ShiftFirstImage': self.shiftFirstImage,
            },
            'ReferenceIDs': list(map(float, self.refIDs)),
            'ReferenceShifts': list(map(lambda l: list(map(float, l)), self.refPositions)),
        }
        return metadata

    def GetAvailableImageKinds(self, options):
        return self.backend.GetAvailableImageKinds(options)

    def GetAvailableStreams(self, options):
        return self.backend.GetAvailableStreams(options)

    def GetAvailableGeometryTypes(self, options):
        return self.backend.GetAvailableGeometryTypes(options)

    def GetGeometryData(self, geometryType, options):
        return self.backend.GetGeometryData(geometryType, options)

    def GetImageShape(self, stream, id, options):
        # return self.backend.GetImageShape(stream, id, options)
        # return self.imageShape
        return (self.imageShape[0] - 2 * borderCutoff, self.imageShape[1] - 2 * borderCutoff)

    def GetPerImageMetadata(self, stream, id, options):
        return self.backend.GetPerImageMetadata(stream, id, options)

    def ReadImages(self, imageKind, images, inputRegionStart, rawData, firstOutputImageId, outputRegionStart, regionSize, options):
        with rawData.CreateUpdate() as update, rawData.GetBufferWritable(update) as outBuf:
            for i, (imageStream, imageId) in enumerate(images):
                # print('QQ', imageStream, repr(imageId), flush=True)
                # TODO: Support for different image sizes?
                self.backend.ReadImages(imageKind, [(imageStream, imageId)], (0, 0), (self.bufferData._busName, self.bufferData._objectPath), 0, (0, 0), self.imageShape, options)
                img = self.buffer[:, :, 0]
                if imageStream == '':
                    shiftX = np.interp(imageId, self.refIDs, self.refPositions[:, 0])
                    shiftY = np.interp(imageId, self.refIDs, self.refPositions[:, 1])
                    # print(imageId, shiftX, shiftY, flush=True)

                    img = scipy.ndimage.shift(img, [shiftX, shiftY], order=1, mode='constant', cval=0)
                outBuf[outputRegionStart[0]:outputRegionStart[0] + regionSize[0], outputRegionStart[1]:outputRegionStart[1] + regionSize[1], firstOutputImageId + i] = img[borderCutoff + inputRegionStart[0]:borderCutoff + inputRegionStart[0] + regionSize[0], borderCutoff + inputRegionStart[1]:borderCutoff + inputRegionStart[1] + regionSize[1]]
            with update.Finish() as version:
                versionStr = version.VersionString
            return versionStr

    def incRefCount(self):
        self.refCount += 1

    def decRefCount(self):
        self.refCount -= 1


with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationRunFilter']).ClaimOperationAndCatch() as op:
    filterPath = op.FilterObject
    pars = op.Parameters
    # print (pars)
    properties = pars[filterPath._objectPath]['Properties'].getValue('a{sv}')
    # print (properties)
    inputPath = properties['de.uni_stuttgart.Voxie.Input'].getValue('o')
    inputDataPath = pars[inputPath]['Data'].getValue('o')
    inputVersionPath = pars[inputPath]['DataVersion'].getValue('o')
    outputPath = properties['de.uni_stuttgart.Voxie.Output'].getValue('o')

    seed = properties['de.uni_stuttgart.Voxie.Example.Filter.RawDataAddRandomShift.Seed'].getValue('x')
    shiftFirstImage = properties['de.uni_stuttgart.Voxie.Example.Filter.RawDataAddRandomShift.ShiftFirstImage'].getValue('b')

    backend = context.makeObject(context.bus, context.busName, inputDataPath, ['de.uni_stuttgart.Voxie.TomographyRawData2DAccessor'])
    provider = TomographyRawData2DAccessorOperationsImpl(context.bus, backend=backend, seed=seed, shiftFirstImage=shiftFirstImage)

    clientManager.objects[provider.path] = provider
    providerTuple = (context.bus.get_unique_name() if not instance._context.isPeerToPeerConnection else '', dbus.ObjectPath(provider.path))
    with instance.CreateTomographyRawData2DAccessor(providerTuple, {}, DBusObject_handleMessages=True) as data:
        with data.GetCurrentVersion() as version:
            result = {}
            result[outputPath] = {
                'Data': voxie.Variant('o', data),
                'DataVersion': voxie.Variant('o', version),
            }
            op.Finish(result)

# if provider is not None:
#     print('RefCount: %d' % provider.refCount, flush=True)
while provider is not None and provider.refCount > 0:
    context.iteration()
    # print('RefCount: %d' % provider.refCount, flush=True)
# print('Terminating', flush=True)
