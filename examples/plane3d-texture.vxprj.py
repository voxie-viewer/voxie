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

# PYTHONPATH=pythonlib/ examples/plane3d-texture.py 

import voxie, dbus

instance = voxie.instanceFromArgs()


### de.uni_stuttgart.Voxie.ObjectKind.Property ###
# de.uni_stuttgart.Voxie.PlaneProperty #
o_PlaneProperty_1 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.PlaneProperty', {
  'de.uni_stuttgart.Voxie.PlaneProperty.Origin': voxie.Variant('(ddd)', (0.0, 0.0, 0.0)),
  'de.uni_stuttgart.Voxie.PlaneProperty.Orientation': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 0.0)),
})

### de.uni_stuttgart.Voxie.ObjectKind.Data ###
# de.uni_stuttgart.Voxie.VolumeObject #
o_VolumeObject_1 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.VolumeObject', {
})

### de.uni_stuttgart.Voxie.ObjectKind.Object3D ###
# de.uni_stuttgart.Voxie.Object3D.Plane #
o_Plane_1 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.Object3D.Plane', {
  'de.uni_stuttgart.Voxie.Object3D.Plane.ClippingDirection': voxie.Variant('s', 'de.uni_stuttgart.Voxie.Object3D.Plane.ClippingDirection.None'),
  'de.uni_stuttgart.Voxie.Object3D.Plane.Color': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 0.30000001192092896)),
  'de.uni_stuttgart.Voxie.Object3D.Plane.Plane': voxie.Variant('o', o_PlaneProperty_1),
  'de.uni_stuttgart.Voxie.Object3D.Plane.ShowVolumeSlice': voxie.Variant('b', True),
  'de.uni_stuttgart.Voxie.Object3D.Plane.SliceTextureResolution': voxie.Variant('s', 'de.uni_stuttgart.Voxie.Object3D.Plane.SliceTextureResolution.R512'),
  'de.uni_stuttgart.Voxie.Object3D.Plane.SliceValueColorMapping': voxie.Variant('a(d(dddd))', [(float('NaN'), (0.0, 0.0, 0.0, 0.0)), (0.0, (0.0, 0.0, 0.0, 1.0)), (1.0, (1.0, 1.0, 1.0, 1.0))]),
  'de.uni_stuttgart.Voxie.Object3D.Plane.SliceVolume': voxie.Variant('o', o_VolumeObject_1),
})

### de.uni_stuttgart.Voxie.ObjectKind.Filter ###
# de.uni_stuttgart.Voxie.Example.TheSphereGenerator #
o_TheSphereGenerator_1 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.Example.TheSphereGenerator', {
  'de.uni_stuttgart.Voxie.Example.TheSphereGenerator.Seed': voxie.Variant('x', 1337),
  'de.uni_stuttgart.Voxie.Example.TheSphereGenerator.Size': voxie.Variant('x', 129),
  'de.uni_stuttgart.Voxie.Output': voxie.Variant('o', o_VolumeObject_1),
})

### de.uni_stuttgart.Voxie.ObjectKind.Visualizer ###
# de.uni_stuttgart.Voxie.SliceVisualizer #
o_SliceVisualizer_1 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.SliceVisualizer', {
  'de.uni_stuttgart.Voxie.Input': voxie.Variant('ao', [o_PlaneProperty_1, o_VolumeObject_1]),
  'de.uni_stuttgart.Voxie.SliceVisualizer.GeometricPrimitive': voxie.Variant('o', None),
})
# de.uni_stuttgart.Voxie.Visualizer3D #
o_Visualizer3D_1 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.Visualizer3D', {
  'de.uni_stuttgart.Voxie.Visualizer3D.Camera.FieldOfView': voxie.Variant('d', 0.698),
  'de.uni_stuttgart.Voxie.Visualizer3D.Camera.LookAt': voxie.Variant('(ddd)', (0.0, 0.0, 0.0)),
  'de.uni_stuttgart.Voxie.Visualizer3D.Camera.Orientation': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 0.0)),
  'de.uni_stuttgart.Voxie.Visualizer3D.Camera.ViewSizeUnzoomed': voxie.Variant('d', 0.25),
  'de.uni_stuttgart.Voxie.Visualizer3D.Camera.ZoomLog': voxie.Variant('d', 0.0),
  'de.uni_stuttgart.Voxie.Visualizer3D.Objects': voxie.Variant('ao', [o_Plane_1]),
})

instance._context.client.destroy()
