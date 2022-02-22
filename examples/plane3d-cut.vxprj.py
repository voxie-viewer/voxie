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
import voxie, dbus
instance = voxie.instanceFromArgs()


### de.uni_stuttgart.Voxie.ObjectKind.Property ###
# de.uni_stuttgart.Voxie.PlaneProperty #
o_PlaneProperty_1 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.PlaneProperty', {
  'de.uni_stuttgart.Voxie.PlaneProperty.Origin': voxie.Variant('(ddd)', (-3.725290298461914e-09, 0.00035387277603149414, -3.725290298461914e-09)),
  'de.uni_stuttgart.Voxie.PlaneProperty.Orientation': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 0.0)),
})

### de.uni_stuttgart.Voxie.ObjectKind.Data ###
# de.uni_stuttgart.Voxie.SurfaceObject #
#o_SurfaceObject_1 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.SurfaceObject', {})
o_SurfaceObject_1 = instance.OpenFile('/home/kiesssn/work/stuff/iso-rpi2-10.ply')
# de.uni_stuttgart.Voxie.VolumeObject #
#o_VolumeObject_1 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.VolumeObject', {})
o_VolumeObject_1 = instance.OpenFile('/home/kiesssn/files/stuff/rpi-2.hdf5')

### de.uni_stuttgart.Voxie.ObjectKind.Object3D ###
# de.uni_stuttgart.Voxie.Object3D.Plane #
o_Plane_1 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.Object3D.Plane', {
  'de.uni_stuttgart.Voxie.Object3D.Plane.ClippingDirection': voxie.Variant('s', 'de.uni_stuttgart.Voxie.Object3D.Plane.ClippingDirection.Positive'),
  'de.uni_stuttgart.Voxie.Object3D.Plane.Color': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 0.30000001192092896)),
  'de.uni_stuttgart.Voxie.Object3D.Plane.Plane': voxie.Variant('o', o_PlaneProperty_1),
  'de.uni_stuttgart.Voxie.Object3D.Plane.ShowVolumeSlice': voxie.Variant('b', False),
  'de.uni_stuttgart.Voxie.Object3D.Plane.SliceVolume': voxie.Variant('o', None),
})

### de.uni_stuttgart.Voxie.ObjectKind.Visualizer ###
# de.uni_stuttgart.Voxie.SliceVisualizer #
o_SliceVisualizer_1 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.SliceVisualizer', {
  'de.uni_stuttgart.Voxie.Input': voxie.Variant('ao', [o_VolumeObject_1, o_PlaneProperty_1]),
})
# de.uni_stuttgart.Voxie.Visualizer3D #
o_Visualizer3D_1 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.Visualizer3D', {
  'de.uni_stuttgart.Voxie.Input': voxie.Variant('ao', [o_SurfaceObject_1]),
  'de.uni_stuttgart.Voxie.Visualizer3D.Objects': voxie.Variant('ao', [o_Plane_1]),
})
