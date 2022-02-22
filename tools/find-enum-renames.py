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
            # print(name)
            for propertyName in prototype['Properties']:
                property = prototype['Properties'][propertyName]
                if property['Type'] != 'de.uni_stuttgart.Voxie.PropertyType.Enumeration':
                    continue

                compatNames = []
                if 'CompatibilityNames' in property:
                    compatNames = property['CompatibilityNames']
                if propertyName == 'de.uni_stuttgart.Voxie.Filter.Sobel.Direction':
                    compatNames.append('de.uni_stuttgart.Voxie.Filter.SobelFilter.Direction')
                if propertyName == 'de.uni_stuttgart.Voxie.Filter.ConstantOperation.Operation':
                    compatNames.append('de.uni_stuttgart.Voxie.BinaryOperation.Operation')

                for enumEntryName in property['EnumEntries']:
                    if enumEntryName.startswith(propertyName + '.'):
                        continue

                    if enumEntryName.startswith('de.uni_stuttgart.Voxie.Interpolation.'):
                        continue
                    if enumEntryName.startswith('de.uni_stuttgart.Voxie.ShadingTechnique.'):
                        continue

                    # print(enumEntryName)
                    found = False
                    for compat in compatNames:
                        if enumEntryName.startswith(compat + '.'):
                            newName = propertyName + enumEntryName[len(compat):]
                            print('%s %s %s %s' % (name, propertyName, enumEntryName, newName))
                            found = True
                            break
                    if found:
                        continue
                    print('Unknown enum entry: ' + enumEntryName, file=sys.stderr)
