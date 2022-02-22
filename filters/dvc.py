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

__author__ = "Pouya Shahidi,Alexander Zeising"

import time
import json
import datetime

from pathlib import Path

import numpy as np
import dbus

import voxie
import digitalvolumecorrelation.profiling as profiling
import digitalvolumecorrelation.dvc_impl as dvc
import digitalvolumecorrelation.config as dvc_config
from digitalvolumecorrelation.config import SubvoxelMode, CalculationMode

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()


if args.voxie_action != 'RunFilter':
    raise Exception('Invalid operation: ' + args.voxie_action)


LOGGING = True
OBJECT_DVC = 'de.uni_stuttgart.Voxie.Filter.DigitalVolumeCorrelation'
OUTPUT = OBJECT_DVC + '.Output'


def obj_to_enum(obj_path: str, enum_class) -> 'enum':
    return enum_class(obj_path.rsplit('.')[-1])


def write_log_of(config: dvc_config.Config):
    config_stripped = config._asdict()
    config_stripped.pop('reference_volume')
    config_stripped.pop('deformed_volume')
    config_stripped['subvoxel_mode'] = (
        config_stripped['subvoxel_mode'].name)
    config_stripped['calculation_mode'] = (
        config_stripped['calculation_mode'].name)

    out_data = dict(config=config_stripped,
                    seconds_elapsed=seconds_elapsed,
                    profiling=profiling.summarize())

    logfile_path = Path(__file__).parent / 'digitalvolumecorrelation/logs'
    logfile_path.mkdir(parents=True, exist_ok=True)

    logfile_name = datetime.datetime.now().isoformat().replace(':', '')
    logfile_name += '.dvclog.json'

    logfile_path = logfile_path / logfile_name
    json.dump(out_data, logfile_path.open('w'), indent=4)


with (context.makeObject(context.bus, context.busName, args.voxie_operation,
                         ['de.uni_stuttgart.Voxie.ExternalOperationRunFilter'])
      .ClaimOperationAndCatch()) as op:
    outputPathDX = op.Properties[OUTPUT + '.Displacement.X'].getValue('o')
    outputPathDY = op.Properties[OUTPUT + '.Displacement.Y'].getValue('o')
    outputPathDZ = op.Properties[OUTPUT + '.Displacement.Z'].getValue('o')
    outputPathCorrMax = op.Properties[OUTPUT + '.CorrelationMax'].getValue('o')

    strain_tensor_enabled = False

    if strain_tensor_enabled:
        outputPathSTxx = op.Properties[OUTPUT +
                                       '.StrainTensorXX'].getValue('o')
        outputPathSTyy = op.Properties[OUTPUT +
                                       '.StrainTensorYY'].getValue('o')
        outputPathSTzz = op.Properties[OUTPUT +
                                       '.StrainTensorZZ'].getValue('o')
        outputPathSTxy = op.Properties[OUTPUT +
                                       '.StrainTensorXY'].getValue('o')
        outputPathSTxz = op.Properties[OUTPUT +
                                       '.StrainTensorXZ'].getValue('o')
        outputPathSTyz = op.Properties[OUTPUT +
                                       '.StrainTensorYZ'].getValue('o')

    # define correlation window size
    m = op.Properties[OBJECT_DVC + '.Kernel.X'].getValue('x')
    n = op.Properties[OBJECT_DVC + '.Kernel.Y'].getValue('x')
    p = op.Properties[OBJECT_DVC + '.Kernel.Z'].getValue('x')

    std_threshold = op.Properties[OBJECT_DVC + '.StdvThreshold'].getValue('d')

    roix = op.Properties[OBJECT_DVC + '.Roi.X'].getValue('x')
    roiy = op.Properties[OBJECT_DVC + '.Roi.Y'].getValue('x')
    roiz = op.Properties[OBJECT_DVC + '.Roi.Z'].getValue('x')

    strideX = op.Properties[OBJECT_DVC + '.Stride.X'].getValue('x')
    strideY = op.Properties[OBJECT_DVC + '.Stride.Y'].getValue('x')
    strideZ = op.Properties[OBJECT_DVC + '.Stride.Z'].getValue('x')

    subvoxel_mode = op.Properties[OBJECT_DVC +
                                  '.SubvoxelAccuracyMode'].getValue('s')

    subvoxel_mode = obj_to_enum(subvoxel_mode, SubvoxelMode)

    calculation_mode = op.Properties[OBJECT_DVC +
                                     '.CalculationMode'].getValue('s')
    calculation_mode = obj_to_enum(calculation_mode, CalculationMode)

    workers_per_gpu = op.Properties[OBJECT_DVC +
                                    '.WorkersPerGPU'].getValue('x')

    interpolation_order = (op.Properties[OBJECT_DVC +
                                         '.OptimalFilter.InterpolationOrder']
                           .getValue('x'))
    print('OptimalFilter: Interpolation Order', interpolation_order)
    # raise exception if strideX size is negative
    if np.min((strideX, strideY, strideZ)) < 1:
        print("Stride Shape: ", (strideX, strideY, strideZ))
        raise ValueError(
            "DVC error: Stride size too small.")
    startTime = time.time()
    # check if both inputs are available
    inputPath1 = op.Properties[OBJECT_DVC + '.Reference'].getValue('o')
    if inputPath1 == dbus.ObjectPath('/'):
        raise Exception('No input connected to reference volume')
    inputPath2 = op.Properties[OBJECT_DVC + '.Deformed'].getValue('o')
    if inputPath2 == dbus.ObjectPath('/'):
        raise Exception('No input connected to deformed volume')
    inputDataVoxel1 = op.GetInputData(OBJECT_DVC + '.Reference').CastTo(
        'de.uni_stuttgart.Voxie.VolumeDataVoxel')
    inputDataVoxel2 = op.GetInputData(OBJECT_DVC + '.Deformed').CastTo(
        'de.uni_stuttgart.Voxie.VolumeDataVoxel')

    with inputDataVoxel1.GetBufferReadonly() as bufferV1, inputDataVoxel2.GetBufferReadonly() as bufferV2:
        reference_volume = bufferV1[:]
        deformed_volume = bufferV2[:]
        print("data types", inputDataVoxel1.DataType)
        # define size of region of interest in voxel
        window_shape = (m, n, p)
        roi_shape = (roix, roiy, roiz)
        stride_shape = (strideX, strideY, strideZ)
        # print results as input data for DVC algorithm
        print("DVC Algorithm input data:\n----\nReference volume (shape): ",
              reference_volume.shape)
        # print("Modified volume (data): ", v1)
        print("Deformed volume (shape): ", deformed_volume.shape)
        #  print("Modified volume (data): ", v2)
        print("Correlation window (shape): ", m, "x", n, "x", p)
        print("Region of Interest (shape): ", roi_shape)
        spacing = list(np.array(inputDataVoxel1.GridSpacing))

        # build config tuple
        config = dvc_config.Config(reference_volume=reference_volume,
                                   deformed_volume=deformed_volume,
                                   kernel_shape=window_shape,
                                   roi_shape=roi_shape,
                                   stride_shape=stride_shape,
                                   calculation_mode=calculation_mode,
                                   subvoxel_mode=subvoxel_mode,
                                   workers_per_gpu=workers_per_gpu,
                                   gpu_count=0,
                                   interpolation_order=interpolation_order,
                                   map_displacements=False,
                                   std_threshold=std_threshold)

        # initialize tuple globally for other systems and receive updated copy
        config = dvc_config.init(config)

        print('SubvoxelAccuracyMode:', config.subvoxel_mode, flush=True)
        print('CalculationMode:', config.calculation_mode, flush=True)
        print('WorkersPerGPU', config.workers_per_gpu, flush=True)

        correlationResult = dvc.dvc(op, config)

        origin = inputDataVoxel1.VolumeOrigin
        v1_size = inputDataVoxel1.ArrayShape
        datatype = inputDataVoxel1.DataType
        outputSize = correlationResult.shape[:-1]
        print("Correlation Result (shape): ", correlationResult.shape)

        spacing[0] = float(spacing[0] * v1_size[0] / outputSize[0])
        spacing[1] = float(spacing[1] * v1_size[1] / outputSize[1])
        spacing[2] = float(spacing[2] * v1_size[2] / outputSize[2])

        if strain_tensor_enabled:
            exx, eyy, ezz, exy, exz, eyz = dvc.strain_tensor(
                correlationResult)

        # output volumes
        dataDX = instance.CreateVolumeDataVoxel(
            outputSize, datatype, origin, spacing)
        dataDY = instance.CreateVolumeDataVoxel(
            outputSize, datatype, origin, spacing)
        dataDZ = instance.CreateVolumeDataVoxel(
            outputSize, datatype, origin, spacing)
        dataCorrMax = instance.CreateVolumeDataVoxel(
            outputSize, datatype, origin, spacing)
        if strain_tensor_enabled:
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

        if strain_tensor_enabled:
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

        if strain_tensor_enabled:
            bufferSTxx = dataSTxx.GetBufferWritable(updateSTxx)
            bufferSTyy = dataSTyy.GetBufferWritable(updateSTyy)
            bufferSTzz = dataSTzz.GetBufferWritable(updateSTzz)
            bufferSTxy = dataSTxy.GetBufferWritable(updateSTxy)
            bufferSTxz = dataSTxz.GetBufferWritable(updateSTxz)
            bufferSTyz = dataSTyz.GetBufferWritable(updateSTyz)

        bufferDX[:] = correlationResult[..., 0]
        bufferDY[:] = correlationResult[..., 1]
        bufferDZ[:] = correlationResult[..., 2]

        bufferCorrMax[:] = correlationResult[..., 3]

        if strain_tensor_enabled:
            bufferSTxx[:] = exx[:]
            bufferSTyy[:] = eyy[:]
            bufferSTzz[:] = ezz[:]
            bufferSTxy[:] = exy[:]
            bufferSTxz[:] = exz[:]
            bufferSTyz[:] = eyz[:]

        versionX = updateDX.Finish()
        versionY = updateDY.Finish()
        versionZ = updateDZ.Finish()

        versionCorrMax = updateCorrMax.Finish()

        if strain_tensor_enabled:
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

        if strain_tensor_enabled:
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
        seconds_elapsed = (endTime - startTime)
        minutes_elapsed = seconds_elapsed / 60

        print(f'The operation took: {minutes_elapsed} minutes')

        profiling.pprint_summary()

        if LOGGING:
            write_log_of(config)

        versionX._referenceCountingObject.destroy()
        versionY._referenceCountingObject.destroy()
        versionZ._referenceCountingObject.destroy()
        versionCorrMax._referenceCountingObject.destroy()

        if strain_tensor_enabled:
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

        if strain_tensor_enabled:
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

        if strain_tensor_enabled:
            dataSTxx.__exit__(None, None, None)
            dataSTyy.__exit__(None, None, None)
            dataSTzz.__exit__(None, None, None)
            dataSTxy.__exit__(None, None, None)
            dataSTxz.__exit__(None, None, None)
            dataSTyz.__exit__(None, None, None)

        instance._context.client.destroy()
