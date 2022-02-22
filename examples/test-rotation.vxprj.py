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
  'de.uni_stuttgart.Voxie.PlaneProperty.Origin': voxie.Variant('(ddd)', (0.0, 0.0, 0.0)),
  'de.uni_stuttgart.Voxie.PlaneProperty.Orientation': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 0.0)),
})
o_PlaneProperty_2 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.PlaneProperty', {
  'de.uni_stuttgart.Voxie.PlaneProperty.Origin': voxie.Variant('(ddd)', (0.0, 0.0, -0.0)),
  'de.uni_stuttgart.Voxie.PlaneProperty.Orientation': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 0.0)),
})

### de.uni_stuttgart.Voxie.ObjectKind.Data ###
# de.uni_stuttgart.Voxie.VolumeObject #
o_VolumeObject_1 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.VolumeObject', {
})
o_VolumeObject_2 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.VolumeObject', {
})

### de.uni_stuttgart.Voxie.ObjectKind.Filter ###
# de.uni_stuttgart.Voxie.Example.TheSphereGenerator #
o_TheSphereGenerator_1 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.Example.TheSphereGenerator', {
  'de.uni_stuttgart.Voxie.Example.TheSphereGenerator.Seed': voxie.Variant('x', 1337),
  'de.uni_stuttgart.Voxie.Example.TheSphereGenerator.Size': voxie.Variant('x', 10),
  'de.uni_stuttgart.Voxie.Output': voxie.Variant('o', o_VolumeObject_1),
})
# de.uni_stuttgart.Voxie.Filter.Rotate #
o_Rotate_1 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.Filter.Rotate', {
  'de.uni_stuttgart.Voxie.Input': voxie.Variant('o', o_VolumeObject_1),
  'de.uni_stuttgart.Voxie.Output': voxie.Variant('o', o_VolumeObject_2),
  'de.uni_stuttgart.Voxie.Plane': voxie.Variant('o', o_PlaneProperty_1),
})

### de.uni_stuttgart.Voxie.ObjectKind.Visualizer ###
# de.uni_stuttgart.Voxie.SliceVisualizer #
o_SliceVisualizer_1 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.SliceVisualizer', {
  'de.uni_stuttgart.Voxie.View2D.CenterPoint': voxie.Variant('(dd)', (0.0, 0.0)),
  'de.uni_stuttgart.Voxie.SliceVisualizer.Filter2DConfiguration': voxie.Variant('s', '<?xml version="1.0"?>\n<!DOCTYPE filterchain>\n<filterchain2d version="1.0"/>\n'),
  'de.uni_stuttgart.Voxie.SliceVisualizer.GeometricPrimitive': voxie.Variant('o', None),
  'de.uni_stuttgart.Voxie.SliceVisualizer.GeometricPrimitiveColorBehindSlice': voxie.Variant('(dddd)', (0.0, 0.0, 0.0, 1.0)),
  'de.uni_stuttgart.Voxie.SliceVisualizer.GeometricPrimitiveColorInFrontOfSlice': voxie.Variant('(dddd)', (0.0, 0.0, 0.0, 1.0)),
  'de.uni_stuttgart.Voxie.SliceVisualizer.GeometricPrimitiveColorOnSlice': voxie.Variant('(dddd)', (0.0, 0.0, 0.0, 1.0)),
  'de.uni_stuttgart.Voxie.SliceVisualizer.GeometricPrimitiveVisibilityDistance': voxie.Variant('d', 0.0),
  'de.uni_stuttgart.Voxie.SliceVisualizer.GridShow': voxie.Variant('b', False),
  'de.uni_stuttgart.Voxie.SliceVisualizer.Plane': voxie.Variant('o', o_PlaneProperty_2),
  'de.uni_stuttgart.Voxie.SliceVisualizer.Plane.Orientation': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 0.0)),
  'de.uni_stuttgart.Voxie.SliceVisualizer.Plane.Origin': voxie.Variant('(ddd)', (0.0, 0.0, -0.0)),
  'de.uni_stuttgart.Voxie.SliceVisualizer.RulerShow': voxie.Variant('b', False),
  'de.uni_stuttgart.Voxie.SliceVisualizer.Surface': voxie.Variant('ao', []),
  'de.uni_stuttgart.Voxie.SliceVisualizer.ValueColorMapping': voxie.Variant('a(d(dddd))', [(float('NaN'), (0.0, 0.0, 0.0, 0.0)), (0.0, (0.0, 0.0, 0.0, 1.0)), (1.0, (1.0, 1.0, 1.0, 1.0))]),
  'de.uni_stuttgart.Voxie.View2D.VerticalSize': voxie.Variant('d', 0.01),
  'de.uni_stuttgart.Voxie.SliceVisualizer.Volume': voxie.Variant('o', o_VolumeObject_1),
  'de.uni_stuttgart.Voxie.SliceVisualizer.VolumeGridShow': voxie.Variant('b', False),
})
o_SliceVisualizer_2 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.SliceVisualizer', {
  'de.uni_stuttgart.Voxie.View2D.CenterPoint': voxie.Variant('(dd)', (0.0, 0.0)),
  'de.uni_stuttgart.Voxie.SliceVisualizer.Filter2DConfiguration': voxie.Variant('s', '<?xml version="1.0"?>\n<!DOCTYPE filterchain>\n<filterchain2d version="1.0"/>\n'),
  'de.uni_stuttgart.Voxie.SliceVisualizer.GeometricPrimitive': voxie.Variant('o', None),
  'de.uni_stuttgart.Voxie.SliceVisualizer.GeometricPrimitiveColorBehindSlice': voxie.Variant('(dddd)', (0.0, 0.0, 0.0, 1.0)),
  'de.uni_stuttgart.Voxie.SliceVisualizer.GeometricPrimitiveColorInFrontOfSlice': voxie.Variant('(dddd)', (0.0, 0.0, 0.0, 1.0)),
  'de.uni_stuttgart.Voxie.SliceVisualizer.GeometricPrimitiveColorOnSlice': voxie.Variant('(dddd)', (0.0, 0.0, 0.0, 1.0)),
  'de.uni_stuttgart.Voxie.SliceVisualizer.GeometricPrimitiveVisibilityDistance': voxie.Variant('d', 0.0),
  'de.uni_stuttgart.Voxie.SliceVisualizer.GridShow': voxie.Variant('b', False),
  'de.uni_stuttgart.Voxie.SliceVisualizer.Plane': voxie.Variant('o', o_PlaneProperty_2),
  'de.uni_stuttgart.Voxie.SliceVisualizer.Plane.Orientation': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 0.0)),
  'de.uni_stuttgart.Voxie.SliceVisualizer.Plane.Origin': voxie.Variant('(ddd)', (0.0, 0.0, -0.0)),
  'de.uni_stuttgart.Voxie.SliceVisualizer.RulerShow': voxie.Variant('b', False),
  'de.uni_stuttgart.Voxie.SliceVisualizer.Surface': voxie.Variant('ao', []),
  'de.uni_stuttgart.Voxie.SliceVisualizer.ValueColorMapping': voxie.Variant('a(d(dddd))', [(float('NaN'), (0.0, 0.0, 0.0, 0.0)), (0.0, (0.0, 0.0, 0.0, 1.0)), (1.0, (1.0, 1.0, 1.0, 1.0))]),
  'de.uni_stuttgart.Voxie.View2D.VerticalSize': voxie.Variant('d', 0.01),
  'de.uni_stuttgart.Voxie.SliceVisualizer.Volume': voxie.Variant('o', o_VolumeObject_2),
  'de.uni_stuttgart.Voxie.SliceVisualizer.VolumeGridShow': voxie.Variant('b', False),
})
