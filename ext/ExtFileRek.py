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

# dtype = np.dtype('<f4') # Not working currently
dtype = np.dtype('<u2')
# dtype = np.dtype('<u1')

# Note: On importing, vgstudio by default mirrors Y and Z. Also do that.

doRescaling = dtype.kind != 'f'
if doRescaling:
    info = np.iinfo(dtype)

'''
exporter = instance.Components.GetComponent('de.uni_stuttgart.Voxie.ComponentType.Exporter', 'de.uni_stuttgart.Voxie.FileFormat.TomographyRawData.Rek.Export').CastTo('de.uni_stuttgart.Voxie.Exporter')
for node in instance.ListNodes():
  if node.Prototype.NodeKind != 'de.uni_stuttgart.Voxie.NodeKind.Data':
    continue
  node = node.CastTo('de.uni_stuttgart.Voxie.DataNode')
  with exporter.StartExport(node.Data, node.FileName + '.rek') as res:
    res.Operation.WaitFor()
'''

# See ctcontrol repo Vgl/RekHeader.cs

rekFields = [
    ('H', 'ImageWidth'),
    ('H', 'ImageHeight'),
    ('H', 'BitsPerPixel'),
    ('H', 'ImageDepth'),
    ('H', 'HeaderSize'),
    ('H', 'MajorVersion'),
    ('H', 'MinorVersion'),
    ('H', 'Revision'),
    ('4x', None),
    ('I', 'DistanceSourceDetectorUM'),
    ('I', 'DistanceSourceAxisUM'),
    ('I', 'OverallProjectionCount'),
    ('I', 'DetectorWidthUM'),
    ('I', 'DetectorHeightUM'),
    ('I', 'DetectorWidthPixel'),
    ('I', 'DetectorHeightPixel'),
    ('4x', None),
    ('I', 'ReconstructionZMin'),
    ('I', 'ReconstructionZMax'),
    ('508x', None),
    ('I', 'ReconstructionWidth'),
    ('I', 'ReconstructionHeight'),
    ('I', 'ReconstructionDepth'),
    ('f', 'ReconstructionScalingFactor'),
    ('f', 'VoxelSizeXYUM'),
    ('f', 'VoxelSizeZUM'),
    ('4x', None),
    ('H', 'Unknown3'),
    ('H', 'Unknown4'),
    ('H', 'Unknown5'),
    ('H', 'Unknown6'),
    ('84x', None),
    ('I', 'Unknown7'),
    ('I', 'Unknown8'),
    ('328x', None),
    ('1024x', None),
]

structFormat = '@'
fieldNames = ''
fieldDefaults = []
fields = []
for ty, name in rekFields:
    structFormat += ty
    if name is not None:
        defVal = struct.unpack(ty, struct.calcsize(ty) * b'\0')[0]
        fieldNames += name + ' '
        fieldDefaults.append(defVal)
        fields.append((name, typing.Any, dataclasses.field(default=defVal)))
rekHeaderStruct = struct.Struct(structFormat)
# RekHeader = collections.namedtuple('RekHeader', fieldNames, defaults=fieldDefaults)
RekHeader = dataclasses.make_dataclass('RekHeader', fields)
if rekHeaderStruct.size != 2048:
    raise Exception('rekHeaderStruct.size != 2048')

# print(rekHeaderStruct.size)
# data = open('/tmp/raspberry_pi_v03.rek', 'rb').read(rekHeaderStruct.size)
# print(data)
# h = RekHeader._make(rekHeaderStruct.unpack(data))
# print(h)

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

if args.voxie_action != 'Export' and args.voxie_action != 'Import':
    raise Exception('Invalid operation: ' + repr(args.voxie_action))

if args.voxie_action == 'Export':
    with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationExport']).ClaimOperationAndCatch() as op:
        filename = op.Filename
        data = op.Data.CastTo('de.uni_stuttgart.Voxie.VolumeData')
        data = data.CastTo('de.uni_stuttgart.Voxie.VolumeDataVoxel')

        # print('Export %s %s' % (filename, data))

        shape = data.ArrayShape
        spacing = data.GridSpacing

        if spacing[0] != spacing[1]:
            raise Exception('Spacing in X and Y direction not the same')

        with data.GetBufferReadonly() as inputArray:
            if not doRescaling:
                progressMax = 0
                progressSave = 1
            else:
                progressMax = 0.2
                progressSave = 0.8

            scalingFactor = 1
            if doRescaling and shape[0] > 0 and shape[1] > 0 and shape[2] > 0:
                maxVal = np.max(inputArray[:, :, 0])
                for z in range(1, shape[2]):
                    op.SetProgress(progressMax * z / shape[2])
                    maxVal = np.maximum(maxVal, np.max(inputArray[:, :, z]))
                scalingFactor = info.max / maxVal
                # print(maxVal, scalingFactor)
            op.SetProgress(progressMax)

            header = RekHeader()
            header.ImageWidth = shape[0]
            header.ImageHeight = shape[1]
            header.ImageDepth = shape[2]
            header.BitsPerPixel = dtype.itemsize * 8
            header.HeaderSize = 2048
            header.MajorVersion = 2
            header.MinorVersion = 4
            header.Revision = 0  # TODO: Use 5?
            header.ReconstructionZMin = 0
            header.ReconstructionZMax = shape[2] - 1
            header.ReconstructionWidth = shape[0]
            header.ReconstructionHeight = shape[1]
            header.ReconstructionDepth = shape[2]
            header.ReconstructionScalingFactor = scalingFactor
            header.VoxelSizeXYUM = spacing[0] * 1e6
            header.VoxelSizeZUM = spacing[2] * 1e6
            # header.DetectorWidthPixel = 992
            # header.DetectorHeightPixel = 992
            # TODO: Are there more values needed for FP?
            # header.Unknown3 = 5
            # header.Unknown4 = 71
            # header.Unknown5 = 10
            # header.Unknown6 = 10
            # header.Unknown7 = 2
            # header.Unknown8 = 1500

            # print(header)
            data = rekHeaderStruct.pack(*dataclasses.astuple(header))
            # print(data)
            with open(filename, 'wb') as file:
                file.write(data)
                for z0 in range(shape[2]):
                    z = shape[2] - 1 - z0  # Mirror z
                    op.SetProgress(progressMax + progressSave * z0 / shape[2])
                    slice = inputArray[:, :, z]
                    slice = slice[:, ::-1]  # Mirror y
                    if doRescaling:
                        slice = slice * scalingFactor
                        slice = np.clip(slice, info.min, info.max)
                    slice = np.array(slice, dtype=dtype, order='F')
                    file.write(slice.tobytes(order='F'))
            op.SetProgress(progressMax + progressSave)

        op.Finish()
else:
    with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationImport']).ClaimOperationAndCatch() as op:
        raise Exception('Not implemented')

context.client.destroy()
