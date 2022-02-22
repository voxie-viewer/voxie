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

# scripts/setEnv scripts/getPrototypeInfo.py

import sys
import os
import dbus
import io

import voxie

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args, enableService=True)
instance = context.createInstance()

allPropertyTypes = set()


def showComponentInfo(component):
    component = voxie.castImplicit(
        component, 'de.uni_stuttgart.Voxie.Component')
    print('  Component:')
    container = component.ComponentContainer
    # print (container.SupportedInterfaces)
    found = False
    for interface in container.SupportedInterfaces:
        if interface == 'de.uni_stuttgart.Voxie.Plugin':
            plugin = container.CastTo('de.uni_stuttgart.Voxie.Plugin')
            print('    Plugin: %s%s' % (repr(plugin.Name),
                                        ' (core plugin)' if plugin.IsCorePlugin else ''))
            found = True
            break
        if interface == 'de.uni_stuttgart.Voxie.Extension':
            extension = container.CastTo('de.uni_stuttgart.Voxie.Extension')
            print('    Extension: %s' % (repr(extension.ExecutableFilename),))
            found = True
            break
    if not found:
        print('    Unknown container')
    print('    Name: %s' % (repr(component.Name),))
    print('    Type: %s' % (repr(component.ComponentType),))
    print()


prototypes = instance.ListPrototypes()
# print (prototypes)
for prototype in prototypes:
    print(prototype.Name)
    print('  DisplayName: %s' % (repr(prototype.DisplayName),))
    print('  Description: %s' % (repr(prototype.Description),))
    # print ('  Allowed Input Types: %s' % ([ t.Name for t in prototype.ListAllowedInputTypes() ],))
    print('  Properties:')
    for prop in prototype.ListObjectProperties():
        print('    %s' % (repr(prop.DisplayName),))
        print('      Type: %s' % (prop.Type.Name,))
        allPropertyTypes.add(prop.Type)
    showComponentInfo(prototype)

if False:
    allPropertyTypes = list(allPropertyTypes)
    allPropertyTypes.sort(key=lambda p: p.Name)
    propertyTypesPrinted = set()
    for ptype in allPropertyTypes:
        if ptype._objectPath in propertyTypesPrinted:
            continue
        propertyTypesPrinted.add(ptype._objectPath)

        print(ptype.Name)
        showComponentInfo(ptype)

context.client.destroy()
