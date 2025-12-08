import sys
import json
import codecs
import os
import builtins
import contextlib

import numpy as np

import voxie.json_util
import voxie.data_types

import voxie.file.data_source


def throwOnUnexpectedMember(data, desc, expected):
    expected = set(expected)
    optional = set()
    if 'OptionalMembers' in data:
        optional = set(data['OptionalMembers'])
    for key in data:
        if key not in expected and key not in optional and key != 'OptionalMembers':
            raise Exception('Unexpected member {!r} in {}'.format(key, desc))


class Volume:
    pass


class OpenVolume:
    def __init__(self, *, volume):
        self.volume = volume


class OpenVolumeVoxel(OpenVolume):
    def __init__(self, *, volume, source, file=None):
        super().__init__(volume=volume)
        self.source = source
        self.file = file


class VolumeVoxel(Volume):
    def __init__(self, *, filename, data_type, array_shape, grid_spacing, volume_origin, data_source_type, data_source, data_properties=None, json_data=None):
        self.type = 'Voxel'

        self.filename = voxie.json_util.expect_string(filename)
        self.data_type = voxie.json_util.expect_array(list(data_type))
        self.array_shape = voxie.json_util.expect_int3(list(array_shape))
        self.grid_spacing = voxie.json_util.expect_float3(list(grid_spacing))
        self.volume_origin = voxie.json_util.expect_float3(list(volume_origin))
        self.data_source_type = voxie.json_util.expect_string(data_source_type)
        self.data_source = voxie.json_util.expect_object(data_source)

        self.data_properties = data_properties
        self.json_data = json_data

    @contextlib.contextmanager
    def open(self):
        with voxie.file.data_source.get(self.filename, self.data_source_type, self.data_source, array_shape=self.array_shape, data_type=self.data_type) as source:
            yield OpenVolumeVoxel(volume=self, source=source)

    @contextlib.contextmanager
    def open_write(self):
        if self.data_source_type != 'DenseBinaryFile':
            raise Exception(f'open_write() not supported for data source type {self.data_source_type!r}')

        throwOnUnexpectedMember(self.data_source, 'JSON object "DataSource"', ('DataFilename', 'Offset', 'StorageOrder', 'ValueOffset', 'ValueScalingFactor'))
        # TODO: Support some of these?
        if 'Offset' in self.data_source:
            raise Exception("Offset not supported in open_write()")
        if 'StorageOrder' in self.data_source:
            raise Exception("StorageOrder not supported in open_write()")
        if 'ValueOffset' in self.data_source:
            raise Exception("ValueOffset not supported in open_write()")
        if 'ValueScalingFactor' in self.data_source:
            raise Exception("ValueScalingFactor not supported in open_write()")

        rawDataFilename = os.path.join(os.path.dirname(self.filename), voxie.json_util.expect_string(self.data_source['DataFilename']))

        with open(rawDataFilename, 'wb') as file:
            yield OpenVolumeVoxel(volume=self, source=None, file=file)

    # TODO: Add tests for load_to_array()
    def load_to_array(self):
        with self.open() as open_file:
            result = np.zeros(self.array_shape, dtype=open_file.source.memDtype)

            # Apply storageOrder
            outData = open_file.source.with_storage_order_output(result)

            for z in range(open_file.source.diskSize[2]):
                outData[:, :, z] = open_file.source.read_image_disk_coord(z)

            return result

    def write_metadata(self, filename=None):
        if filename is None:
            filename = self.filename

        json_data = {
            'Type': 'de.uni_stuttgart.Voxie.FileFormat.Volume.VxVol',
            'VolumeType': self.type,
            'DataType': self.data_type,
            'ArrayShape': self.array_shape,
            'GridSpacing': self.grid_spacing,
            'VolumeOrigin': self.volume_origin,
            'DataSourceType': self.data_source_type,
            'DataSource': self.data_source,
        }
        if self.data_properties is not None:
            json_data['DataProperties'] = self.data_properties

        data = json.dumps(json_data, allow_nan=False, sort_keys=True, ensure_ascii=False, indent=2) + '\n'

        with builtins.open(filename, 'w', encoding='utf-8') as file:
            file.write(data)


class OpenVolumeBlockJpeg(OpenVolume):
    def __init__(self, *, volume, data_file, len_file):
        super().__init__(volume=volume)
        self.data_file = data_file
        self.len_file = len_file


class VolumeBlockJpeg(Volume):
    def __init__(self, *, array_shape, block_shape, grid_spacing, volume_origin, data_filename, len_filename, value_scaling_factor, value_offset, sample_precision, huffman_table_DC, huffman_table_AC, quantization_table, data_properties=None, json_data=None):
        self.type = 'BlockJpeg'

        self.array_shape = array_shape
        self.block_shape = block_shape
        self.grid_spacing = grid_spacing
        self.volume_origin = volume_origin
        self.data_filename = data_filename
        self.len_filename = len_filename
        self.value_scaling_factor = value_scaling_factor
        self.value_offset = value_offset
        self.sample_precision = sample_precision
        self.huffman_table_DC = huffman_table_DC
        self.huffman_table_AC = huffman_table_AC
        self.quantization_table = quantization_table

        self.data_properties = data_properties
        self.json_data = json_data

        self.block_count = [None, None, None]
        for i in range(3):
            self.block_count[i] = (self.array_shape[i] + self.block_shape[i] - 1) // self.block_shape[i]
        self.block_count = tuple(self.block_count)

        self.overall_blocks = self.block_count[0] * self.block_count[1] * self.block_count[2]

    @contextlib.contextmanager
    def open(self):
        with builtins.open(self.data_filename, 'rb') as data_file:
            with builtins.open(self.len_filename, 'rb') as len_file:
                yield OpenVolumeBlockJpeg(volume=self, data_file=data_file, len_file=len_file)


def read(filename):
    with builtins.open(filename, 'rb') as file:
        jsonData = json.load(codecs.getreader('utf-8')(file))

    if voxie.json_util.expect_string(jsonData['Type']) != 'de.uni_stuttgart.Voxie.FileFormat.Volume.VxVol':
        raise Exception('Expected type %s, got %s' % (
            repr('de.uni_stuttgart.Voxie.FileFormat.Volume.VxVol'), repr(jsonData['Type'])))

    if voxie.json_util.expect_string(jsonData['VolumeType']) == 'Voxel':
        throwOnUnexpectedMember(jsonData, 'JSON file', ('Type', 'VolumeType', 'DataType', 'ArrayShape', 'GridSpacing', 'VolumeOrigin', 'DataSourceType', 'DataSource', 'DataProperties'))

        dataType = (voxie.json_util.expect_string(jsonData['DataType'][0]), voxie.json_util.expect_int(jsonData['DataType'][1]), voxie.json_util.expect_string(jsonData['DataType'][2]))
        arrayShape = voxie.json_util.expect_int3(jsonData['ArrayShape'])
        gridSpacing = voxie.json_util.expect_float3(jsonData['GridSpacing'])
        volumeOrigin = voxie.json_util.expect_float3(jsonData['VolumeOrigin'])

        dataSourceType = jsonData['DataSourceType']

        return VolumeVoxel(filename=filename, data_type=dataType, array_shape=arrayShape, grid_spacing=gridSpacing, volume_origin=volumeOrigin, data_source_type=dataSourceType, data_source=jsonData['DataSource'], data_properties=jsonData.get('DataProperties'), json_data=jsonData)
    elif voxie.json_util.expect_string(jsonData['VolumeType']) == 'BlockJpeg':
        if voxie.json_util.expect_string(jsonData['BlockJpegType']) != '2D_XYSlices':
            raise Exception('Expected BlockJpeg type %s, got %s' % (
                repr(['2D_XYSlices']), repr(jsonData['BlockJpegType'])))

        if voxie.json_util.expect_string(jsonData['StorageType']) != 'VLQLengthsAndBitstream':
            raise Exception('Expected BlockJpeg storage type %s, got %s' % (
                repr(['VLQLengthsAndBitstream']), repr(jsonData['StorageType'])))

        throwOnUnexpectedMember(jsonData, 'JSON file', ('Type', 'VolumeType', 'ArrayShape', 'BlockShape', 'GridSpacing', 'VolumeOrigin', 'BlockJpegType', 'JpegInfo', 'StorageType', 'BitstreamFilename', 'VLQLengthFilename', 'DataProperties'))
        jpegInfo = voxie.json_util.expect_object(jsonData['JpegInfo'])
        throwOnUnexpectedMember(jpegInfo, 'JpegInfo', ('ValueOffset', 'ValueScalingFactor', 'SamplePrecision', 'HuffmanTableDC', 'HuffmanTableAC', 'QuantizationTable'))

        arrayShape = voxie.json_util.expect_int3(jsonData['ArrayShape'])
        blockShape = voxie.json_util.expect_int3(jsonData['BlockShape'])
        gridSpacing = voxie.json_util.expect_float3(jsonData['GridSpacing'])
        volumeOrigin = voxie.json_util.expect_float3(jsonData['VolumeOrigin'])

        dataFilename = voxie.json_util.expect_string(jsonData['BitstreamFilename'])
        lenFilename = voxie.json_util.expect_string(jsonData['VLQLengthFilename'])

        dataFilename = os.path.join(os.path.dirname(filename), dataFilename)
        lenFilename = os.path.join(os.path.dirname(filename), lenFilename)

        valueScalingFactor = voxie.json_util.expect_float(jpegInfo['ValueScalingFactor'])
        valueOffset = voxie.json_util.expect_float(jpegInfo['ValueOffset'])
        samplePrecision = voxie.json_util.expect_int(jpegInfo['SamplePrecision'])
        huffmanTableDC = voxie.json_util.expect_array(jpegInfo['HuffmanTableDC'])
        for entry in huffmanTableDC:
            voxie.json_util.expect_array(entry)
            for entry2 in entry:
                voxie.json_util.expect_int(entry2)
        huffmanTableAC = voxie.json_util.expect_array(jpegInfo['HuffmanTableAC'])
        for entry in huffmanTableAC:
            voxie.json_util.expect_array(entry)
            for entry2 in entry:
                voxie.json_util.expect_int(entry2)
        quantizationTable = voxie.json_util.expect_array(jpegInfo['QuantizationTable'])
        for entry in quantizationTable:
            voxie.json_util.expect_array(entry)
            for entry2 in entry:
                voxie.json_util.expect_int(entry2)

        return VolumeBlockJpeg(array_shape=arrayShape, block_shape=blockShape, grid_spacing=gridSpacing, volume_origin=volumeOrigin, data_filename=dataFilename, len_filename=lenFilename, value_scaling_factor=valueScalingFactor, value_offset=valueOffset, sample_precision=samplePrecision, huffman_table_DC=huffmanTableDC, huffman_table_AC=huffmanTableAC, quantization_table=quantizationTable, data_properties=jsonData.get('DataProperties'), json_data=jsonData)
    else:
        raise Exception('Expected volume type %s, got %s' % (repr(['Voxel', 'BlockJpeg']), repr(jsonData['VolumeType'])))


def write(filename, data, grid_spacing, volume_origin, data_properties=None):
    array_shape = data.shape

    dtype = data.dtype
    byteorder_code = dtype.byteorder
    if byteorder_code == '=':
        if sys.byteorder == 'little':
            byteorder_code = '<'
        elif sys.byteorder == 'big':
            byteorder_code = '>'
        else:
            raise Exception(f'Unknown sys.byteorder: {sys.byteorder}')
    byteorder = voxie.data_types.endianness_from_numpy[byteorder_code]
    data_type = (voxie.data_types.base_type_from_numpy[dtype.kind], dtype.itemsize * 8, byteorder)
    if (data_type[1] == 8) != (byteorder == 'none'):
        raise Exception("(data_type[1] == 8) != (byteorder == 'none')")

    baseFilename = os.path.basename(filename)
    if baseFilename.endswith('.json'):
        baseFilename = baseFilename[:-len('.json')]
    dataFilename = baseFilename + '.dat'
    data_source = {
        'DataFilename': dataFilename,
    }

    volume = VolumeVoxel(filename=filename, data_type=data_type, array_shape=array_shape, grid_spacing=grid_spacing, volume_origin=volume_origin, data_source_type='DenseBinaryFile', data_source=data_source, data_properties=data_properties)

    with volume.open_write() as open_file:
        # TODO: Move this code to OpenVolumeVoxel?
        for z in range(array_shape[2]):
            # TODO: Use a buffer instead? (See ExtFileVxVol.py)
            open_file.file.write(data[:, :, z].tobytes(order='F'))

    volume.write_metadata()
