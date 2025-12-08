#!/usr/bin/python3
import numpy as np
import voxie
import dbus


# TODO: Support for multiple output dimensions
# TODO: Allow creating a Volume or a VolumeSeries as output, depending on input dimensions? Would require changes to Voxie.
class MultiDimensionalData:
    def __init__(self, data, relevant_dimensions_names):
        self.data = data
        self.relevant_dimensions_names = relevant_dimensions_names

        self.all_dimensions = self.data.Dimensions
        self.relevant_dimension_indices = [None for name in self.relevant_dimensions_names]
        self.other_dimension_indices = []
        for index in range(len(self.all_dimensions)):
            dim = self.all_dimensions[index]
            name = dim.Property.Name
            if name in self.relevant_dimensions_names:
                rpos = self.relevant_dimensions_names.index(name)
                if self.relevant_dimension_indices[rpos] is not None:
                    raise Exception(f'Duplicate dimension name {name!r}')
                self.relevant_dimension_indices[rpos] = index
            else:
                self.other_dimension_indices.append(index)
        for index, name in enumerate(self.relevant_dimensions_names):
            if self.relevant_dimension_indices[index] is None:
                raise Exception(f'Could not find dimension with name {name!r}')
        # print(self.relevant_dimension_indices, self.other_dimension_indices)

        self.relevant_dimensions = []
        for index in self.relevant_dimension_indices:
            self.relevant_dimensions.append(self.all_dimensions[index])
        self.other_dimensions = []
        for index in self.other_dimension_indices:
            self.other_dimensions.append(self.all_dimensions[index])

        all_other_entries = set()
        all_entries = self.data.ListKeys()
        for entry in all_entries:
            entry2 = []
            for index in self.other_dimension_indices:
                entry2.append(entry[index])
            all_other_entries.add(tuple(entry2))
        self.all_other_entries = sorted(all_other_entries)
        # print(all_entries, self.all_other_entries)

        self.output_dimensions = self.other_dimensions

        self.count = len(self.all_other_entries)

    def iterate_and_finish(self, op, indices_list, fun):
        for indices in indices_list:
            if len(indices) != len(self.relevant_dimensions):
                raise Exception("len(indices) != len(self.relevant_dimensions)")

        with instance.CreateVolumeSeriesData(self.output_dimensions, self.data.VolumeOrigin, self.data.VolumeSize) as output_data:
            with output_data.CreateUpdate() as output_update:

                idx = 0
                for entry in self.all_other_entries:
                    output_entry = entry

                    input_entries = []
                    input_data = []
                    for indices in indices_list:
                        input_entry = []
                        for index in range(len(self.all_dimensions)):
                            if index in self.other_dimension_indices:
                                pos = self.other_dimension_indices.index(index)
                                val = entry[pos]
                            else:
                                pos = self.relevant_dimension_indices.index(index)
                                val = indices[pos]
                            input_entry.append(val)
                        input_entries.append(input_entry)
                        input_data.append(inputData.LookupEntry(input_entry).CastTo('de.uni_stuttgart.Voxie.VolumeDataVoxel'))

                    with fun(idx, input_data) as vol_output_data:
                        options = {
                            'ReplaceMode': voxie.Variant('s', 'de.uni_stuttgart.Voxie.ReplaceMode.Insert'),
                        }
                        output_data.SetEntry(output_update, output_entry, vol_output_data, options)
                    idx += 1

                with output_update.Finish() as version:
                    result = {}
                    result[outputPath] = {
                        'Data': voxie.Variant('o', output_data._objectPath),
                        'DataVersion': voxie.Variant('o', version._objectPath),
                    }
                    op.Finish(result)


args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()
if args.voxie_action != 'RunFilter':
    raise Exception('Invalid action: ' + repr(args.voxie_action))

with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationRunFilter']).ClaimOperationAndCatch() as op:
    inputData = op.GetInputData('de.uni_stuttgart.Voxie.Input').CastTo('de.uni_stuttgart.Voxie.VolumeSeriesData')
    outputPath = op.Properties['de.uni_stuttgart.Voxie.Output'].getValue('o')

    md = MultiDimensionalData(inputData, ['de.uni_stuttgart.Voxie.DataProperty.TensorFieldComponent'])
    input_dim = md.relevant_dimensions[0]

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

    volumeOrigin = inputData.VolumeOrigin
    volumeSize = inputData.VolumeSize

    def fun(idx, vols):
        vol_x, vol_y, vol_z = vols
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
                    for z in range(array_shape[2]):
                        op.ThrowIfCancelled()

                        x0 = buf_x[:, :, z]
                        y0 = buf_y[:, :, z]
                        z0 = buf_z[:, :, z]

                        outputBuffer[:, :, z] = (x0 ** 2 + y0 ** 2 + z0 ** 2) ** 0.5

                        op.SetProgress(((z + 1) / array_shape[2] + idx) / md.count)
                    with update.Finish() as version:
                        pass
                    return outputData._clone()

    md.iterate_and_finish(op, [[key_x], [key_y], [key_z]], fun)
