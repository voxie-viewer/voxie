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
    inputData = context.makeObject(context.bus, context.busName, inputDataPath, [
                                   'de.uni_stuttgart.Voxie.VolumeDataVoxel'])
    outputPath = properties['de.uni_stuttgart.Voxie.Output'].getValue('o')
    planePath = properties['de.uni_stuttgart.Voxie.Filter.Rotate.Plane'].getValue('o')
    bbPath = properties['de.uni_stuttgart.Voxie.BoundingBoxData'].getValue('o')
    # print(inputProperties['de.uni_stuttgart.Voxie.MovableDataNode.Translation'].getValue('(ddd)'))
    # print(inputProperties['de.uni_stuttgart.Voxie.MovableDataNode.Rotation'].getValue('(dddd)'))

    setMissingDataToNaN = properties['de.uni_stuttgart.Voxie.Filter.Rotate.SetMissingDataToNaN'].getValue(
        'b')
    enableOverwriteVoxelSize = properties['de.uni_stuttgart.Voxie.Filter.Rotate.EnableOverwriteVoxelSize'].getValue(
        'b')
    overwriteVoxelSize = properties['de.uni_stuttgart.Voxie.Filter.Rotate.OverwriteVoxelSize'].getValue(
        'd')
    sizeRoundingMode = properties['de.uni_stuttgart.Voxie.SizeRoundingMode'].getValue(
        's')
    if sizeRoundingMode == 'de.uni_stuttgart.Voxie.SizeRoundingMode.Floor':
        sizeRounding = np.floor
    elif sizeRoundingMode == 'de.uni_stuttgart.Voxie.SizeRoundingMode.Round':
        sizeRounding = np.round
    elif sizeRoundingMode == 'de.uni_stuttgart.Voxie.SizeRoundingMode.Ceil':
        sizeRounding = np.ceil
    else:
        raise Exception('Unknown SizeRoundingMode: ' + repr(sizeRoundingMode))

    translationVolume = np.array(
        inputProperties['de.uni_stuttgart.Voxie.MovableDataNode.Translation'].getValue('(ddd)'))
    rotationVolume = voxie.Rotation(
        inputProperties['de.uni_stuttgart.Voxie.MovableDataNode.Rotation'].getValue('(dddd)'))
    translationPlane = np.array((0.0, 0.0, 0.0))
    rotationPlane = voxie.Rotation((1, 0, 0, 0))
    if planePath != dbus.ObjectPath('/'):
        planeProperties = pars[planePath]['Properties'].getValue('a{sv}')
        # print(planeProperties['de.uni_stuttgart.Voxie.Property.Plane.Origin'].getValue('(ddd)'))
        # print(planeProperties['de.uni_stuttgart.Voxie.Property.Plane.Orientation'].getValue('(dddd)'))
        translationPlane = np.array(
            planeProperties['de.uni_stuttgart.Voxie.Property.Plane.Origin'].getValue('(ddd)'))
        rotationPlane = voxie.Rotation(
            planeProperties['de.uni_stuttgart.Voxie.Property.Plane.Orientation'].getValue('(dddd)'))
    rotation = rotationVolume.inverse * rotationPlane

    pointType = instance.Components.GetComponent(
        'de.uni_stuttgart.Voxie.ComponentType.GeometricPrimitiveType', 'de.uni_stuttgart.Voxie.GeometricPrimitive.Point').CastTo('de.uni_stuttgart.Voxie.GeometricPrimitiveType')
    points = None
    if bbPath != dbus.ObjectPath('/'):
        bbDataPath = pars[bbPath]['Data'].getValue('o')
        bbData = context.makeObject(context.bus, context.busName, bbDataPath, [
                                    'de.uni_stuttgart.Voxie.GeometricPrimitiveData'])
        points = []
        for primitive in bbData.GetPrimitives(0, 2**64 - 1):
            type = primitive[1]
            primitiveValues = primitive[3]
            if type != pointType._objectPath:
                print('Warning: Unknown primitive:', type, file=sys.stderr)
                continue
            position = primitiveValues['Position'].getValue('(ddd)')
            points.append(np.array(position))
    # print(points)

    # factor = properties['de.uni_stuttgart.Voxie.Filter.Downsample.Factor'].getValue('x')

    origin = inputData.VolumeOrigin
    sizeOrig = inputData.ArrayShape
    spacingOrig = np.array(inputData.GridSpacing)
    # print (origin, sizeOrig, spacingOrig)

    # TODO: Clean up / document coordinate systems

    # posmin, posmax are in the new volume coordinate system

    posmin = posmax = None
    if points is None:
        for corner in [[0, 0, 0], [0, 0, 1], [0, 1, 0], [
                0, 1, 1], [1, 0, 0], [1, 0, 1], [1, 1, 0], [1, 1, 1]]:
            cpos = np.array(corner, dtype=np.double) * sizeOrig
            cpos = rotation.inverse * (origin + spacingOrig * cpos)
            if posmin is None:
                posmin = cpos
            if posmax is None:
                posmax = cpos
            posmin = np.minimum(posmin, cpos)
            posmax = np.maximum(posmax, cpos)
            # print (cpos)
    else:
        if len(points) == 0:
            raise Exception('Got a bounding box input but no points in it')
        for point in points:
            cpos = rotationPlane.inverse * (point - translationVolume)
            if posmin is None:
                posmin = cpos
            if posmax is None:
                posmax = cpos
            posmin = np.minimum(posmin, cpos)
            posmax = np.maximum(posmax, cpos)
            # print (cpos)
    # print (posmin)
    # print (posmax)

    voxelSize = np.mean(spacingOrig)
    if enableOverwriteVoxelSize:
        voxelSize = overwriteVoxelSize

    sizeOutput = np.uint64(sizeRounding((posmax - posmin) / voxelSize))

    # print (voxelSize, sizeOutput)

    dataType = ('float', 32, 'native')  # TODO?

    newOrigin = posmin
    # TODO: use translationPlane? (to rotate around the plane origin)
    newOrigin = posmin + rotationPlane.inverse * translationVolume
    with instance.CreateVolumeDataVoxel(sizeOutput, dataType, newOrigin, (voxelSize, voxelSize, voxelSize)) as data:
        with data.CreateUpdate() as update, data.GetBufferWritable(update) as buffer:
            with instance.CreateImage((sizeOutput[0], sizeOutput[1]), 1, ('float', 32, 'native')) as image, image.GetBufferReadonly() as ibuffer:
                options = {}
                # options['Interpolation'] = voxie.Variant('s', 'NearestNeighbor')
                options['Interpolation'] = voxie.Variant('s', 'Linear')

                zCount = sizeOutput[2]
                for z in range(0, zCount):
                    op.ThrowIfCancelled()
                    pos0 = (posmin[0], posmin[1], (z + 0.5)
                            * voxelSize + posmin[2])
                    pos = rotation * pos0
                    # print ('ExtractSlice', inputData, pos, tuple(map(float, rotation.quaternion.value)), (sizeOutput[0], sizeOutput[1]), (voxelSize, voxelSize), image, options)
                    instance.Utilities.ExtractSlice(inputData, pos, tuple(map(
                        float, rotation.quaternion.value)), (sizeOutput[0], sizeOutput[1]), (voxelSize, voxelSize), image, options)
                    outputSlice = buffer[:, :, z]
                    outputSlice[:] = ibuffer[:, :, 0]
                    if not setMissingDataToNaN:
                        # TODO: Do this using a parameter to ExtractSlice and
                        # avoid setting data which is nan in the input data to
                        # 0
                        outputSlice[np.isnan(outputSlice)] = 0
                    op.SetProgress((z + 1) / zCount)
            version = update.Finish()

        result = {}
        result[outputPath] = {
            'Data': voxie.Variant('o', data._objectPath),
            'DataVersion': voxie.Variant('o', version._objectPath),
        }
        op.Finish(result)
