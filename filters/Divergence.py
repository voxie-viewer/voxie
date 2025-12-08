#!/usr/bin/python3
import numpy as np
import voxie
import dbus

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()
if args.voxie_action != 'RunFilter':
    raise Exception('Invalid action: ' + repr(args.voxie_action))

with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationRunFilter']).ClaimOperationAndCatch() as op:
    inputData = op.GetInputData('de.uni_stuttgart.Voxie.Input').CastTo('de.uni_stuttgart.Voxie.VolumeSeriesData')
    outputPath = op.Properties['de.uni_stuttgart.Voxie.Output'].getValue('o')

    input_dims = inputData.Dimensions
    if len(input_dims) != 1:
        raise Exception('Input data is not one-dimensional')
    input_dim = input_dims[0]

    if input_dim.Property.Name != 'de.uni_stuttgart.Voxie.DataProperty.TensorFieldComponent':
        raise Exception('Input dimension is not de.uni_stuttgart.Voxie.DataProperty.TensorFieldComponent')
    if input_dim.Property.Type.Name != 'de.uni_stuttgart.Voxie.PropertyType.TensorComponent':
        raise Exception('Input dimension type is not de.uni_stuttgart.Voxie.PropertyType.TensorComponent')

    # TODO: Support different tensors?
    key_x = input_dim.LookupEntryByValue(voxie.Variant('(auau)', ([0], [])))
    if key_x == 2**64 - 1:
        raise Exception('Failed to look up key for X dimension')
    key_y = input_dim.LookupEntryByValue(voxie.Variant('(auau)', ([1], [])))
    if key_y == 2**64 - 1:
        raise Exception('Failed to look up key for Y dimension')
    key_z = input_dim.LookupEntryByValue(voxie.Variant('(auau)', ([2], [])))
    if key_z == 2**64 - 1:
        raise Exception('Failed to look up key for Z dimension')

    vol_x = inputData.LookupEntry([key_x]).CastTo('de.uni_stuttgart.Voxie.VolumeDataVoxel')
    vol_y = inputData.LookupEntry([key_y]).CastTo('de.uni_stuttgart.Voxie.VolumeDataVoxel').CastTo('de.uni_stuttgart.Voxie.VolumeDataVoxel')
    vol_z = inputData.LookupEntry([key_z]).CastTo('de.uni_stuttgart.Voxie.VolumeDataVoxel')

    volumeOrigin = inputData.VolumeOrigin
    volumeSize = inputData.VolumeSize
    array_shape = vol_x.ArrayShape
    grid_spacing = vol_x.GridSpacing
    if array_shape != vol_x.ArrayShape:
        raise Exception('array_shape != vol_x.ArrayShape')
    if array_shape != vol_y.ArrayShape:
        raise Exception('array_shape != vol_y.ArrayShape')
    if array_shape != vol_z.ArrayShape:
        raise Exception('array_shape != vol_z.ArrayShape')
    if grid_spacing != vol_x.GridSpacing:
        raise Exception('grid_spacing != vol_x.GridSpacing')
    if grid_spacing != vol_y.GridSpacing:
        raise Exception('grid_spacing != vol_y.GridSpacing')
    if grid_spacing != vol_z.GridSpacing:
        raise Exception('grid_spacing != vol_z.GridSpacing')

    with vol_x.GetBufferReadonly() as buf_x, vol_y.GetBufferReadonly() as buf_y, vol_z.GetBufferReadonly() as buf_z:
        # TODO: Data type?
        with instance.CreateVolumeDataVoxel(array_shape, ('float', 32, 'native'), volumeOrigin, grid_spacing) as outputData:
            with outputData.CreateUpdate() as update, outputData.GetBufferWritable(update) as outputBuffer:
                zeros = np.zeros_like(outputData, shape=array_shape[:2])
                for z in range(array_shape[2]):
                    op.ThrowIfCancelled()

                    # TODO: Clean up, merge partially with curl?

                    if z == 0:
                        x_m1 = zeros
                        y_m1 = zeros
                        z_m1 = zeros
                    else:
                        x_m1 = buf_x[:, :, z - 1]
                        y_m1 = buf_y[:, :, z - 1]
                        z_m1 = buf_z[:, :, z - 1]

                    if z == array_shape[2] - 1:
                        x_p1 = zeros
                        y_p1 = zeros
                        z_p1 = zeros
                    else:
                        x_p1 = buf_x[:, :, z + 1]
                        y_p1 = buf_y[:, :, z + 1]
                        z_p1 = buf_z[:, :, z + 1]

                    x0 = np.pad(buf_x[:, :, z], 1)
                    y0 = np.pad(buf_y[:, :, z], 1)
                    z0 = np.pad(buf_z[:, :, z], 1)

                    x_diff_x = x0[2:, 1:-1] - x0[:-2, 1:-1]
                    y_diff_y = y0[1:-1, 2:] - y0[1:-1, :-2]
                    z_diff_z = z_p1 - z_m1

                    outputBuffer[:, :, z] = x_diff_x / (2 * grid_spacing[0]) + y_diff_y / (2 * grid_spacing[1]) + z_diff_z / (2 * grid_spacing[2])

                    op.SetProgress((z + 1) / array_shape[2])
                with update.Finish() as version:
                    result = {}
                    result[outputPath] = {
                        'Data': voxie.Variant('o', outputData._objectPath),
                        'DataVersion': voxie.Variant('o', version._objectPath),
                    }
                    op.Finish(result)
