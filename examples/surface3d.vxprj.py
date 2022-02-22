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
# de.uni_stuttgart.Voxie.SurfaceObject #
#o_SurfaceObject_1 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.SurfaceObject', {})
o_SurfaceObject_1 = instance.OpenFile('/home/kiesssn/work/stuff/iso-rpi2-10.ply')

### de.uni_stuttgart.Voxie.ObjectKind.Object3D ###
# de.uni_stuttgart.Voxie.Object3D.Surface #
o_Surface_1 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.Object3D.Surface', {
  'de.uni_stuttgart.Voxie.Object3D.DrawAxisArrows': voxie.Variant('b', False),
  'de.uni_stuttgart.Voxie.Object3D.DrawBoundingBox': voxie.Variant('b', True),
  'de.uni_stuttgart.Voxie.Object3D.DrawOrigin': voxie.Variant('b', True),
  'de.uni_stuttgart.Voxie.Object3D.Surface.FaceCulling': voxie.Variant('s', 'de.uni_stuttgart.Voxie.Object3D.Surface.FaceCulling.None'),
  'de.uni_stuttgart.Voxie.Object3D.Surface.Surface': voxie.Variant('o', o_SurfaceObject_1),
  'de.uni_stuttgart.Voxie.Object3D.Visible': voxie.Variant('b', True),
})

### de.uni_stuttgart.Voxie.ObjectKind.Visualizer ###
# de.uni_stuttgart.Voxie.Visualizer3D #
o_Visualizer3D_1 = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.Visualizer3D', {
  'de.uni_stuttgart.Voxie.Visualizer3D.Objects': voxie.Variant('ao', [o_Surface_1]),
})
