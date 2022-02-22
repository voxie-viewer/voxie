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

# scripts/getDBusInterfaces.py > de.uni_stuttgart.Voxie.xml
# PYTHONPATH=pythonlib scripts/getDBusInterfaces.py | grep 'interface name' | sed 's/.*name="//;s/">//' | sort
# git grep "Q_CLASSINFO(.D-Bus Interface" | sed 's/.*D-Bus Interface", "//;s/")//' | sort

import sys
import os
import dbus
import io

import voxie

import xml.etree.ElementTree

args = voxie.parser.parse_args()

instance = voxie.Voxie(args)

factories = instance.dbus.ListPrototypes()
# print (factories)
for path in factories:
    factory = voxie.DBusObject(instance.bus.get_object(instance.bus_name, path), [
                               'de.uni_stuttgart.Voxie.ObjectPrototype'])
    # print (factory.Name)
    if factory.Name == 'de.uni_stuttgart.Voxie.Data.TomographyRawData':
        continue
    factory.Create({}, [], {})

ds = voxie.VolumeObject(instance, instance.getPlugin('Example').getMemberDBus(
    'de.uni_stuttgart.Voxie.Importer', 'TheSphereGenerator').GenerateSphere(10, voxie.emptyOptions))
slice = ds.createSlice()
client = instance.createClient()
image = instance.createImage(client, (10, 10))
# vis3d = instance, instance.getPlugin ('Vis3D').getMemberDBus ('de.uni_stuttgart.Voxie.VisualizerPrototype', 'IsosurfaceMetaVisualizer').Create ([ds.path], dbus.Array(signature='o'), voxie.emptyOptions)
# vis2d = instance, instance.getPlugin ('VisSlice').getMemberDBus ('de.uni_stuttgart.Voxie.VisualizerPrototype', 'SliceMetaVisualizer').Create (dbus.Array(signature='o'), [slice.path], voxie.emptyOptions)
# visraw = instance, instance.getPlugin ('VisRaw').getMemberDBus ('de.uni_stuttgart.Voxie.VisualizerPrototype', 'RawMetaVisualizer').Create (dbus.Array(signature='o'), dbus.Array(signature='o'), voxie.emptyOptions)

interfacesList = []


def walk(name):
    # print (name)
    obj = instance.bus.get_object(instance.bus_name, name)
    introspectable = dbus.Interface(obj, 'org.freedesktop.DBus.Introspectable')
    result = introspectable.Introspect()
    if not isinstance(result, dbus.String):
        raise Exception('Got a non-string reply')
    # print (result)
    doc = xml.etree.ElementTree.fromstring(result)
    childNodes = []
    for child in doc:
        if child.tag == 'interface':
            interfacesList.append((child, name))
        elif child.tag == 'node':
            parentName = name
            if parentName != '/':
                parentName = parentName + '/'
            childName = parentName + child.attrib['name']
            if childName == parentName:
                raise ('childName == parentName')
            childNodes.append(childName)
        else:
            raise ('Unknown element: ' + child.tag)
    for childName in childNodes:
        walk(childName)


walk('/')

node = xml.etree.ElementTree.Element('node')

interfaces = {}
interfacesPaths = {}

for (interface, path) in interfacesList:
    name = interface.attrib['name']
    if name.startswith('org.freedesktop.DBus.'):
        continue
    if name in interfaces:
        output1 = io.StringIO()
        xml.etree.ElementTree.ElementTree(
            interfaces[name]).write(output1, encoding='unicode')
        output2 = io.StringIO()
        xml.etree.ElementTree.ElementTree(
            interface).write(output2, encoding='unicode')
        if output1.getvalue() != output2.getvalue():
            # raise Exception("Got different data for interface '%1s' at %2s and %3s:\n%2s\n%3s\n" % (name, interfacesPaths[name], path, output1.getvalue(), output2.getvalue()))
            raise Exception("Got different data for interface '%1s' at %2s and %3s" % (
                name, interfacesPaths[name], path))
            # sys.stderr.write ("Got different data for interface '%1s' at %2s and %3s\n" % (name, interfacesPaths[name], path))
    else:
        interfaces[name] = interface
        interfacesPaths[name] = path
    # print (name)

interfaceNames = list(interfaces.keys())
interfaceNames.sort()
for name in interfaceNames:
    interface = interfaces[name]
    node.append(interface)

node.text = '\n  '

le = len(node)
if le != 0:
    node[le - 1].tail = '\n'
else:
    node.text = '\n'

node.tail = '\n'

xml.etree.ElementTree.ElementTree(node).write(
    sys.stdout.buffer, encoding='utf-8', xml_declaration=True)
