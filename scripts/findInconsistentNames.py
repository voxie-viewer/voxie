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
import json
import io

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

for prototype in instance.Components.ListComponents('de.uni_stuttgart.Voxie.ComponentType.NodePrototype'):
    prototype = prototype.CastTo('de.uni_stuttgart.Voxie.NodePrototype')
    # print (prototype)
    prototypeName = prototype.Name
    for property in prototype.ListProperties():
        propertyName = property.Name
        propertyJson = property.PropertyDefinition
        if not propertyName.startswith(prototypeName + '.') and propertyName != 'de.uni_stuttgart.Voxie.Input' and propertyName != 'de.uni_stuttgart.Voxie.Output':
            print('%s => %s' % (prototypeName, propertyName))
        if propertyJson['Type'] == 'de.uni_stuttgart.Voxie.PropertyType.Enumeration':
            # print(propertyJson['EnumEntries'])
            for enumEntryName in propertyJson['EnumEntries']:
                if not enumEntryName.startswith(propertyName + '.'):
                    print('%s => %s => %s' % (prototypeName, propertyName, enumEntryName))
