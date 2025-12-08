#!/usr/bin/python3

import voxie, dbus, os, sys

import numpy as np

import types
import re
import filecmp
import json

import voxie.file.vxvol

instance = voxie.instanceFromArgs()

test_dir = os.path.dirname(__file__)

test_out_dir = test_dir + '/../../../build/test'
try:
    os.mkdir(test_out_dir)
except FileExistsError:
    pass
out_dir = test_out_dir + '/jpeg_vxvol'
try:
    os.mkdir(out_dir)
except FileExistsError:
    pass

'''
import numpy as np
open('testarray_6000.dat', 'wb').write(np.arange(6000, dtype='<u2').tobytes())

# Compress using jpeg_compress_data.vxprj.py and store as jpeg_vol_1_8bit.vxvol.json
'''

def expect(dat, name, expected_value):
    actual_value = getattr(dat, name)
    if actual_value != expected_value:
        raise Exception(f'For value {name} expected {expected_value} got {actual_value}')
def expect_val(actual_value, expected_value):
    if actual_value != expected_value:
        raise Exception(f'Expected {expected_value} got {actual_value}')

def expect_files(file1, file2):
    if not filecmp.cmp(file1, file2):
        raise Exception(f'Files {file1} and {file2} differ')

# Read file

vol_voxel = voxie.cast(voxie.cast(instance.OpenFile(f'{test_dir}/jpeg_voxel_vol_1.vxvol.json'), 'de.uni_stuttgart.Voxie.DataNode').Data, 'de.uni_stuttgart.Voxie.VolumeDataVoxel')
vol_jpeg_node = voxie.cast(instance.OpenFile(f'{test_dir}/jpeg_vol_1_8bit.vxvol.json'), 'de.uni_stuttgart.Voxie.DataNode')
vol_jpeg = voxie.cast(vol_jpeg_node.Data, 'de.uni_stuttgart.Voxie.VolumeDataBlockJpeg')

expect(vol_voxel, 'ArrayShape', (10, 20, 30))
expect(vol_voxel, 'GridSpacing', (1.5, 2.5, 3.5))
expect(vol_voxel, 'VolumeOrigin', (-10.5, -20.5, -30.5))
expect(vol_voxel, 'VolumeSize', (15.0, 50.0, 105.0))
expect_val([float(f) for f in vol_voxel[:].transpose().flatten()], list(np.arange(6000)))

expect(vol_jpeg, 'ArrayShape', (10, 20, 30))
expect(vol_jpeg, 'GridSpacing', (1.5, 2.5, 3.5))
expect(vol_jpeg, 'VolumeOrigin', (-10.5, -20.5, -30.5))
expect(vol_jpeg, 'VolumeSize', (15.0, 50.0, 105.0))
expect(vol_jpeg, 'BlockShape', (16, 16, 16))
expect(vol_jpeg, 'SamplePrecision', 8)

vol_decom_node = voxie.cast(instance.CreateNodeChecked('de.uni_stuttgart.Voxie.Data.Volume', {}), 'de.uni_stuttgart.Voxie.DataNode')
decompress = instance.CreateNodeChecked('de.uni_stuttgart.Voxie.Filter.DecompressVolume', {
  'de.uni_stuttgart.Voxie.Input': voxie.Variant('o', vol_jpeg_node),
  'de.uni_stuttgart.Voxie.Output': voxie.Variant('o', vol_decom_node),
})
with decompress.CastTo('de.uni_stuttgart.Voxie.FilterNode').RunFilter() as result:
    result.Operation.WaitFor()

vol_decom = voxie.cast(vol_decom_node.Data, 'de.uni_stuttgart.Voxie.VolumeDataVoxel')

expect(vol_decom, 'ArrayShape', (10, 20, 30))
expect(vol_decom, 'GridSpacing', (1.5, 2.5, 3.5))
expect(vol_decom, 'VolumeOrigin', (-10.5, -20.5, -30.5))
expect(vol_decom, 'VolumeSize', (15.0, 50.0, 105.0))
# expect_val([float(f) for f in vol_decom[:].transpose().flatten()], list(np.arange(6000)))
difference = vol_voxel[:] - vol_decom[:]
min_diff = np.min(difference)
max_diff = np.max(difference)
if min_diff < -50 or max_diff > 50:
    raise Exception('min_diff < -50 or max_diff > 50')

# Write file

exporter = instance.Components.GetComponent('de.uni_stuttgart.Voxie.ComponentType.Exporter', 'de.uni_stuttgart.Voxie.FileFormat.Volume.VxVol.Export').CastTo('de.uni_stuttgart.Voxie.Exporter')

fn = f'{test_dir}/jpeg_vol_1_8bit.vxvol.json'
prefix = f'{test_dir}/jpeg_vol_1_8bit.vxvol.'
out_prefix = out_dir + '/out.vxvol.'
out_fn = out_dir + '/out.vxvol.json'

with exporter.StartExport(vol_jpeg, out_fn) as result:
    result.Operation.WaitFor()

expect_files(prefix + 'dat', out_prefix + 'dat')
expect_files(prefix + 'len', out_prefix + 'len')

json_orig = json.load(open(fn, 'r'))
json_out = json.load(open(out_fn, 'r'))

json_orig['BitstreamFilename'] = None
json_out['BitstreamFilename'] = None
json_orig['VLQLengthFilename'] = None
json_out['VLQLengthFilename'] = None

if json_orig != json_out:
    json.dump(json_orig, sys.stdout, indent=4, allow_nan=False, sort_keys=True, ensure_ascii=False)
    print(flush=True)
    json.dump(json_out, sys.stdout, indent=4, allow_nan=False, sort_keys=True, ensure_ascii=False)
    print(flush=True)
    raise Exception('JSON files differ')

# Try to load written volume
vol = voxie.cast(voxie.cast(instance.OpenFile(out_fn), 'de.uni_stuttgart.Voxie.DataNode').Data, 'de.uni_stuttgart.Voxie.VolumeDataBlockJpeg')
expect(vol, 'ArrayShape', (10, 20, 30))
expect(vol, 'GridSpacing', (1.5, 2.5, 3.5))
expect(vol, 'VolumeOrigin', (-10.5, -20.5, -30.5))
expect(vol, 'VolumeSize', (15.0, 50.0, 105.0))
# TODO: Verify whether data / tables etc. is the same?

# Test getting metainformation with voxie.file.vxvol
volume = voxie.file.vxvol.read(fn)
expect(volume, 'array_shape', list(vol_jpeg.ArrayShape))
expect(volume, 'grid_spacing', list(vol_jpeg.GridSpacing))
expect(volume, 'volume_origin', list(vol_jpeg.VolumeOrigin))
expect(volume, 'block_shape', list(vol_jpeg.BlockShape))
expect(volume, 'value_scaling_factor', vol_jpeg.ValueScalingFactor)
expect(volume, 'value_offset', vol_jpeg.ValueOffset)
expect(volume, 'sample_precision', vol_jpeg.SamplePrecision)
expect(volume, 'huffman_table_DC', vol_jpeg.HuffmanTableDC)
expect(volume, 'huffman_table_AC', vol_jpeg.HuffmanTableAC)
expect(volume, 'quantization_table', vol_jpeg.QuantizationTable)


instance._context.client.destroy()
