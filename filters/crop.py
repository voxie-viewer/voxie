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

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

if args.voxie_action != 'RunFilter':
    raise Exception('Invalid operation: ' + args.voxie_action)

with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationRunFilter']).ClaimOperationAndCatch() as op:
    inputData = op.GetInputData('de.uni_stuttgart.Voxie.Input').CastTo(
        'de.uni_stuttgart.Voxie.VolumeData')
    inputProperties = op.ParametersCached[op.Properties['de.uni_stuttgart.Voxie.Input'].getValue(
        'o')]['Properties'].getValue('a{sv}')
    bbData = op.GetInputData('de.uni_stuttgart.Voxie.BoundingBoxData').CastTo(
        'de.uni_stuttgart.Voxie.GeometricPrimitiveData')
    outputPath = op.Properties['de.uni_stuttgart.Voxie.Output'].getValue('o')

    setMissingDataToNaN = op.Properties['de.uni_stuttgart.Voxie.Filter.Crop.SetMissingDataToNaN'].getValue(
        'b')
    sizeRoundingMode = op.Properties['de.uni_stuttgart.Voxie.SizeRoundingMode'].getValue(
        's')
    if sizeRoundingMode == 'de.uni_stuttgart.Voxie.SizeRoundingMode.Floor':
        sizeRounding = np.floor
    elif sizeRoundingMode == 'de.uni_stuttgart.Voxie.SizeRoundingMode.Round':
        sizeRounding = np.round
    elif sizeRoundingMode == 'de.uni_stuttgart.Voxie.SizeRoundingMode.Ceil':
        sizeRounding = np.ceil
    else:
        raise Exception('Unknown SizeRoundingMode: ' + repr(sizeRoundingMode))

    inputDataVoxel = inputData.CastTo('de.uni_stuttgart.Voxie.VolumeDataVoxel')

    # TODO: Use this (and probably set it on the output)
    # translationVolume = np.array(inputProperties["de.uni_stuttgart.Voxie.MovableDataNode.Translation"].getValue("(ddd)"))
    # rotationVolume = voxie.Rotation (inputProperties["de.uni_stuttgart.Voxie.MovableDataNode.Rotation"].getValue("(dddd)"))

    # TODO: Move bounding box code somewhere else
    pointType = instance.Components.GetComponent(
        'de.uni_stuttgart.Voxie.ComponentType.GeometricPrimitiveType', 'de.uni_stuttgart.Voxie.GeometricPrimitive.Point').CastTo('de.uni_stuttgart.Voxie.GeometricPrimitiveType')
    points = []
    for primitive in bbData.GetPrimitives(0, 2**64 - 1):
        ptype = primitive[1]
        primitiveValues = primitive[3]
        if ptype != pointType._objectPath:
            print('Warning: Unknown primitive:', ptype, file=sys.stderr)
            continue
        position = primitiveValues['Position'].getValue('(ddd)')
        points.append(np.array(position))
    # print(points)

    posmin = posmax = None
    if len(points) == 0:
        raise Exception('Got a bounding box input but no points in it')
    for cpos in points:
        if posmin is None:
            posmin = cpos
        if posmax is None:
            posmax = cpos
        posmin = np.minimum(posmin, cpos)
        posmax = np.maximum(posmax, cpos)
    # print (posmin)
    # print (posmax)

    origin = inputData.VolumeOrigin
    sizeOrig = np.int64(inputDataVoxel.ArrayShape)
    voxelSize = np.array(inputDataVoxel.GridSpacing)
    # print (origin, sizeOrig, spacingOrig)

    # Position of new volume relative to old volume, in voxels
    posminVoxel = -np.int64(sizeRounding(-(posmin - origin) / voxelSize))
    posmaxVoxel = np.int64(sizeRounding((posmax - origin) / voxelSize))
    sizeOutput = posmaxVoxel - posminVoxel

    # print (voxelSize, sizeOutput)

    newOrigin = posminVoxel * voxelSize + origin
    with instance.CreateVolumeDataVoxel(sizeOutput, inputData.DataType, newOrigin, voxelSize) as data:
        with data.CreateUpdate() as update, data.GetBufferWritable(update) as outputBuffer:
            # TODO: do this with better performance?
            zCount = data[:].shape[2]
            for z in range(0, zCount):
                op.ThrowIfCancelled()
                if setMissingDataToNaN:
                    outputBuffer.array[:, :, z] = np.nan
                else:
                    outputBuffer.array[:, :, z] = 0
                op.SetProgress((z + 1) / zCount / 2)

            xMinOld = np.clip(posminVoxel[0], 0, sizeOrig[0])
            xMaxOld = np.clip(
                posminVoxel[0] + data[:].shape[0], 0, sizeOrig[0])
            yMinOld = np.clip(posminVoxel[1], 0, sizeOrig[1])
            yMaxOld = np.clip(
                posminVoxel[1] + data[:].shape[1], 0, sizeOrig[1])

            xMinNew = xMinOld - posminVoxel[0]
            xMaxNew = xMaxOld - posminVoxel[0]
            yMinNew = yMinOld - posminVoxel[1]
            yMaxNew = yMaxOld - posminVoxel[1]

            for z in range(0, zCount):
                op.ThrowIfCancelled()
                zOld = z + posminVoxel[2]
                if zOld < 0 or zOld >= sizeOrig[2]:
                    continue
                # print (xMinOld, xMaxOld, yMinOld, yMaxOld, zOld, posminVoxel, posmaxVoxel, sizeOrig)
                outputBuffer.array[xMinNew:xMaxNew, yMinNew:yMaxNew,
                                   z] = inputDataVoxel[xMinOld:xMaxOld, yMinOld:yMaxOld, zOld]
                op.SetProgress((z + 1) / zCount / 2 + 0.5)
            version = update.Finish()

        result = {}
        result[outputPath] = {
            'Data': voxie.Variant('o', data._objectPath),
            'DataVersion': voxie.Variant('o', version._objectPath),
        }
        op.Finish(result)
        version._referenceCountingObject.destroy()
