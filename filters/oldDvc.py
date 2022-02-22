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

__author__ = "Pouya Shahidi"

import numpy as np
import voxie
import dbus
import math
import time
import oldDvcImpl as dvc
args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

if args.voxie_action != 'RunFilter':
    raise Exception('Invalid operation: ' + args.voxie_action)

with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationRunFilter']).ClaimOperationAndCatch() as op:

    outputPathDX = op.Properties['de.uni_stuttgart.Voxie.Filter.DVC.Output.DisplacementX'].getValue(
        'o')
    outputPathDY = op.Properties['de.uni_stuttgart.Voxie.Filter.DVC.Output.DisplacementY'].getValue(
        'o')
    outputPathDZ = op.Properties['de.uni_stuttgart.Voxie.Filter.DVC.Output.DisplacementZ'].getValue(
        'o')
    outputPathCorrMax = op.Properties['de.uni_stuttgart.Voxie.Filter.DVC.Output.CorrelationMax'].getValue(
        'o')

    outputPathSTxx = op.Properties['de.uni_stuttgart.Voxie.Filter.DVC.Output.StrainTensorXX'].getValue(
        'o')
    outputPathSTyy = op.Properties['de.uni_stuttgart.Voxie.Filter.DVC.Output.StrainTensorYY'].getValue(
        'o')
    outputPathSTzz = op.Properties['de.uni_stuttgart.Voxie.Filter.DVC.Output.StrainTensorZZ'].getValue(
        'o')
    outputPathSTxy = op.Properties['de.uni_stuttgart.Voxie.Filter.DVC.Output.StrainTensorXY'].getValue(
        'o')
    outputPathSTxz = op.Properties['de.uni_stuttgart.Voxie.Filter.DVC.Output.StrainTensorXZ'].getValue(
        'o')
    outputPathSTyz = op.Properties['de.uni_stuttgart.Voxie.Filter.DVC.Output.StrainTensorYZ'].getValue(
        'o')

    # define correlation window size
    m = op.Properties['de.uni_stuttgart.Voxie.Filter.DVC.CX'].getValue('x')
    n = op.Properties['de.uni_stuttgart.Voxie.Filter.DVC.CY'].getValue('x')
    p = op.Properties['de.uni_stuttgart.Voxie.Filter.DVC.CZ'].getValue('x')

    stdvThreshold = op.Properties['de.uni_stuttgart.Voxie.Filter.DVC.StdvThreshold'].getValue(
        'd')

    roix = op.Properties['de.uni_stuttgart.Voxie.Filter.DVC.RoiX'].getValue(
        'x')
    roiy = op.Properties['de.uni_stuttgart.Voxie.Filter.DVC.RoiY'].getValue(
        'x')
    roiz = op.Properties['de.uni_stuttgart.Voxie.Filter.DVC.RoiZ'].getValue(
        'x')

    strideX = op.Properties['de.uni_stuttgart.Voxie.Filter.DVC.StrideX'].getValue(
        'x')
    strideY = op.Properties['de.uni_stuttgart.Voxie.Filter.DVC.StrideY'].getValue(
        'x')
    strideZ = op.Properties['de.uni_stuttgart.Voxie.Filter.DVC.StrideZ'].getValue(
        'x')

    fitWindowSize = op.Properties['de.uni_stuttgart.Voxie.Filter.DVC.CurveFitWindow'].getValue(
        'x')

    # raise exception if strideX size is negative
    if np.min((strideX, strideY, strideZ)) < 1:
        print("Stride Shape: ", (strideX, strideY, strideZ))
        raise ValueError(
            "DVC error: Stride size too small.")

    # raise exception if fitWindowSize size is negative or too big
    if fitWindowSize < 0 or fitWindowSize > np.min((m, n, p)):
        print("fitWindowSize: ", fitWindowSize)
        raise ValueError(
            "DVC error: fitWindowSize invalid.")
    startTime = time.time()
    # check if both inputs are available
    inputPath1 = op.Properties['de.uni_stuttgart.Voxie.Filter.DVC.V1'].getValue(
        'o')
    if inputPath1 == dbus.ObjectPath('/'):
        raise Exception('No input for volume 1 connected')
    inputPath2 = op.Properties['de.uni_stuttgart.Voxie.Filter.DVC.V2'].getValue(
        'o')
    if inputPath2 == dbus.ObjectPath('/'):
        raise Exception('No input for volume 2 connected')
    inputDataVoxel1 = op.GetInputData('de.uni_stuttgart.Voxie.Filter.DVC.V1').CastTo(
        'de.uni_stuttgart.Voxie.VolumeDataVoxel')
    inputDataVoxel2 = op.GetInputData('de.uni_stuttgart.Voxie.Filter.DVC.V2').CastTo(
        'de.uni_stuttgart.Voxie.VolumeDataVoxel')
    with inputDataVoxel1.GetBufferReadonly() as bufferV1, inputDataVoxel2.GetBufferReadonly() as bufferV2:
        v1 = bufferV1[:]
        v2 = bufferV2[:]
        print("data types", inputDataVoxel1.DataType)
        # define size of region of interest in voxel
        roiShape = (roix, roiy, roiz)
        windowShape = (m, n, p)
        strideShape = (strideX, strideY, strideZ)
        # print results as input data for DVC algorithm
        print(
            "DVC Algorithm input data:\n----\nReference volume (shape): ",
            v1.shape)
        # print("Modified volume (data): ", v1)
        print("Modified volume (shape): ", v2.shape)
        #  print("Modified volume (data): ", v2)
        print("Correlation window (shape): ", m, "x", n, "x", p)
        print("Region of Interest (shape): ", roiShape)
        spacing = list(np.array(inputDataVoxel1.GridSpacing))
        correlationResult = dvc.dvc(v1, v2, windowShape, roiShape, op, strideShape,
                                    fitWindowSize, mapDisplayement=False, stdvThreshold=stdvThreshold)
        if correlationResult is None:
            print("CorrelationResult: None!")
            op.Finish({})
            instance._context.client.destroy()
        else:
            origin = inputDataVoxel1.VolumeOrigin
            v1Size = inputDataVoxel1.ArrayShape
            datatype = inputDataVoxel1.DataType
            outputSize = correlationResult.shape[:-1]
            print("Correlation Result (shape): ", correlationResult.shape)
            spacing[0] = float(spacing[0] * v1Size[0] / outputSize[0])
            spacing[1] = float(spacing[1] * v1Size[1] / outputSize[1])
            spacing[2] = float(spacing[2] * v1Size[2] / outputSize[2])

            exx, eyy, ezz, exy, exz, eyz = dvc.strainTensor(correlationResult)

            # output volumes
            dataDX = instance.CreateVolumeDataVoxel(
                outputSize, datatype, origin, spacing)
            dataDY = instance.CreateVolumeDataVoxel(
                outputSize, datatype, origin, spacing)
            dataDZ = instance.CreateVolumeDataVoxel(
                outputSize, datatype, origin, spacing)
            dataCorrMax = instance.CreateVolumeDataVoxel(
                outputSize, datatype, origin, spacing)
            dataSTxx = instance.CreateVolumeDataVoxel(
                outputSize, datatype, origin, spacing)
            dataSTyy = instance.CreateVolumeDataVoxel(
                outputSize, datatype, origin, spacing)
            dataSTzz = instance.CreateVolumeDataVoxel(
                outputSize, datatype, origin, spacing)
            dataSTxy = instance.CreateVolumeDataVoxel(
                outputSize, datatype, origin, spacing)
            dataSTxz = instance.CreateVolumeDataVoxel(
                outputSize, datatype, origin, spacing)
            dataSTyz = instance.CreateVolumeDataVoxel(
                outputSize, datatype, origin, spacing)

            updateDX = dataDX.CreateUpdate()
            updateDY = dataDY.CreateUpdate()
            updateDZ = dataDZ.CreateUpdate()
            updateCorrMax = dataCorrMax.CreateUpdate()
            updateSTxx = dataSTxx.CreateUpdate()
            updateSTyy = dataSTyy.CreateUpdate()
            updateSTzz = dataSTzz.CreateUpdate()
            updateSTxy = dataSTxy.CreateUpdate()
            updateSTxz = dataSTxz.CreateUpdate()
            updateSTyz = dataSTyz.CreateUpdate()

            bufferDX = dataDX.GetBufferWritable(updateDX)
            bufferDY = dataDY.GetBufferWritable(updateDY)
            bufferDZ = dataDZ.GetBufferWritable(updateDZ)
            bufferCorrMax = dataCorrMax.GetBufferWritable(updateCorrMax)
            bufferSTxx = dataSTxx.GetBufferWritable(updateSTxx)
            bufferSTyy = dataSTyy.GetBufferWritable(updateSTyy)
            bufferSTzz = dataSTzz.GetBufferWritable(updateSTzz)
            bufferSTxy = dataSTxy.GetBufferWritable(updateSTxy)
            bufferSTxz = dataSTxz.GetBufferWritable(updateSTxz)
            bufferSTyz = dataSTyz.GetBufferWritable(updateSTyz)

            progressStep = 0.5 / outputSize[0]
            currentProgress = 0.5
            for p in range(0, outputSize[0]):
                for q in range(0, outputSize[1]):
                    for v in range(0, outputSize[2]):
                        bufferDX[p, q, v] = correlationResult[p, q, v, 0]
                        bufferDY[p, q, v] = correlationResult[p, q, v, 1]
                        bufferDZ[p, q, v] = correlationResult[p, q, v, 2]
                        bufferCorrMax[p, q, v] = correlationResult[p, q, v, 3]
                        bufferSTxx[p, q, v] = exx[p, q, v]
                        bufferSTyy[p, q, v] = eyy[p, q, v]
                        bufferSTzz[p, q, v] = ezz[p, q, v]
                        bufferSTxy[p, q, v] = exy[p, q, v]
                        bufferSTxz[p, q, v] = exz[p, q, v]
                        bufferSTyz[p, q, v] = eyz[p, q, v]
                currentProgress += progressStep
                op.SetProgress(currentProgress)
                op.ThrowIfCancelled()

            versionX = updateDX.Finish()
            versionY = updateDY.Finish()
            versionZ = updateDZ.Finish()
            versionCorrMax = updateCorrMax.Finish()
            versionSTxx = updateSTxx.Finish()
            versionSTyy = updateSTyy.Finish()
            versionSTzz = updateSTzz.Finish()
            versionSTxy = updateSTxy.Finish()
            versionSTxz = updateSTxz.Finish()
            versionSTyz = updateSTyz.Finish()

            result = {}

            result[outputPathDX] = {
                'Data': voxie.Variant('o', dataDX._objectPath),
                'DataVersion': voxie.Variant('o', versionX._objectPath),
            }
            result[outputPathDY] = {
                'Data': voxie.Variant('o', dataDY._objectPath),
                'DataVersion': voxie.Variant('o', versionY._objectPath),
            }
            result[outputPathDZ] = {
                'Data': voxie.Variant('o', dataDZ._objectPath),
                'DataVersion': voxie.Variant('o', versionZ._objectPath),
            }
            result[outputPathCorrMax] = {
                'Data': voxie.Variant('o', dataCorrMax._objectPath),
                'DataVersion': voxie.Variant('o', versionCorrMax._objectPath),
            }

            result[outputPathSTxx] = {
                'Data': voxie.Variant('o', dataSTxx._objectPath),
                'DataVersion': voxie.Variant('o', versionSTxx._objectPath),
            }
            result[outputPathSTyy] = {
                'Data': voxie.Variant('o', dataSTyy._objectPath),
                'DataVersion': voxie.Variant('o', versionSTyy._objectPath),
            }
            result[outputPathSTzz] = {
                'Data': voxie.Variant('o', dataSTzz._objectPath),
                'DataVersion': voxie.Variant('o', versionSTzz._objectPath),
            }

            result[outputPathSTxy] = {
                'Data': voxie.Variant('o', dataSTxy._objectPath),
                'DataVersion': voxie.Variant('o', versionSTxy._objectPath),
            }
            result[outputPathSTxz] = {
                'Data': voxie.Variant('o', dataSTxz._objectPath),
                'DataVersion': voxie.Variant('o', versionSTxz._objectPath),
            }
            result[outputPathSTyz] = {
                'Data': voxie.Variant('o', dataSTyz._objectPath),
                'DataVersion': voxie.Variant('o', versionSTyz._objectPath),
            }

            op.Finish(result)

            endTime = time.time()

            print(
                "The operation took: ",
                (endTime - startTime) / 60,
                " minuts")

            versionX._referenceCountingObject.destroy()
            versionY._referenceCountingObject.destroy()
            versionZ._referenceCountingObject.destroy()
            versionCorrMax._referenceCountingObject.destroy()
            versionSTxx._referenceCountingObject.destroy()
            versionSTyy._referenceCountingObject.destroy()
            versionSTzz._referenceCountingObject.destroy()
            versionSTxy._referenceCountingObject.destroy()
            versionSTxz._referenceCountingObject.destroy()
            versionSTyz._referenceCountingObject.destroy()

            # manual clean up as it is not possible to use "with blocks"
            #  because of the static nested block limit it python
            updateDX.__exit__(None, None, None)
            updateDY.__exit__(None, None, None)
            updateDZ.__exit__(None, None, None)
            updateCorrMax.__exit__(None, None, None)
            updateSTxx.__exit__(None, None, None)
            updateSTyy.__exit__(None, None, None)
            updateSTzz.__exit__(None, None, None)
            updateSTxy.__exit__(None, None, None)
            updateSTxz.__exit__(None, None, None)
            updateSTyz.__exit__(None, None, None)
            dataDX.__exit__(None, None, None)
            dataDY.__exit__(None, None, None)
            dataDZ.__exit__(None, None, None)
            dataCorrMax.__exit__(None, None, None)
            dataSTxx.__exit__(None, None, None)
            dataSTyy.__exit__(None, None, None)
            dataSTzz.__exit__(None, None, None)
            dataSTxy.__exit__(None, None, None)
            dataSTxz.__exit__(None, None, None)
            dataSTyz.__exit__(None, None, None)

            instance._context.client.destroy()
