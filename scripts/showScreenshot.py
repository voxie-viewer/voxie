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

import numpy as np
import voxie

import matplotlib.pyplot as plt

import scipy.misc

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

vis = instance.Gui.ActiveVisualizer
if vis is None:
    surface = instance.Gui.SelectedObjects[0]

    surface3D = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.Object3D.Surface', {
        'de.uni_stuttgart.Voxie.Object3D.Surface.Surface': voxie.Variant('o', surface),
    })

    vis = instance.CreateObjectChecked('de.uni_stuttgart.Voxie.Visualizer3D', {
        # 'de.uni_stuttgart.Voxie.Visualizer.View3D.Camera.LookAt': voxie.Variant('(ddd)', (0.0, 0.0, 0.0)),
        # 'de.uni_stuttgart.Voxie.Visualizer.View3D.Camera.Orientation': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 0.0)),
        # 'de.uni_stuttgart.Voxie.Visualizer.View3D.Camera.ZoomLog': voxie.Variant('d', 0.0),
        'de.uni_stuttgart.Voxie.Visualizer.View3D.Objects': voxie.Variant('ao', [surface3D]),
    })
    vis = vis.CastTo('de.uni_stuttgart.Voxie.VisualizerObject')

# size = (100, 100)
size = (750, 500)
# size = (7500, 5000)
img = instance.CreateImage(size, 4, ('float', 32, 'native'))
vis.RenderScreenshot(img, (0, 0), size)

# img.GetPILImage().save('/tmp/out.png')
plt.imshow(img.GetMatplotlibImage())
plt.show()
