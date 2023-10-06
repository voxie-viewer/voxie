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

import scipy.interpolate

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

if args.voxie_action != 'RunFilter':
    raise Exception('Invalid operation: ' + repr(args.voxie_action))

with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationRunFilter']).ClaimOperationAndCatch() as op:
    filterPath = op.FilterObject
    pars = op.Parameters
    # print (pars)
    properties = pars[filterPath._objectPath]['Properties'].getValue('a{sv}')
    # print (properties)
    inputPath = properties['de.uni_stuttgart.Voxie.Input'].getValue('o')
    if inputPath == dbus.ObjectPath('/'):
        raise Exception('No input volume object connected')
    inputProperties = pars[inputPath]['Properties'].getValue('a{sv}')
    inputDataPath = pars[inputPath]['Data'].getValue('o')
    inputDataVoxel = context.makeObject(context.bus, context.busName, inputDataPath, [
                                        'de.uni_stuttgart.Voxie.VolumeDataVoxel'])

    inputXPath = properties['de.uni_stuttgart.Voxie.Filter.ShiftVolume.ShiftAmount.X'].getValue(
        'o')
    if inputXPath == dbus.ObjectPath('/'):
        raise Exception('No inputX volume object connected')
    inputXProperties = pars[inputXPath]['Properties'].getValue('a{sv}')
    inputXDataPath = pars[inputXPath]['Data'].getValue('o')
    inputXDataVoxel = context.makeObject(context.bus, context.busName, inputXDataPath, [
                                         'de.uni_stuttgart.Voxie.VolumeDataVoxel'])
    if inputXDataVoxel[()].shape != inputDataVoxel[()].shape:
        raise Exception('Invalid shape for shift X')

    inputYPath = properties['de.uni_stuttgart.Voxie.Filter.ShiftVolume.ShiftAmount.Y'].getValue(
        'o')
    if inputYPath == dbus.ObjectPath('/'):
        raise Exception('No inputY volume object connected')
    inputYProperties = pars[inputYPath]['Properties'].getValue('a{sv}')
    inputYDataPath = pars[inputYPath]['Data'].getValue('o')
    inputYDataVoxel = context.makeObject(context.bus, context.busName, inputYDataPath, [
                                         'de.uni_stuttgart.Voxie.VolumeDataVoxel'])
    if inputYDataVoxel[()].shape != inputDataVoxel[()].shape:
        raise Exception('Invalid shape for shift Y')

    inputZPath = properties['de.uni_stuttgart.Voxie.Filter.ShiftVolume.ShiftAmount.Z'].getValue(
        'o')
    if inputZPath == dbus.ObjectPath('/'):
        raise Exception('No inputZ volume object connected')
    inputZProperties = pars[inputZPath]['Properties'].getValue('a{sv}')
    inputZDataPath = pars[inputZPath]['Data'].getValue('o')
    inputZDataVoxel = context.makeObject(context.bus, context.busName, inputZDataPath, [
                                         'de.uni_stuttgart.Voxie.VolumeDataVoxel'])
    if inputZDataVoxel[()].shape != inputDataVoxel[()].shape:
        raise Exception('Invalid shape for shift Z')

    outputPath = properties['de.uni_stuttgart.Voxie.Output'].getValue('o')

    spacing = inputDataVoxel.GridSpacing

    with op.GetOutputVolumeDataVoxelLike(outputPath, inputDataVoxel) as data:
        with data.CreateUpdate() as update, data.GetBufferWritable(update) as buffer:
            points = (np.arange(data[()].shape[0]), np.arange(
                data[()].shape[1]), np.arange(data[()].shape[2]))
            shiftGrid = np.mgrid[:data[()].shape[0], :data[()].shape[1], :1]
            shiftGrid = shiftGrid.transpose([1, 2, 3, 0])
            shiftGrid = np.array(shiftGrid, dtype=np.float64)
            shift = np.zeros(shiftGrid.shape, shiftGrid.dtype)
            zCount = data[()].shape[2]
            for z in range(0, zCount):
                # print(shift.shape)
                shift[:] = shiftGrid
                shift[:, :, 0, 0] -= inputXDataVoxel[:, :, z] / spacing[0]
                shift[:, :, 0, 1] -= inputYDataVoxel[:, :, z] / spacing[1]
                shift[:, :, 0, 2] += z
                shift[:, :, 0, 2] -= inputZDataVoxel[:, :, z] / spacing[2]
                # buffer[:, :, z] = inputDataVoxel[:, :, z]
                res = scipy.interpolate.interpn(
                    points, inputDataVoxel[()], shift, bounds_error=False, fill_value=0)
                # print (res.shape)
                buffer[:, :, z] = res[:, :, 0]
                op.SetProgress((z + 1) / zCount)
            version = update.Finish()

        result = {}
        result[outputPath] = {
            'Data': voxie.Variant('o', data._objectPath),
            'DataVersion': voxie.Variant('o', version._objectPath),
        }
        op.Finish(result)
