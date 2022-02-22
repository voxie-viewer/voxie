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

# scripts/setEnv scripts/dumpFilterXml.py

import sys
import os
import dbus
import io
import csv

import voxie

import xml.etree.ElementTree

parser = voxie.parser
parser.add_argument('--json', action='store_true')
args = parser.parse_args()

context = voxie.VoxieContext(args, enableService=True)
instance = context.createInstance()

currentObject = instance.Gui.SelectedObjects[0]

xmlstr = currentObject.GetProperty(
    'de.uni_stuttgart.Voxie.Visualizer.Slice.Filter2DConfiguration').getValue('s')

if args.json:
    doc = xml.etree.ElementTree.fromstring(xmlstr)
    for filter in doc:
        # print (filter)
        print(filter.attrib['selectionJson'].strip())
else:
    print(xmlstr)
