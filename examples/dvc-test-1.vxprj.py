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
# Stored using voxie version 'Version 0.2 rev stupro-final-version-3963-g3c1302a8 built with QT 5.11.3'
import voxie, dbus
instance = voxie.instanceFromArgs()


### de.uni_stuttgart.Voxie.ObjectKind.Property ###
# de.uni_stuttgart.Voxie.Property.Plane #
o_Plane_1 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.Property.Plane', {
  'de.uni_stuttgart.Voxie.Property.Plane.Orientation': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 0.0)),
  'de.uni_stuttgart.Voxie.Property.Plane.Origin': voxie.Variant('(ddd)', (0.0, 0.0, -0.0002000031527131796)),
})
o_Plane_1.ManualDisplayName = (False, '')
o_Plane_1.GraphPosition = (-240.0, 0.0)

### de.uni_stuttgart.Voxie.ObjectKind.Data ###
# de.uni_stuttgart.Voxie.Data.Volume #
o_Volume_1 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.Data.Volume', {
  'de.uni_stuttgart.Voxie.MovableDataObject.Rotation': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 0.0)),
  'de.uni_stuttgart.Voxie.MovableDataObject.Translation': voxie.Variant('(ddd)', (0.0, 0.0, 0.0)),
})
o_Volume_1.ManualDisplayName = (False, '')
o_Volume_1.GraphPosition = (-320.0, 80.0)
o_Volume_2 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.Data.Volume', {
  'de.uni_stuttgart.Voxie.MovableDataObject.Rotation': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 0.0)),
  'de.uni_stuttgart.Voxie.MovableDataObject.Translation': voxie.Variant('(ddd)', (0.0, 0.0, 0.0)),
})
o_Volume_2.ManualDisplayName = (False, '')
o_Volume_2.GraphPosition = (160.0, 80.0)
o_Volume_3 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.Data.Volume', {
  'de.uni_stuttgart.Voxie.MovableDataObject.Rotation': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 0.0)),
  'de.uni_stuttgart.Voxie.MovableDataObject.Translation': voxie.Variant('(ddd)', (0.0, 0.0, 0.0)),
})
o_Volume_3.ManualDisplayName = (False, '')
o_Volume_3.GraphPosition = (0.0, 80.0)
o_Volume_4 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.Data.Volume', {
  'de.uni_stuttgart.Voxie.MovableDataObject.Rotation': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 0.0)),
  'de.uni_stuttgart.Voxie.MovableDataObject.Translation': voxie.Variant('(ddd)', (0.0, 0.0, 0.0)),
})
o_Volume_4.ManualDisplayName = (False, '')
o_Volume_4.GraphPosition = (-160.0, 80.0)
o_Volume_5 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.Data.Volume', {
  'de.uni_stuttgart.Voxie.MovableDataObject.Rotation': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 0.0)),
  'de.uni_stuttgart.Voxie.MovableDataObject.Translation': voxie.Variant('(ddd)', (0.0, 0.0, 0.0)),
})
o_Volume_5.ManualDisplayName = (False, '')
o_Volume_5.GraphPosition = (-80.0, 240.0)
o_Volume_6 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.Data.Volume', {
  'de.uni_stuttgart.Voxie.MovableDataObject.Rotation': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 0.0)),
  'de.uni_stuttgart.Voxie.MovableDataObject.Translation': voxie.Variant('(ddd)', (0.0, 0.0, 0.0)),
})
o_Volume_6.ManualDisplayName = (False, '')
o_Volume_6.GraphPosition = (-160.0, 400.0)
o_Volume_7 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.Data.Volume', {
  'de.uni_stuttgart.Voxie.MovableDataObject.Rotation': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 0.0)),
  'de.uni_stuttgart.Voxie.MovableDataObject.Translation': voxie.Variant('(ddd)', (0.0, 0.0, 0.0)),
})
o_Volume_7.ManualDisplayName = (False, '')
o_Volume_7.GraphPosition = (-320.0, 400.0)
o_Volume_8 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.Data.Volume', {
  'de.uni_stuttgart.Voxie.MovableDataObject.Rotation': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 0.0)),
  'de.uni_stuttgart.Voxie.MovableDataObject.Translation': voxie.Variant('(ddd)', (0.0, 0.0, 0.0)),
})
o_Volume_8.ManualDisplayName = (False, '')
o_Volume_8.GraphPosition = (0.0, 400.0)
o_Volume_9 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.Data.Volume', {
  'de.uni_stuttgart.Voxie.MovableDataObject.Rotation': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 0.0)),
  'de.uni_stuttgart.Voxie.MovableDataObject.Translation': voxie.Variant('(ddd)', (0.0, 0.0, 0.0)),
})
o_Volume_9.ManualDisplayName = (False, '')
o_Volume_9.GraphPosition = (160.0, 400.0)

### de.uni_stuttgart.Voxie.ObjectKind.Filter ###
# de.uni_stuttgart.Voxie.Example.Filter.CreateExampleShiftVolumes #
o_CreateExampleShiftVolumes_1 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.Example.Filter.CreateExampleShiftVolumes', {
  'de.uni_stuttgart.Voxie.Example.Filter.CreateExampleShiftVolumes.OutputX': voxie.Variant('o', o_Volume_2),
  'de.uni_stuttgart.Voxie.Example.Filter.CreateExampleShiftVolumes.OutputY': voxie.Variant('o', o_Volume_3),
  'de.uni_stuttgart.Voxie.Example.Filter.CreateExampleShiftVolumes.OutputZ': voxie.Variant('o', o_Volume_4),
})
o_CreateExampleShiftVolumes_1.ManualDisplayName = (False, '')
o_CreateExampleShiftVolumes_1.GraphPosition = (80.0, 0.0)
# de.uni_stuttgart.Voxie.Example.Filter.TheSphereGenerator #
o_TheSphereGenerator_1 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.Example.Filter.TheSphereGenerator', {
  'de.uni_stuttgart.Voxie.Example.Filter.TheSphereGenerator.Seed': voxie.Variant('x', 1337),
  'de.uni_stuttgart.Voxie.Example.Filter.TheSphereGenerator.Size': voxie.Variant('x', 129),
  'de.uni_stuttgart.Voxie.Output': voxie.Variant('o', o_Volume_1),
})
o_TheSphereGenerator_1.ManualDisplayName = (False, '')
o_TheSphereGenerator_1.GraphPosition = (-80.0, 0.0)
# de.uni_stuttgart.Voxie.Filter.DigitalVolumeCorrelation #
o_DigitalVolumeCorrelation_1 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.Filter.DigitalVolumeCorrelation', {
  'de.uni_stuttgart.Voxie.Filter.DigitalVolumeCorrelation.CX': voxie.Variant('x', 16),
  'de.uni_stuttgart.Voxie.Filter.DigitalVolumeCorrelation.CY': voxie.Variant('x', 16),
  'de.uni_stuttgart.Voxie.Filter.DigitalVolumeCorrelation.CZ': voxie.Variant('x', 16),
  'de.uni_stuttgart.Voxie.Filter.DigitalVolumeCorrelation.CurveFitWindow': voxie.Variant('x', 0),
  'de.uni_stuttgart.Voxie.Filter.DigitalVolumeCorrelation.Output.CorrelationMax': voxie.Variant('o', o_Volume_6),
  'de.uni_stuttgart.Voxie.Filter.DigitalVolumeCorrelation.Output.DisplacementX': voxie.Variant('o', o_Volume_7),
  'de.uni_stuttgart.Voxie.Filter.DigitalVolumeCorrelation.Output.DisplacementY': voxie.Variant('o', o_Volume_8),
  'de.uni_stuttgart.Voxie.Filter.DigitalVolumeCorrelation.Output.DisplacementZ': voxie.Variant('o', o_Volume_9),
  'de.uni_stuttgart.Voxie.Filter.DigitalVolumeCorrelation.RoiX': voxie.Variant('x', 31),
  'de.uni_stuttgart.Voxie.Filter.DigitalVolumeCorrelation.RoiY': voxie.Variant('x', 31),
  'de.uni_stuttgart.Voxie.Filter.DigitalVolumeCorrelation.RoiZ': voxie.Variant('x', 31),
  'de.uni_stuttgart.Voxie.Filter.DigitalVolumeCorrelation.StdvThreshold': voxie.Variant('d', 0.0),
  'de.uni_stuttgart.Voxie.Filter.DigitalVolumeCorrelation.StrideX': voxie.Variant('x', 8),
  'de.uni_stuttgart.Voxie.Filter.DigitalVolumeCorrelation.StrideY': voxie.Variant('x', 8),
  'de.uni_stuttgart.Voxie.Filter.DigitalVolumeCorrelation.StrideZ': voxie.Variant('x', 8),
  'de.uni_stuttgart.Voxie.Filter.DigitalVolumeCorrelation.SubvoxelAccuracyMode': voxie.Variant('s', 'de.uni_stuttgart.Voxie.Filter.DigitalVolumeCorrelation.SubvoxelAccuracyMode.OptimalFilter'),
  'de.uni_stuttgart.Voxie.Filter.DigitalVolumeCorrelation.V1': voxie.Variant('o', o_Volume_1),
  'de.uni_stuttgart.Voxie.Filter.DigitalVolumeCorrelation.V2': voxie.Variant('o', o_Volume_5),
})
o_DigitalVolumeCorrelation_1.ManualDisplayName = (False, '')
o_DigitalVolumeCorrelation_1.GraphPosition = (0.0, 320.0)
# de.uni_stuttgart.Voxie.Filter.ShiftVolume #
o_ShiftVolume_1 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.Filter.ShiftVolume', {
  'de.uni_stuttgart.Voxie.Filter.ShiftVolume.ShiftAmount.X': voxie.Variant('o', o_Volume_2),
  'de.uni_stuttgart.Voxie.Filter.ShiftVolume.ShiftAmount.Y': voxie.Variant('o', o_Volume_3),
  'de.uni_stuttgart.Voxie.Filter.ShiftVolume.ShiftAmount.Z': voxie.Variant('o', o_Volume_4),
  'de.uni_stuttgart.Voxie.Input': voxie.Variant('o', o_Volume_1),
  'de.uni_stuttgart.Voxie.Output': voxie.Variant('o', o_Volume_5),
})
o_ShiftVolume_1.ManualDisplayName = (False, '')
o_ShiftVolume_1.GraphPosition = (-80.0, 160.0)

### de.uni_stuttgart.Voxie.ObjectKind.Visualizer ###
# de.uni_stuttgart.Voxie.Visualizer.Slice #
o_Slice_1 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.Visualizer.Slice', {
  'de.uni_stuttgart.Voxie.View2D.CenterPoint': voxie.Variant('(dd)', (0.0, 0.0)),
  'de.uni_stuttgart.Voxie.View2D.VerticalSize': voxie.Variant('d', 0.142),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Filter2DConfiguration': voxie.Variant('s', '<?xml version="1.0"?>\n<!DOCTYPE filterchain>\n<filterchain2d version="1.0"/>\n'),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitive': voxie.Variant('o', None),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitiveColorBehindSlice': voxie.Variant('(dddd)', (0.0, 0.0, 1.0, 1.0)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitiveColorInFrontOfSlice': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 1.0)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitiveColorOnSlice': voxie.Variant('(dddd)', (0.0, 1.0, 0.0, 1.0)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitiveVisibilityDistance': voxie.Variant('d', 0.0),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GridColor': voxie.Variant('(dddd)', (1.0, 1.0, 0.0, 0.5)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GridShow': voxie.Variant('b', False),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GridSpacing': voxie.Variant('d', 0.001),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GridSpacingAutomatic': voxie.Variant('b', True),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.InfoTable': voxie.Variant('o', None),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Interpolation': voxie.Variant('s', 'de.uni_stuttgart.Voxie.Interpolation.Linear'),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Plane': voxie.Variant('o', o_Plane_1),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Plane.Orientation': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 0.0)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Plane.Origin': voxie.Variant('(ddd)', (0.0, 0.0, -0.0002000031527131796)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.RulerColor': voxie.Variant('(dddd)', (1.0, 1.0, 0.0, 0.5)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.RulerShow': voxie.Variant('b', False),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.RulerSpacing': voxie.Variant('d', 0.001),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.RulerSpacingAutomatic': voxie.Variant('b', True),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Show2DFilterMask': voxie.Variant('x', 0),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.ShowSliceCenter': voxie.Variant('b', True),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Surface': voxie.Variant('ao', []),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.ValueColorMapping': voxie.Variant('a(d(dddd)i)', [(0.0, (0.0, 0.0, 0.0, 1.0), 0), (1.0, (1.0, 1.0, 1.0, 1.0), 0)]),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Volume': voxie.Variant('o', o_Volume_1),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.VolumeGridColor': voxie.Variant('(dddd)', (1.0, 1.0, 0.0, 0.5)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.VolumeGridShow': voxie.Variant('b', False),
})
o_Slice_1.ManualDisplayName = (False, '')
o_Slice_1.GraphPosition = (-240.0, 160.0)
o_Slice_2 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.Visualizer.Slice', {
  'de.uni_stuttgart.Voxie.View2D.CenterPoint': voxie.Variant('(dd)', (0.0, 0.0)),
  'de.uni_stuttgart.Voxie.View2D.VerticalSize': voxie.Variant('d', 0.142),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Filter2DConfiguration': voxie.Variant('s', '<?xml version="1.0"?>\n<!DOCTYPE filterchain>\n<filterchain2d version="1.0"/>\n'),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitive': voxie.Variant('o', None),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitiveColorBehindSlice': voxie.Variant('(dddd)', (0.0, 0.0, 1.0, 1.0)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitiveColorInFrontOfSlice': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 1.0)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitiveColorOnSlice': voxie.Variant('(dddd)', (0.0, 1.0, 0.0, 1.0)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitiveVisibilityDistance': voxie.Variant('d', 0.0),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GridColor': voxie.Variant('(dddd)', (1.0, 1.0, 0.0, 0.5)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GridShow': voxie.Variant('b', False),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GridSpacing': voxie.Variant('d', 0.001),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GridSpacingAutomatic': voxie.Variant('b', True),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.InfoTable': voxie.Variant('o', None),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Interpolation': voxie.Variant('s', 'de.uni_stuttgart.Voxie.Interpolation.Linear'),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Plane': voxie.Variant('o', o_Plane_1),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Plane.Orientation': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 0.0)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Plane.Origin': voxie.Variant('(ddd)', (0.0, 0.0, -0.0002000031527131796)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.RulerColor': voxie.Variant('(dddd)', (1.0, 1.0, 0.0, 0.5)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.RulerShow': voxie.Variant('b', False),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.RulerSpacing': voxie.Variant('d', 0.001),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.RulerSpacingAutomatic': voxie.Variant('b', True),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Show2DFilterMask': voxie.Variant('x', 0),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.ShowSliceCenter': voxie.Variant('b', True),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Surface': voxie.Variant('ao', []),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.ValueColorMapping': voxie.Variant('a(d(dddd)i)', [(0.0, (0.0, 0.0, 0.0, 1.0), 0), (1.0, (1.0, 1.0, 1.0, 1.0), 0)]),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Volume': voxie.Variant('o', o_Volume_5),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.VolumeGridColor': voxie.Variant('(dddd)', (1.0, 1.0, 0.0, 0.5)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.VolumeGridShow': voxie.Variant('b', False),
})
o_Slice_2.ManualDisplayName = (False, '')
o_Slice_2.GraphPosition = (-160.0, 320.0)
o_Slice_3 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.Visualizer.Slice', {
  'de.uni_stuttgart.Voxie.View2D.CenterPoint': voxie.Variant('(dd)', (0.0, 0.0)),
  'de.uni_stuttgart.Voxie.View2D.VerticalSize': voxie.Variant('d', 0.142),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Filter2DConfiguration': voxie.Variant('s', '<?xml version="1.0"?>\n<!DOCTYPE filterchain>\n<filterchain2d version="1.0"/>\n'),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitive': voxie.Variant('o', None),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitiveColorBehindSlice': voxie.Variant('(dddd)', (0.0, 0.0, 1.0, 1.0)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitiveColorInFrontOfSlice': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 1.0)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitiveColorOnSlice': voxie.Variant('(dddd)', (0.0, 1.0, 0.0, 1.0)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitiveVisibilityDistance': voxie.Variant('d', 0.0),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GridColor': voxie.Variant('(dddd)', (1.0, 1.0, 0.0, 0.5)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GridShow': voxie.Variant('b', False),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GridSpacing': voxie.Variant('d', 0.001),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GridSpacingAutomatic': voxie.Variant('b', True),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.InfoTable': voxie.Variant('o', None),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Interpolation': voxie.Variant('s', 'de.uni_stuttgart.Voxie.Interpolation.Linear'),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Plane': voxie.Variant('o', o_Plane_1),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Plane.Orientation': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 0.0)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Plane.Origin': voxie.Variant('(ddd)', (0.0, 0.0, -0.0002000031527131796)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.RulerColor': voxie.Variant('(dddd)', (1.0, 1.0, 0.0, 0.5)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.RulerShow': voxie.Variant('b', False),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.RulerSpacing': voxie.Variant('d', 0.001),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.RulerSpacingAutomatic': voxie.Variant('b', True),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Show2DFilterMask': voxie.Variant('x', 0),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.ShowSliceCenter': voxie.Variant('b', True),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Surface': voxie.Variant('ao', []),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.ValueColorMapping': voxie.Variant('a(d(dddd)i)', [(0.0, (0.0, 0.0, 0.0, 1.0), 0), (0.01, (1.0, 1.0, 1.0, 1.0), 0)]),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Volume': voxie.Variant('o', o_Volume_2),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.VolumeGridColor': voxie.Variant('(dddd)', (1.0, 1.0, 0.0, 0.5)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.VolumeGridShow': voxie.Variant('b', False),
})
o_Slice_3.ManualDisplayName = (False, '')
o_Slice_3.GraphPosition = (80.0, 160.0)
o_Slice_4 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.Visualizer.Slice', {
  'de.uni_stuttgart.Voxie.View2D.CenterPoint': voxie.Variant('(dd)', (0.0, 0.0)),
  'de.uni_stuttgart.Voxie.View2D.VerticalSize': voxie.Variant('d', 0.142),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Filter2DConfiguration': voxie.Variant('s', '<?xml version="1.0"?>\n<!DOCTYPE filterchain>\n<filterchain2d version="1.0"/>\n'),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitive': voxie.Variant('o', None),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitiveColorBehindSlice': voxie.Variant('(dddd)', (0.0, 0.0, 1.0, 1.0)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitiveColorInFrontOfSlice': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 1.0)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitiveColorOnSlice': voxie.Variant('(dddd)', (0.0, 1.0, 0.0, 1.0)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitiveVisibilityDistance': voxie.Variant('d', 0.0),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GridColor': voxie.Variant('(dddd)', (1.0, 1.0, 0.0, 0.5)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GridShow': voxie.Variant('b', False),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GridSpacing': voxie.Variant('d', 0.001),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GridSpacingAutomatic': voxie.Variant('b', True),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.InfoTable': voxie.Variant('o', None),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Interpolation': voxie.Variant('s', 'de.uni_stuttgart.Voxie.Interpolation.Linear'),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Plane': voxie.Variant('o', o_Plane_1),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Plane.Orientation': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 0.0)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Plane.Origin': voxie.Variant('(ddd)', (0.0, 0.0, -0.0002000031527131796)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.RulerColor': voxie.Variant('(dddd)', (1.0, 1.0, 0.0, 0.5)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.RulerShow': voxie.Variant('b', False),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.RulerSpacing': voxie.Variant('d', 0.001),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.RulerSpacingAutomatic': voxie.Variant('b', True),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Show2DFilterMask': voxie.Variant('x', 0),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.ShowSliceCenter': voxie.Variant('b', True),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Surface': voxie.Variant('ao', []),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.ValueColorMapping': voxie.Variant('a(d(dddd)i)', [(-10.0, (0.4470588235294118, 0.6235294117647059, 0.8117647058823529, 1.0), 0), (0.0, (0.0, 0.0, 0.0, 1.0), 0), (10.0, (1.0, 1.0, 1.0, 1.0), 0)]),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Volume': voxie.Variant('o', o_Volume_7),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.VolumeGridColor': voxie.Variant('(dddd)', (1.0, 1.0, 0.0, 0.5)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.VolumeGridShow': voxie.Variant('b', False),
})
o_Slice_4.ManualDisplayName = (False, '')
o_Slice_4.GraphPosition = (-80.0, 480.0)
instance.Gui.MdiViewMode = 'de.uni_stuttgart.Voxie.MdiViewMode.SubWindowTiled'
