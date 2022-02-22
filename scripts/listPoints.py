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

# PYTHONPATH=pythonlib scripts/listPoints.py

import sys
import os
import dbus
import io
import csv

import voxie

import xml.etree.ElementTree

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args, enableService=True)
instance = context.createInstance()

currentObject = instance.Gui.SelectedObjects[0]
data = currentObject.CastTo('de.uni_stuttgart.Voxie.DataObject').Data.CastTo(
    'de.uni_stuttgart.Voxie.GeometricPrimitiveData')

primitives = data.GetPrimitives(0, 2**64 - 1)
# print (primitives)

writer = csv.writer(sys.stdout, quoting=csv.QUOTE_NONNUMERIC)
pointType = instance.Components.GetComponent(
    'de.uni_stuttgart.Voxie.ComponentType.GeometricPrimitiveType', 'de.uni_stuttgart.Voxie.GeometricPrimitive.Point').CastTo('de.uni_stuttgart.Voxie.GeometricPrimitiveType')
for primitive in primitives:
    id = primitive[0]
    type = primitive[1]
    displayName = primitive[2]
    primitiveValues = primitive[3]
    options = primitive[4]
    if type != pointType._objectPath:
        continue
    position = primitiveValues['Position'].getValue('(ddd)')
    writer.writerow([id, displayName, position[0], position[1], position[2]])
