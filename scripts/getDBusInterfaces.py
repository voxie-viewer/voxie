#!/usr/bin/python3

# code/scripts/getDBusInterfaces.py > code/scripts/de.uni_stuttgart.Voxie.xml
# code/scripts/getDBusInterfaces.py | grep 'interface name' | sed 's/.*name="//;s/">//' | sort
# git grep "Q_CLASSINFO(.D-Bus Interface" | sed 's/.*D-Bus Interface", "//;s/")//' | sort 

import sys
import os
import dbus
import io

import voxie

import xml.etree.ElementTree

args = voxie.parser.parse_args()

instance = voxie.Voxie(args)

ds = voxie.DataSet (instance, instance.getPlugin ('ExamplePlugin').getMemberDBus ('de.uni_stuttgart.Voxie.Importer', 'TheSphereGenerator').GenerateSphere (10, voxie.emptyOptions))
slice = ds.createSlice ()
client = instance.createClient()
image = instance.createImage(client, (10, 10))
vis3d = instance, instance.getPlugin ('Voxie3D').getMemberDBus ('de.uni_stuttgart.Voxie.VisualizerFactory', 'IsosurfaceMetaVisualizer').Create ([ds.path], dbus.Array(signature='o'), voxie.emptyOptions)
vis2d = instance, instance.getPlugin ('SliceView').getMemberDBus ('de.uni_stuttgart.Voxie.VisualizerFactory', 'SliceMetaVisualizer').Create (dbus.Array(signature='o'), [slice.path], voxie.emptyOptions)

interfacesList = []

def walk(name):
    #print (name)
    obj = instance.bus.get_object (instance.bus_name, name)
    introspectable = dbus.Interface (obj, 'org.freedesktop.DBus.Introspectable')
    result = introspectable.Introspect()
    if not isinstance(result, dbus.String):
        raise Exception('Got a non-string reply')
    #print (result)
    doc = xml.etree.ElementTree.fromstring (result)
    childNodes = []
    for child in doc:
        if child.tag == 'interface':
            interfacesList.append ((child, name))
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

walk ('/')

node = xml.etree.ElementTree.Element ('node')

interfaces = {}
interfacesPaths = {}

for (interface, path) in interfacesList:
    name = interface.attrib['name']
    if name.startswith ('org.freedesktop.DBus.'):
        continue
    if name in interfaces:
        output1 = io.StringIO()
        xml.etree.ElementTree.ElementTree(interfaces[name]).write(output1, encoding='unicode')
        output2 = io.StringIO()
        xml.etree.ElementTree.ElementTree(interface).write(output2, encoding='unicode')
        if output1.getvalue() != output2.getvalue():
            #raise Exception("Got different data for interface '%1s' at %2s and %3s:\n%2s\n%3s\n" % (name, interfacesPaths[name], path, output1.getvalue(), output2.getvalue()))
            raise Exception("Got different data for interface '%1s' at %2s and %3s" % (name, interfacesPaths[name], path))
            #sys.stderr.write ("Got different data for interface '%1s' at %2s and %3s\n" % (name, interfacesPaths[name], path))
    else:
        interfaces[name] = interface
        interfacesPaths[name] = path
    #print (name)

interfaceNames = list(interfaces.keys())
interfaceNames.sort()
for name in interfaceNames:
    interface = interfaces[name]
    node.append (interface)

node.text = '\n  '

l = len(node)
if l != 0:
    node[l-1].tail = '\n'
else:
    node.text = '\n'

node.tail = '\n'

xml.etree.ElementTree.ElementTree(node).write(sys.stdout.buffer, encoding='utf-8', xml_declaration=True)
