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
import os
import json
import codecs
import subprocess
import io
import argparse

import property_types
import codegen_utils
import prototype_helpers


def renameProperty(properties, oldName, newName):
    names = list(properties)
    newData = [(key if key != oldName else newName, properties[key]) for key in names]
    return dict(newData)


def addMember(properties, after, newName, newValue):
    newProp = []
    found = False
    if after is None:
        newProp.append((newName, newValue))
        found = True
    for key in properties:
        newProp.append((key, properties[key]))
        if key == after:
            if found:
                raise Exception('found')
            newProp.append((newName, newValue))
            found = True
    if not found:
        raise Exception('not found')
    return dict(newProp)


def fixCondition(renameData, renameEnumData, name, condition):
    if isinstance(condition, list):
        for entry in condition:
            if isinstance(entry, list) or isinstance(entry, dict):
                fixCondition(renameData, renameEnumData, name, entry)
        return

    for key in condition:
        if isinstance(condition[key], list) or isinstance(condition[key], dict):
            fixCondition(renameData, renameEnumData, name, condition[key])

    if 'Property' in condition:
        newName = getNewName(renameData, name, condition['Property'])
        if newName is not None:
            condition['Property'] = newName

    if 'Values' in condition:
        values = condition['Values']
        for i in range(len(values)):
            newName = getNewName3(renameEnumData, name, condition['Property'], values[i])
            if newName is not None:
                values[i] = newName


def getNewName(renameData, name, propertyName):
    newName = None
    for renName, renPropertyName, renPropertyNewName in renameData:
        if name == renName and propertyName == renPropertyName:
            if newName is not None:
                raise Exception('newName is not None')
            newName = renPropertyNewName
    return newName


def getNewName3(renameEnumData, name, propertyName, enumEntryName):
    newName = None
    for renName, renPropertyName, renEnumEntryName, renEnumEntryNewName in renameEnumData:
        if name == renName and propertyName == renPropertyName and enumEntryName == renEnumEntryName:
            if newName is not None:
                raise Exception('newName is not None')
            newName = renEnumEntryNewName
    return newName


parser = argparse.ArgumentParser()
parser.add_argument('--rename-data', help='file with prototype name, old property name, new property name on each line', type=str)
parser.add_argument('--rename-enum-data', help='file with prototype name, property name, old enum entry name, new enum entry name on each line', type=str)
prototype_helpers.add_arguments(parser)
args = parser.parse_args()

renameData = []
if args.rename_data is not None:
    with open(args.rename_data, 'r') as file:
        for line in file.readlines():
            line = line.strip()
            if line != '':
                lineSplit = line.split(' ')
                if len(lineSplit) != 3:
                    raise Exception('len(lineSplit) != 3')
                renameData.append(lineSplit)

renameEnumData = []
if args.rename_enum_data is not None:
    with open(args.rename_enum_data, 'r') as file:
        for line in file.readlines():
            line = line.strip()
            if line != '':
                lineSplit = line.split(' ')
                if len(lineSplit) != 4:
                    raise Exception('len(lineSplit) != 4')
                renameEnumData.append(lineSplit)

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
            # print(name)
            if 'RunFilterEnabledCondition' in prototype:
                fixCondition(renameData, renameEnumData, name, prototype['RunFilterEnabledCondition'])
            if 'UI' in prototype and 'SidePanelSections' in prototype['UI']:
                fixCondition(renameData, renameEnumData, name, prototype['UI']['SidePanelSections'])
            for propertyName in sorted(prototype['Properties']):
                propertyData = prototype['Properties'][propertyName]
                if 'EnabledCondition' in propertyData:
                    fixCondition(renameData, renameEnumData, name, propertyData['EnabledCondition'])
                newName = getNewName(renameData, name, propertyName)
                if 'ParentProperty' in propertyData:
                    parentNewName = getNewName(renameData, name, propertyData['ParentProperty'])
                    if parentNewName is not None:
                        propertyData['ParentProperty'] = parentNewName
                if newName is None:
                    continue
                if 'CompatibilityNames' not in propertyData:
                    propertyData = addMember(propertyData, None, 'CompatibilityNames', [])
                    prototype['Properties'][propertyName] = propertyData
                propertyData['CompatibilityNames'].append(propertyName)
                prototype['Properties'] = renameProperty(prototype['Properties'], propertyName, newName)

            for propertyName in sorted(prototype['Properties']):
                propertyData = prototype['Properties'][propertyName]
                if propertyData['Type'] != 'de.uni_stuttgart.Voxie.PropertyType.Enumeration':
                    continue
                if 'DefaultValue' in propertyData:
                    newName = getNewName3(renameEnumData, name, propertyName, propertyData['DefaultValue'])
                    if newName is not None:
                        propertyData['DefaultValue'] = newName
                for enumEntryName in propertyData['EnumEntries']:
                    enumEntryData = propertyData['EnumEntries'][enumEntryName]
                    newName = getNewName3(renameEnumData, name, propertyName, enumEntryName)
                    # print (newName)
                    if newName is None:
                        continue
                    if 'CompatibilityNames' not in enumEntryData:
                        enumEntryData = addMember(enumEntryData, None, 'CompatibilityNames', [])
                        propertyData['EnumEntries'][enumEntryName] = enumEntryData
                    enumEntryData['CompatibilityNames'].append(enumEntryName)
                    propertyData['EnumEntries'] = renameProperty(propertyData['EnumEntries'], enumEntryName, newName)

    with open(path + '.new', 'wb') as file:
        # Note: Use sort_keys=False
        json.dump(data, codecs.getwriter('utf-8')(file), allow_nan=False, sort_keys=False, ensure_ascii=False, indent=4)
        file.write(b'\n')
    codegen_utils.rename(path)
