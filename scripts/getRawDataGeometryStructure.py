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

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

rawDataObj = instance.Gui.SelectedObjects[0].CastTo('de.uni_stuttgart.Voxie.DataObject')
rawData = rawDataObj.Data.CastTo('de.uni_stuttgart.Voxie.TomographyRawData2DAccessor')


def show(geom, indent):
    keys = list(geom.keys())
    keys.sort()
    for key in keys:
        if isinstance(geom[key], dict):
            print(indent + '{}:'.format(key))
            show(geom[key], indent + '  ')
        elif isinstance(geom[key], list):
            if len(geom[key]) > 0 and isinstance(geom[key][0], dict) and 'ImageReference' in geom[key][0]:
                print(indent + '{}: ImageList [{}]'.format(key, len(geom[key])))
            else:
                print(indent + '{}: [{}]'.format(key, len(geom[key])))
                if False:
                    for i in range(len(geom[key])):
                        print(indent + '  {}'.format(i))
                        if isinstance(geom[key][i], dict):
                            show(geom[key][i], indent + '    ')
        else:
            print(indent + '{}'.format(key))


for geomType in rawData.GetAvailableGeometryTypes():
    print('{}:'.format(geomType))
    geom = rawData.GetGeometryData(geomType)
    show(geom, '  ')
    print()

context.client.destroy()
