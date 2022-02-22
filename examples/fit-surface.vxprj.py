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


### de.uni_stuttgart.Voxie.ObjectKind.Data ###
# de.uni_stuttgart.Voxie.GeometricPrimitiveObject #
o_GeometricPrimitiveObject_1 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.GeometricPrimitiveObject', {
})
# de.uni_stuttgart.Voxie.SurfaceObject #
o_SurfaceObject_1 = instance.OpenFileChecked('/home/kiesssn/work/stuff/iso-rpi2-10.ply') # 'de.uni_stuttgart.Voxie.SurfaceObject'
o_SurfaceObject_1.SetPropertiesChecked({
})
# de.uni_stuttgart.Voxie.VolumeObject #
o_VolumeObject_1 = instance.OpenFileChecked('/home/kiesssn/files/stuff/rpi-2.hdf5') # 'de.uni_stuttgart.Voxie.VolumeObject'
o_VolumeObject_1.SetPropertiesChecked({
})

### de.uni_stuttgart.Voxie.ObjectKind.Filter ###
# de.uni_stuttgart.Voxie.FitPlane #
o_FitPlane_1 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.FitPlane', {
  'de.uni_stuttgart.Voxie.FitPlane.GeometricPrimitive': voxie.Variant('o', o_GeometricPrimitiveObject_1),
  'de.uni_stuttgart.Voxie.FitPlane.MaximumDistance': voxie.Variant('d', 0.001),
  'de.uni_stuttgart.Voxie.FitPlane.Point1': voxie.Variant('t', 1),
  'de.uni_stuttgart.Voxie.FitPlane.Point2': voxie.Variant('t', 2),
  'de.uni_stuttgart.Voxie.FitPlane.Point3': voxie.Variant('t', 3),
  'de.uni_stuttgart.Voxie.FitPlane.Surface': voxie.Variant('o', o_SurfaceObject_1),
})

### de.uni_stuttgart.Voxie.ObjectKind.Visualizer ###
# de.uni_stuttgart.Voxie.SliceVisualizer #
o_SliceVisualizer_1 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.SliceVisualizer', {
  'de.uni_stuttgart.Voxie.Input': voxie.Variant('ao', [o_VolumeObject_1]),
  'de.uni_stuttgart.Voxie.SliceVisualizer.GeometricPrimitive': voxie.Variant('o', o_GeometricPrimitiveObject_1),
})
