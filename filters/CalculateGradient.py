#!/usr/bin/python3
import numpy as np
import voxie
import dbus

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()
if args.voxie_action != 'RunFilter':
    raise Exception('Invalid action: ' + repr(args.voxie_action))

with context.makeObject(context.bus, context.busName, args.voxie_operation, [
    'de.uni_stuttgart.Voxie.ExternalOperationRunFilter'
]).ClaimOperationAndCatch() as op:
    # DOES NOTHING
    inputData = op.GetInputData('de.uni_stuttgart.Voxie.Input').CastTo(
        'de.uni_stuttgart.Voxie.VolumeDataVoxel')
    outputPath = op.Properties['de.uni_stuttgart.Voxie.Output'].getValue('o')
    dim_prop_data = {
        "Type": "de.uni_stuttgart.Voxie.PropertyType.TensorComponent",
        "DisplayName": "Tensor field component",
    }
    dim_prop = instance.CreateDataProperty('de.uni_stuttgart.Voxie.DataProperty.TensorFieldComponent', dim_prop_data)
    dim_type = dim_prop.Type
    dims = [
        instance.CreateSeriesDimension(dim_prop, voxie.Variant('a(auau)', [([0], []), ([1], []), ([2], [])])),
    ]
    volumeOrigin = inputData.VolumeOrigin
    volumeSize = inputData.VolumeSize
    with inputData.GetBufferReadonly() as inputArray:
        gx, gy, gz = np.gradient(inputArray, *inputData.GridSpacing)

        with instance.CreateVolumeDataVoxel(inputData.ArrayShape, ('float', 32, 'native'), volumeOrigin, inputData.GridSpacing) as data_x, instance.CreateVolumeDataVoxel(inputData.ArrayShape, ('float', 32, 'native'), volumeOrigin, inputData.GridSpacing) as data_y, instance.CreateVolumeDataVoxel(inputData.ArrayShape, ('float', 32, 'native'), volumeOrigin, inputData.GridSpacing) as data_z:
            with data_x.CreateUpdate() as update, data_x.GetBufferWritable(update) as outputBuffer_x:
                outputBuffer_x[:, :, :] = gx
            with data_y.CreateUpdate() as update, data_y.GetBufferWritable(update) as outputBuffer_y:
                outputBuffer_y[:, :, :] = gy
            with data_z.CreateUpdate() as update, data_z.GetBufferWritable(update) as outputBuffer_z:
                outputBuffer_z[:, :, :] = gz
            with instance.CreateVolumeSeriesData(dims, volumeOrigin, volumeSize) as resultData:
                with resultData.CreateUpdate() as update:
                    replace_mode = {'ReplaceMode': voxie.Variant('s', 'de.uni_stuttgart.Voxie.ReplaceMode.Insert'), }
                    resultData.SetEntry(update, [0], data_x, replace_mode)
                    resultData.SetEntry(update, [1], data_y, replace_mode)
                    resultData.SetEntry(update, [2], data_z, replace_mode)
                    version = update.Finish()
                result = {}
                result[outputPath] = {
                    'Data': voxie.Variant('o', resultData._objectPath),
                    'DataVersion': voxie.Variant('o', version._objectPath),
                }
                op.Finish(result)
