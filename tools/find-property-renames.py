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

import sys
import glob
import os
import json
import codecs
import subprocess
import io
import argparse

import property_types
import codegen_utils
import prototype_helpers


parser = argparse.ArgumentParser()
prototype_helpers.add_arguments(parser)
args = parser.parse_args()
files = prototype_helpers.find_json_files(args)

for path in files:
    with open(path, 'rb') as file:
        data = json.load(codecs.getreader('utf-8')(file))

    if 'NodePrototype' in data:
        for prototype in data['NodePrototype']:
            name = prototype['Name']
            compatNames = []
            if 'CompatibilityNames' in prototype:
                compatNames = prototype['CompatibilityNames']
            if name == 'de.uni_stuttgart.Voxie.Filter.ConnectedComponentAnalysis':
                compatNames.append('de.uni_stuttgart.Voxie.Filter.CCA')
            if name == 'de.uni_stuttgart.Voxie.Filter.ColorizeLabeledSurface':
                compatNames.append('de.uni_stuttgart.Voxie.ColorizedLabeledSurface')
            if name == 'de.uni_stuttgart.Voxie.Filter.NonLocalMeans':
                compatNames.append('de.uni_stuttgart.Voxie.NonLocalMeansFilter')
            if name == 'de.uni_stuttgart.Voxie.Visualizer.Histogram':
                compatNames.append('de.uni_stuttgart.Voxie.ScatterPlotVisualizer')
            if name == 'de.uni_stuttgart.Voxie.Example.Filter.CreateBoxSurface':
                compatNames.append('de.uni_stuttgart.Voxie.CreateBoxSurface')
            if name == 'de.uni_stuttgart.Voxie.Example.Filter.ModifySurface':
                compatNames.append('de.uni_stuttgart.Voxie.ModifySurface')
            if name == 'de.uni_stuttgart.Voxie.Filter.ShiftVolume':
                compatNames.append('de.uni_stuttgart.Voxie.Filter.Shift')
            if name == 'de.uni_stuttgart.Voxie.Filter.SmallPoreFilter':
                compatNames.append('de.uni_stuttgart.Voxie.SmallPoreFilter')
            if name == 'de.uni_stuttgart.Voxie.Filter.SmallPoreFilter':
                compatNames.append('de.uni_stuttgart.Voxie.SmallPoreFilter')
            if name == 'de.uni_stuttgart.Voxie.Filter.BigPoreFilter':
                compatNames.append('de.uni_stuttgart.Voxie.BigPoreFilter')
            if name == 'de.uni_stuttgart.Voxie.Filter.ChordFilter':
                compatNames.append('de.uni_stuttgart.Voxie.ChordFilter')
            if name == 'de.uni_stuttgart.Voxie.Filter.GaussianFilter':
                compatNames.append('de.uni_stuttgart.Voxie.GaussianFilter')
            if name == 'de.uni_stuttgart.Voxie.Filter.MedianFilter':
                compatNames.append('de.uni_stuttgart.Voxie.MedianFilter')
            if name == 'de.uni_stuttgart.Voxie.Filter.Sobel':
                compatNames.append('de.uni_stuttgart.Voxie.SobelFilter')
            if name == 'de.uni_stuttgart.Voxie.Filter.Watershed':
                compatNames.append('de.uni_stuttgart.Voxie.Watershed')
            if name.startswith('de.uni_stuttgart.Voxie.Filter.Surface.'):
                compatNames.append('de.uni_stuttgart.Voxie.ModifySurfaceCPP')
            if name == 'de.uni_stuttgart.Voxie.Filter.GradientMaximalSurface':
                compatNames.append('de.uni_stuttgart.Voxie.GradientMaximalSurface')
            # print(name)
            for propertyName in prototype['Properties']:
                if propertyName.startswith(name + '.'):
                    continue
                if propertyName == 'de.uni_stuttgart.Voxie.Input' or propertyName == 'de.uni_stuttgart.Voxie.Output':
                    continue
                if propertyName == 'de.uni_stuttgart.Voxie.BoundingBoxData':
                    continue
                if propertyName == 'de.uni_stuttgart.Voxie.SizeRoundingMode':
                    continue
                if propertyName.startswith('de.uni_stuttgart.Voxie.MovableDataNode.'):
                    continue
                if propertyName.startswith('de.uni_stuttgart.Voxie.View2D.'):
                    continue
                if propertyName.startswith('de.uni_stuttgart.Voxie.Object3D.'):
                    continue
                if propertyName == 'de.uni_stuttgart.Voxie.Threshold':
                    continue
                found = False
                for compat in compatNames:
                    if propertyName.startswith(compat + '.'):
                        newName = name + propertyName[len(compat):]
                        print('%s %s %s' % (name, propertyName, newName))
                        found = True
                        break
                if found:
                    continue
                print('Unknown property: ' + propertyName, file=sys.stderr)
