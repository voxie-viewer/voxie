#!/usr/bin/python3

import voxie, dbus, os, sys

import numpy as np

import types
import re
import filecmp
import json

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

# TODO: Test StorageOrder
# TODO: Add tests which fail
# TODO: Test writing

# hdf5_1.hdf5 is vxvol_1.vxvol.json resaved as HDF5

'''
import shutil, h5py, numpy as np

# Same as uint8
shutil.copy('hdf5_1.hdf5', 'hdf5_2.hdf5')
with h5py.File('hdf5_2.hdf5', 'r+') as f:
    data = f['Volume']
    del f['Volume']
    f['Volume'] = np.uint8(data)

# With scaling factor
shutil.copy('hdf5_2.hdf5', 'hdf5_3.hdf5')
with h5py.File('hdf5_3.hdf5', 'r+') as f:
    f['VolumeScalingFactor'] = 0.8
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

    fn = f'{test_dir}/{filename}.hdf5'

    vol = voxie.cast(voxie.cast(instance.OpenFile(fn), 'de.uni_stuttgart.Voxie.DataNode').Data, 'de.uni_stuttgart.Voxie.VolumeDataVoxel')
    data = vol[:]
    expect_val(data.shape, vol.ArrayShape)
    yield vol, data


for vol, data in load_volume('hdf5_1'):
    expect(vol, 'ArrayShape', (2, 3, 4))
    expect(vol, 'GridSpacing', (1.5, 2.5, 3.5))
    expect(vol, 'VolumeOrigin', (-10.5, -20.5, -30.5))
    expect(vol, 'VolumeSize', (3.0, 7.5, 14.0))
    expect(vol, 'DataType', ('float', 16, sys.byteorder))
    val_tr = [float(f) for f in data.transpose().flatten()]
    expect_val(val_tr, list(np.arange(24)))


for vol, data in load_volume('hdf5_2'):
    expect(vol, 'ArrayShape', (2, 3, 4))
    expect(vol, 'GridSpacing', (1.5, 2.5, 3.5))
    expect(vol, 'VolumeOrigin', (-10.5, -20.5, -30.5))
    expect(vol, 'VolumeSize', (3.0, 7.5, 14.0))
    expect(vol, 'DataType', ('uint', 8, 'none'))
    val_tr = [float(f) for f in data.transpose().flatten()]
    expect_val(val_tr, list(np.arange(24)))


for vol, data in load_volume('hdf5_3'):
    expect(vol, 'ArrayShape', (2, 3, 4))
    expect(vol, 'GridSpacing', (1.5, 2.5, 3.5))
    expect(vol, 'VolumeOrigin', (-10.5, -20.5, -30.5))
    expect(vol, 'VolumeSize', (3.0, 7.5, 14.0))
    expect(vol, 'DataType', ('float', 32, sys.byteorder))
    val_tr = [float(f) for f in data.transpose().flatten()]
    expect_val(val_tr, list(np.arange(24, dtype=np.float32) * 0.8))


instance._context.client.destroy()
