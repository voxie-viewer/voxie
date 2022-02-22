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
import sys
import matplotlib.pyplot as plt

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

vals = []
for obj in instance.ListNodes():
    ty = obj.Prototype.Name
    if ty != 'de.uni_stuttgart.Voxie.Data.TomographyRawData':
        continue

    name = obj.DisplayName
    data = obj.CastTo('de.uni_stuttgart.Voxie.DataNode').Data.CastTo('de.uni_stuttgart.Voxie.TomographyRawData2DAccessor')
    imageKind = data.GetAvailableImageKinds()[0]
    shape = data.GetImageShape('', 0)
    # print(shape)
    center = shape[1] // 2
    with instance.CreateTomographyRawData2DRegular((shape[0], 1), 1, ('float', 32, 'native')) as img:
        data.ReadImages(imageKind, [('', 0)], (0, center), (img._busName, img), 0, (0, 0), (shape[0], 1))
        val = img[:, 0, 0]
    # print(val)
    vals.append((name, val))

names = []
for name, val in vals:
    plt.plot(val)
    names.append(name)
plt.legend(names)

plt.grid()
plt.show()
