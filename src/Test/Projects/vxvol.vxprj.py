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
out_dir = test_out_dir + '/vxvol'
try:
    os.mkdir(out_dir)
except FileExistsError:
    pass

# TODO: Verify whether all the StorageOrder operations are actually correct

'''
import numpy as np
open('testarray_24.dat', 'wb').write(np.arange(24, dtype=np.float16).tobytes())

import numpy as np, PIL.Image
d = np.arange(24, dtype=np.uint8).reshape((2, 3, 4), order='F')
for i in range(d.shape[2]):
    PIL.Image.fromarray(d[:, ::-1, i].T).save(f'images/testarray_24_{i+1:04}.tif')

rm -f images.*
zip -0 -r images.zip images
tar cf images.tar images
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


done = set()
def load_volume(filename):
    if filename in done:
        raise Exception(f'Duplicate filename: {filename}')
    done.add(filename)

    fn = f'{test_dir}/{filename}.vxvol.json'

    vol = voxie.cast(voxie.cast(instance.OpenFile(fn), 'de.uni_stuttgart.Voxie.DataNode').Data, 'de.uni_stuttgart.Voxie.VolumeDataVoxel')
    data = vol[:]
    expect_val(data.shape, vol.ArrayShape)
    yield vol, data

    volume = voxie.file.vxvol.read(fn)
    data = volume.load_to_array()
    vol = types.SimpleNamespace()
    vol.ArrayShape = tuple(volume.array_shape)
    vol.GridSpacing = tuple(volume.grid_spacing)
    vol.VolumeOrigin = tuple(volume.volume_origin)
    vol.VolumeSize = tuple([vol.ArrayShape[i] * vol.GridSpacing[i] for i in range(3)])
    yield vol, data

def load_volume_err(filename, pattern):
    fn = f'{test_dir}/{filename}.vxvol.json'

    try:
        vol = instance.OpenFile(fn)
        raise Exception(f'Loading {filename} should have failed')
    except Exception as e:
        if not re.fullmatch(pattern, str(e)):
            raise

    try:
        volume = voxie.file.vxvol.read(fn)
        data = volume.load_to_array()
        raise Exception(f'Loading {filename} should have failed')
    except Exception as e:
        msg = str(e)
        if not re.fullmatch(pattern, msg):
            print(f'Exception message {msg!r} does not match pattern {pattern!r}')
            raise

for vol, data in load_volume('vxvol_1'):
    expect(vol, 'ArrayShape', (2, 3, 4))
    expect(vol, 'GridSpacing', (1.5, 2.5, 3.5))
    expect(vol, 'VolumeOrigin', (-10.5, -20.5, -30.5))
    expect(vol, 'VolumeSize', (3.0, 7.5, 14.0))
    val_tr = [float(f) for f in data.transpose().flatten()]
    expect_val(val_tr, list(np.arange(24)))

load_volume_err('vxvol_2_fail', '''.*Unexpected member 'UnknownProperty' in JSON object "DataSource".*''')

for vol, data in load_volume('vxvol_3'):
    expect(vol, 'ArrayShape', (2, 3, 4))
    expect(vol, 'GridSpacing', (1.5, 2.5, 3.5))
    expect(vol, 'VolumeOrigin', (-10.5, -20.5, -30.5))
    expect(vol, 'VolumeSize', (3.0, 7.5, 14.0))
    val_tr = [float(f) for f in data.flatten()]
    expect_val(val_tr, list(np.arange(24)))

for vol, data in load_volume('vxvol_4'):
    expect(vol, 'ArrayShape', (2, 3, 4))
    expect(vol, 'GridSpacing', (1.5, 2.5, 3.5))
    expect(vol, 'VolumeOrigin', (-10.5, -20.5, -30.5))
    expect(vol, 'VolumeSize', (3.0, 7.5, 14.0))
    val_tr = [float(f) for f in data.transpose([2, 0, 1]).flatten()]
    expect_val(val_tr, list(np.arange(24)))

for vol, data in load_volume('vxvol_ia_1'):
    expect(vol, 'ArrayShape', (2, 3, 4))
    expect(vol, 'GridSpacing', (1.5, 2.5, 3.5))
    expect(vol, 'VolumeOrigin', (-10.5, -20.5, -30.5))
    expect(vol, 'VolumeSize', (3.0, 7.5, 14.0))
    val_tr = [float(f) for f in data.transpose().flatten()]
    expect_val(val_tr, list(np.arange(24)))

for vol, data in load_volume('vxvol_zip_1'):
    expect(vol, 'ArrayShape', (2, 3, 4))
    expect(vol, 'GridSpacing', (1.5, 2.5, 3.5))
    expect(vol, 'VolumeOrigin', (-10.5, -20.5, -30.5))
    expect(vol, 'VolumeSize', (3.0, 7.5, 14.0))
    val_tr = [float(f) for f in data.transpose().flatten()]
    expect_val(val_tr, list(np.arange(24)))

for vol, data in load_volume('vxvol_is_1'):
    expect(vol, 'ArrayShape', (2, 3, 4))
    expect(vol, 'GridSpacing', (1.5, 2.5, 3.5))
    expect(vol, 'VolumeOrigin', (-10.5, -20.5, -30.5))
    expect(vol, 'VolumeSize', (3.0, 7.5, 14.0))
    val_tr = [float(f) for f in data.transpose().flatten()]
    expect_val(val_tr, list(np.arange(24)))

for vol, data in load_volume('vxvol_ia_3'):
    expect(vol, 'ArrayShape', (4, 3, 2))
    expect(vol, 'GridSpacing', (1.5, 2.5, 3.5))
    expect(vol, 'VolumeOrigin', (-10.5, -20.5, -30.5))
    expect(vol, 'VolumeSize', (6.0, 7.5, 7.0))
    val_tr = [float(f) for f in data.flatten()]
    expect_val(val_tr, list(np.arange(24)))

for vol, data in load_volume('vxvol_zip_3'):
    expect(vol, 'ArrayShape', (4, 3, 2))
    expect(vol, 'GridSpacing', (1.5, 2.5, 3.5))
    expect(vol, 'VolumeOrigin', (-10.5, -20.5, -30.5))
    expect(vol, 'VolumeSize', (6.0, 7.5, 7.0))
    val_tr = [float(f) for f in data.flatten()]
    expect_val(val_tr, list(np.arange(24)))

for vol, data in load_volume('vxvol_is_3'):
    expect(vol, 'ArrayShape', (4, 3, 2))
    expect(vol, 'GridSpacing', (1.5, 2.5, 3.5))
    expect(vol, 'VolumeOrigin', (-10.5, -20.5, -30.5))
    expect(vol, 'VolumeSize', (6.0, 7.5, 7.0))
    val_tr = [float(f) for f in data.flatten()]
    expect_val(val_tr, list(np.arange(24)))

for vol, data in load_volume('vxvol_ia_4'):
    expect(vol, 'ArrayShape', (3, 2, 4))
    expect(vol, 'GridSpacing', (1.5, 2.5, 3.5))
    expect(vol, 'VolumeOrigin', (-10.5, -20.5, -30.5))
    expect(vol, 'VolumeSize', (4.5, 5.0, 14.0))
    val_tr = [float(f) for f in data.transpose([2, 0, 1]).flatten()]
    expect_val(val_tr, list(np.arange(24)))

for vol, data in load_volume('vxvol_zip_4'):
    expect(vol, 'ArrayShape', (3, 2, 4))
    expect(vol, 'GridSpacing', (1.5, 2.5, 3.5))
    expect(vol, 'VolumeOrigin', (-10.5, -20.5, -30.5))
    expect(vol, 'VolumeSize', (4.5, 5.0, 14.0))
    val_tr = [float(f) for f in data.transpose([2, 0, 1]).flatten()]
    expect_val(val_tr, list(np.arange(24)))

for vol, data in load_volume('vxvol_is_4'):
    expect(vol, 'ArrayShape', (3, 2, 4))
    expect(vol, 'GridSpacing', (1.5, 2.5, 3.5))
    expect(vol, 'VolumeOrigin', (-10.5, -20.5, -30.5))
    expect(vol, 'VolumeSize', (4.5, 5.0, 14.0))
    val_tr = [float(f) for f in data.transpose([2, 0, 1]).flatten()]
    expect_val(val_tr, list(np.arange(24)))

for vol, data in load_volume('vxvol_dense_5'):
    expect(vol, 'ArrayShape', (2, 3, 4))
    expect(vol, 'GridSpacing', (1.5, 2.5, 3.5))
    expect(vol, 'VolumeOrigin', (-10.5, -20.5, -30.5))
    expect(vol, 'VolumeSize', (3.0, 7.5, 14.0))
    val_tr = [float(f) for f in data[:, ::-1, ::-1].transpose([2, 0, 1]).transpose().flatten()]
    expect_val(val_tr, list(np.arange(24)))

for vol, data in load_volume('vxvol_ia_5'):
    expect(vol, 'ArrayShape', (3, 4, 2))
    expect(vol, 'GridSpacing', (1.5, 2.5, 3.5))
    expect(vol, 'VolumeOrigin', (-10.5, -20.5, -30.5))
    expect(vol, 'VolumeSize', (4.5, 10.0, 7.0))
    val_tr = [float(f) for f in data[:, ::-1, ::-1].transpose([2, 0, 1]).transpose().flatten()]
    expect_val(val_tr, list(np.arange(24)))

for vol, data in load_volume('vxvol_zip_5'):
    expect(vol, 'ArrayShape', (3, 4, 2))
    expect(vol, 'GridSpacing', (1.5, 2.5, 3.5))
    expect(vol, 'VolumeOrigin', (-10.5, -20.5, -30.5))
    expect(vol, 'VolumeSize', (4.5, 10.0, 7.0))
    val_tr = [float(f) for f in data[:, ::-1, ::-1].transpose([2, 0, 1]).transpose().flatten()]
    expect_val(val_tr, list(np.arange(24)))

for vol, data in load_volume('vxvol_is_5'):
    expect(vol, 'ArrayShape', (3, 4, 2))
    expect(vol, 'GridSpacing', (1.5, 2.5, 3.5))
    expect(vol, 'VolumeOrigin', (-10.5, -20.5, -30.5))
    expect(vol, 'VolumeSize', (4.5, 10.0, 7.0))
    val_tr = [float(f) for f in data[:, ::-1, ::-1].transpose([2, 0, 1]).transpose().flatten()]
    expect_val(val_tr, list(np.arange(24)))

for vol, data in load_volume('vxvol_dense_6'):
    expect(vol, 'ArrayShape', (2, 3, 4))
    expect(vol, 'GridSpacing', (1.5, 2.5, 3.5))
    expect(vol, 'VolumeOrigin', (-10.5, -20.5, -30.5))
    expect(vol, 'VolumeSize', (3.0, 7.5, 14.0))
    val_tr = [float(f) for f in data[:, ::-1, ::-1].transpose([2, 0, 1]).transpose().flatten()]
    expect_val(val_tr, list(np.arange(24)*1.5+2.5))

for vol, data in load_volume('vxvol_ia_6'):
    expect(vol, 'ArrayShape', (3, 4, 2))
    expect(vol, 'GridSpacing', (1.5, 2.5, 3.5))
    expect(vol, 'VolumeOrigin', (-10.5, -20.5, -30.5))
    expect(vol, 'VolumeSize', (4.5, 10.0, 7.0))
    val_tr = [float(f) for f in data[:, ::-1, ::-1].transpose([2, 0, 1]).transpose().flatten()]
    expect_val(val_tr, list(np.arange(24)*1.5+2.5))

for vol, data in load_volume('vxvol_zip_6'):
    expect(vol, 'ArrayShape', (3, 4, 2))
    expect(vol, 'GridSpacing', (1.5, 2.5, 3.5))
    expect(vol, 'VolumeOrigin', (-10.5, -20.5, -30.5))
    expect(vol, 'VolumeSize', (4.5, 10.0, 7.0))
    val_tr = [float(f) for f in data[:, ::-1, ::-1].transpose([2, 0, 1]).transpose().flatten()]
    expect_val(val_tr, list(np.arange(24)*1.5+2.5))

for vol, data in load_volume('vxvol_is_6'):
    expect(vol, 'ArrayShape', (3, 4, 2))
    expect(vol, 'GridSpacing', (1.5, 2.5, 3.5))
    expect(vol, 'VolumeOrigin', (-10.5, -20.5, -30.5))
    expect(vol, 'VolumeSize', (4.5, 10.0, 7.0))
    val_tr = [float(f) for f in data[:, ::-1, ::-1].transpose([2, 0, 1]).transpose().flatten()]
    expect_val(val_tr, list(np.arange(24)*1.5+2.5))


# Test writing

fn = f'{test_dir}/vxvol_1.vxvol.json'
fn_dat = f'{test_dir}/testarray_24.dat'
out_fn = out_dir + '/out.vxvol.json'
out_fn_dat = out_dir + '/out.vxvol.dat'
vol = voxie.cast(voxie.cast(instance.OpenFile(fn), 'de.uni_stuttgart.Voxie.DataNode').Data, 'de.uni_stuttgart.Voxie.VolumeDataVoxel')

exporter = instance.Components.GetComponent('de.uni_stuttgart.Voxie.ComponentType.Exporter', 'de.uni_stuttgart.Voxie.FileFormat.Volume.VxVol.Export').CastTo('de.uni_stuttgart.Voxie.Exporter')

with instance.CreateVolumeDataVoxel(vol.ArrayShape, vol.DataType, vol.VolumeOrigin, vol.GridSpacing) as new_vol:
    with new_vol.CreateUpdate() as update, new_vol.GetBufferWritable(update) as buffer:
        buffer[:] = vol[:]
        with update.Finish() as version:
            pass

    with exporter.StartExport(vol, out_fn) as result:
        result.Operation.WaitFor()

expect_files(fn_dat, out_fn_dat)

json_orig = json.load(open(fn, 'r'))
json_out = json.load(open(out_fn, 'r'))

json_orig['DataSource']['DataFilename'] = None
json_out['DataSource']['DataFilename'] = None

if json_orig != json_out:
    json.dump(json_orig, sys.stdout, indent=4, allow_nan=False, sort_keys=True, ensure_ascii=False)
    print(flush=True)
    json.dump(json_out, sys.stdout, indent=4, allow_nan=False, sort_keys=True, ensure_ascii=False)
    print(flush=True)
    raise Exception('JSON files differ')

# Try to load written volume
vol = voxie.cast(voxie.cast(instance.OpenFile(out_fn), 'de.uni_stuttgart.Voxie.DataNode').Data, 'de.uni_stuttgart.Voxie.VolumeDataVoxel')
expect(vol, 'ArrayShape', (2, 3, 4))
expect(vol, 'GridSpacing', (1.5, 2.5, 3.5))
expect(vol, 'VolumeOrigin', (-10.5, -20.5, -30.5))
expect(vol, 'VolumeSize', (3.0, 7.5, 14.0))
val_tr = [float(f) for f in vol[:].transpose().flatten()]
expect_val(val_tr, list(np.arange(24)))

# Test writing with voxie.file.vxvol

fn = f'{test_dir}/vxvol_1.vxvol.json'
fn_dat = f'{test_dir}/testarray_24.dat'
out_fn = out_dir + '/out_2.vxvol.json'
out_fn_dat = out_dir + '/out_2.vxvol.dat'
vol = voxie.cast(voxie.cast(instance.OpenFile(fn), 'de.uni_stuttgart.Voxie.DataNode').Data, 'de.uni_stuttgart.Voxie.VolumeDataVoxel')

voxie.file.vxvol.write(out_fn, vol[:], grid_spacing=vol.GridSpacing, volume_origin=vol.VolumeOrigin)

expect_files(fn_dat, out_fn_dat)

json_orig = json.load(open(fn, 'r'))
json_out = json.load(open(out_fn, 'r'))

json_orig['DataSource']['DataFilename'] = None
json_out['DataSource']['DataFilename'] = None

if json_orig != json_out:
    json.dump(json_orig, sys.stdout, indent=4, allow_nan=False, sort_keys=True, ensure_ascii=False)
    print(flush=True)
    json.dump(json_out, sys.stdout, indent=4, allow_nan=False, sort_keys=True, ensure_ascii=False)
    print(flush=True)
    raise Exception('JSON files differ')

# Try to load written volume
vol = voxie.cast(voxie.cast(instance.OpenFile(out_fn), 'de.uni_stuttgart.Voxie.DataNode').Data, 'de.uni_stuttgart.Voxie.VolumeDataVoxel')
expect(vol, 'ArrayShape', (2, 3, 4))
expect(vol, 'GridSpacing', (1.5, 2.5, 3.5))
expect(vol, 'VolumeOrigin', (-10.5, -20.5, -30.5))
expect(vol, 'VolumeSize', (3.0, 7.5, 14.0))
val_tr = [float(f) for f in vol[:].transpose().flatten()]
expect_val(val_tr, list(np.arange(24)))


instance._context.client.destroy()
