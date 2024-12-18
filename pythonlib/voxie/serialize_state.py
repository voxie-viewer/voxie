import voxie
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
import voxie.json_dbus
import sys
import functools
import dbus
import math

# PYTHONPATH=pythonlib/ python3 -c 'import voxie, voxie.serialize_state; args = voxie.parser.parse_args(); context = voxie.VoxieContext(args); instance = context.createInstance(); voxie.serialize_state.serialize(instance); context.client.destroy()'


class NodeInfo:
    def __init__(self, obj):
        self.obj = obj
        self.path = self.obj._objectPath
        self.prototype = self.obj.Prototype
        self.prototypeName = self.prototype.Name
        self.kind = self.prototype.NodeKind

    def __repr__(self):
        return repr(self.obj)


def isDigit(c):
    return c >= '0' and c <= '9'


def compareStringNumeric(str1, str2):
    pos = 0
    while pos < len(str1) and pos < len(str2) and str1[pos] == str2[pos]:
        pos += 1
    # Check the number of digits in both strings starting at the first character which is different and consider the string with more digits to be larger
    while True:
        isDigit1 = pos < len(str1) and isDigit(str1[pos])
        isDigit2 = pos < len(str2) and isDigit(str2[pos])
        if isDigit1 and not isDigit2:
            return 1
        elif isDigit2 and not isDigit1:
            return -1
        elif not isDigit1 and not isDigit2:
            # Fall back to 'normal' comparison
            if str1 < str2:
                return -1
            elif str1 > str2:
                return 1
            else:
                return 0
        # Else: isDigit1 and isDigit2, continue
        pos = pos + 1


# This objectKindOrder should make sure that all objects which appear as properties of other objects are created before those objects
nodeKindOrder = {
    'de.uni_stuttgart.Voxie.NodeKind.NodeGroup': 1,
    'de.uni_stuttgart.Voxie.NodeKind.Property': 2,
    'de.uni_stuttgart.Voxie.NodeKind.Data': 3,
    'de.uni_stuttgart.Voxie.NodeKind.Object3D': 4,
    'de.uni_stuttgart.Voxie.NodeKind.SegmentationStep': 5,
    'de.uni_stuttgart.Voxie.NodeKind.Filter': 6,
    'de.uni_stuttgart.Voxie.NodeKind.Visualizer': 7,
}


def compareNode(obj1, obj2):
    if obj1.kind in nodeKindOrder:
        if obj2.kind in nodeKindOrder:
            if nodeKindOrder[obj1.kind] < nodeKindOrder[obj2.kind]:
                return -1
            elif nodeKindOrder[obj1.kind] > nodeKindOrder[obj2.kind]:
                return 1
        else:
            return -1
    elif obj2.kind in nodeKindOrder:
        return 1

    res = compareStringNumeric(str(obj1.kind), str(obj2.kind))
    if res < 0:
        return -1
    elif res > 0:
        return 1

    res = compareStringNumeric(
        str(obj1.prototypeName), str(obj2.prototypeName))
    if res < 0:
        return -1
    elif res > 0:
        return 1

    res = compareStringNumeric(str(obj1.path), str(obj2.path))
    if res < 0:
        return -1
    elif res > 0:
        return 1

    return 0

# TODO: remove


def serializePropertyValueSimple(serializedNodes, value):
    if type(value) == tuple or type(value) == list:
        return list(map(lambda x: serializePropertyValueSimple(serializedNodes, x), value))
    if type(value) == dbus.ObjectPath:
        if value == dbus.ObjectPath('/'):
            return None
        elif value in serializedNodes:
            return serializedNodes[value]  # TODO: must not be quoted
        else:
            print('Warning: Could not find node %s' %
                  str(value), file=sys.stderr)
            return None
    if type(value) in [bool, int, float]:
        return value
    print('Got type: %s' % type(value), file=sys.stderr)
    return value


def serializePropertyValue(serializedNodes, sig, value):
    sig = dbus.Signature(sig)
    # return repr(value)
    # return repr(serializePropertyValueSimple(serializedNodes, value))
    if sig == 'b':
        return repr(bool(value))
    elif sig in ['y', 'n', 'q', 'i', 'u', 'x', 't']:
        return repr(int(value))
    elif sig == 'd':
        f = float(value)
        if math.isnan(f):
            return "float('NaN')"
        elif f == float('Infinity'):
            return "float('Infinity')"
        elif f == float('-Infinity'):
            return "float('-Infinity')"
        else:
            return repr(f)
    elif sig == 's':
        return repr(str(value))
    elif sig == 'o':
        if value == dbus.ObjectPath('/'):
            return 'None'
            # return "dbus.ObjectPath('/')"
        elif value in serializedNodes:
            return '%s' % (serializedNodes[value],)
        else:
            print('Warning: Could not find node %s' %
                  str(value), file=sys.stderr)
            return 'None'
            # return "dbus.ObjectPath('/')"
    elif sig[0] == 'a' and sig[1] != '{':
        return '[' + ', '.join([serializePropertyValue(serializedNodes, sig[1:], v) for v in value]) + ']'
    elif sig[0] == '(' and sig[-1] == ')':
        sigs = dbus.Signature(sig[1:-1])
        return '(' + ', '.join([serializePropertyValue(serializedNodes, s, v) for s, v in zip(sigs, value)]) + ')'
    elif sig == 'a{sv}':  # TODO: This assumes this is a JSON-like value
        obj = voxie.json_dbus.dbus_to_json_dict(value)
        # TODO: Is repr() the right thing here?
        return 'voxie.json_dbus.json_to_dbus_dict(' + repr(obj) + ')'
    elif sig == 'v':  # TODO: This assumes this is a JSON-like value
        obj = voxie.json_dbus.dbus_to_json(value)
        # TODO: Is repr() the right thing here?
        return 'voxie.json_dbus.json_to_dbus(' + repr(obj) + ')'
    else:
        print('Warning: Could not serialize DBus signature %s' %
              str(sig), file=sys.stderr)
        return 'None'


def serializeNode(instance, file, serializedNodes, obj, oname):
    propertiesExpr = '{'
    propertiesExpr += '\n'
    for property in obj.prototype.ListProperties():
        propertiesExpr += '  '
        # TODO: Handle errors
        name = property.Name
        try:
            value = obj.obj.GetProperty(name)
        except Exception as e:
            print('Warning: Could not get property %s for node %s: %s' %
                  (repr(name), str(obj.path), e))
            continue
        propertiesExpr += '%s: voxie.Variant(%s, %s),' % (repr(name), repr(str(
            value.signature)), serializePropertyValue(serializedNodes, value.signature, value.value))
        propertiesExpr += '\n'
    propertiesExpr += '}'
    filename = ''
    if obj.kind == 'de.uni_stuttgart.Voxie.NodeKind.Data':
        filename = voxie.cast(
            obj.obj, ['de.uni_stuttgart.Voxie.DataNode']).FileName
    if filename != '':
        objData = obj.obj.CastTo('de.uni_stuttgart.Voxie.DataNode')
        importProperties = objData.ImportProperties
        if len(importProperties) == 0:
            print('%s = instance.OpenFileChecked(%s) # %s' %
                  (oname, repr(filename), repr(obj.prototypeName)), file=file)
        else:
            # TODO: More error handling
            importer = objData.Importer
            importerName = importer.Name
            propertiesExpr2 = '{'
            propertiesExpr2 += '\n'
            for name in importProperties:
                propertiesExpr2 += '  '
                value = importProperties[name]
                propertiesExpr2 += '%s: voxie.Variant(%s, %s),' % (repr(name), repr(str(
                    value.signature)), serializePropertyValue(serializedNodes, value.signature, value.value))
                propertiesExpr2 += '\n'
            propertiesExpr2 += '}'
            print('%s = instance.Components.GetComponent(\'de.uni_stuttgart.Voxie.ComponentType.Importer\', %s).CastTo(\'de.uni_stuttgart.Voxie.Importer\').ImportNode(%s, {\'Properties\': voxie.Variant(\'a{sv}\', %s)}) # %s' %
                  (oname, repr(importerName), repr(filename), propertiesExpr2, repr(obj.prototypeName)), file=file)
        print('if %s is not None:' % (oname), file=file)
        print('    %s.SetPropertiesChecked(%s)' %
              (oname, propertiesExpr), file=file)
    else:
        print('%s = instance.CreateNodeChecked(%s, %s)' %
              (oname, repr(obj.prototypeName), propertiesExpr), file=file)
    print('if %s is not None:' % (oname), file=file)
    mdn = obj.obj.ManualDisplayName
    if mdn != (False, ''):
        print('    %s.ManualDisplayName = %s' % (oname, mdn), file=file)
    print('    %s.GraphPosition = %s' % (oname, obj.obj.GraphPosition), file=file)
    if obj.kind == 'de.uni_stuttgart.Voxie.NodeKind.Visualizer':
        visObj = voxie.cast(
            obj.obj, ['de.uni_stuttgart.Voxie.VisualizerNode'])
        if instance.Gui.MdiViewMode == 'de.uni_stuttgart.Voxie.MdiViewMode.SubWindow' or not visObj.IsAttached:
            # TODO: Add a call to set all values at once? Or what should be the order here?
            print('    %s.CastTo(\'de.uni_stuttgart.Voxie.VisualizerNode\').IsAttached = %s' % (
                oname, visObj.IsAttached), file=file)
            print('    %s.CastTo(\'de.uni_stuttgart.Voxie.VisualizerNode\').VisualizerPosition = %s' % (
                oname, visObj.VisualizerPosition), file=file)
            print('    %s.CastTo(\'de.uni_stuttgart.Voxie.VisualizerNode\').VisualizerSize = %s' % (
                oname, visObj.VisualizerSize), file=file)
            print('    %s.CastTo(\'de.uni_stuttgart.Voxie.VisualizerNode\').WindowMode = %s' % (
                oname, repr(visObj.WindowMode)), file=file)


def serializeGui(instance, file):
    print("### GUI ###", file=file)
    print('instance.Gui.MdiViewMode = \'%s\'' %
          instance.Gui.MdiViewMode, file=file)


def isObjectInNodeGroup(obj, nodeGroupPath):
    if obj.obj.ParentNodeGroup is not None:
        if obj.obj.ParentNodeGroup._objectPath == nodeGroupPath:
            return True
        else:
            return isObjectInNodeGroup(obj.obj.ParentNodeGroup, nodeGroupPath)


def serialize(instance, file=sys.stdout, nodeGroupPath=None):
    """Serialize a voxie instance to a file.

    Args:
        instance: The instance which should be serialized.
        file: File to which the serialized output will be written. Defaults to sys.stdout.
        nodeGroupPath: Defaults to None. If set to none all objects in the project will be serialized.
        If set to the dbus object path of a node group in this instance then only that node group and
        all objects which are (indirect) children of it will be serialized.
    """
    versionInfo = instance.VersionInformation
    print('#!/usr/bin/python3', file=file)
    # TODO: print version string in a different format?
    print('# Stored using voxie version %s' %
          (repr(versionInfo['VersionString'].getValue('s')),), file=file)
    print('import voxie, dbus', file=file)
    print('instance = voxie.instanceFromArgs()\n', file=file)
    lastKind = None
    lastPrototypeName = None
    nodes = list(map(NodeInfo, instance.ListNodes()))
    # print(list(map(lambda x: x.prototype._objectPath, nodes)))
    nodes.sort(key=functools.cmp_to_key(compareNode))
    # TODO: support for reordering if necessary or for use SetProperty() to set the properties later when the ordering above is not sufficient?
    serializedNodes = {}
    # id = 0
    usedIDs = {}

    # if nodeGroupPath is set then filter out all objs in the 'objects' list that are not (indirect) children of the node group.
    if nodeGroupPath is not None:
        nodeGroupObjects = []
        for obj in nodes:
            if isObjectInNodeGroup(obj, nodeGroupPath) or obj.obj._objectPath == nodeGroupPath:
                nodeGroupObjects.append(obj)
        nodes = nodeGroupObjects
        nodes.sort(key=functools.cmp_to_key(compareNode))

    for obj in nodes:
        # print(obj.kind, obj.prototypeName, obj.path)
        if obj.kind != lastKind:
            print('\n### %s ###' % (obj.kind,), file=file)
            lastKind = obj.kind
        if obj.prototypeName != lastPrototypeName:
            print('# %s #' % (obj.prototypeName,), file=file)
            lastPrototypeName = obj.prototypeName

        # id = id + 1
        # name = 'obj%d' % id
        pnameShort = obj.prototypeName
        if '.' in pnameShort:
            pnameShort = pnameShort[(pnameShort.rindex('.') + 1):]
        pnameShortFiltered = ''
        for c in pnameShort:
            if (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z') or (c >= '0' and c <= '9') or c == '_':
                pnameShortFiltered += c
        if pnameShortFiltered not in usedIDs:
            usedIDs[pnameShortFiltered] = 0
        usedIDs[pnameShortFiltered] += 1
        name = 'o_%s_%d' % (pnameShortFiltered, usedIDs[pnameShortFiltered])

        serializeNode(instance, file, serializedNodes, obj, name)
        serializedNodes[obj.path] = name

    # node group relations. We can only set these after we're done with serializing all nodes because we
    # can't guarantee that nested node groups are serialized in the "correct" order (top-level first)
    print("\n### Node Group Relations ###", file=file)
    for obj in nodes:
        parentNodeGroup = obj.obj.ParentNodeGroup
        if parentNodeGroup is not None:
            print('%s.ParentNodeGroup = %s' % (
                serializedNodes[obj.path], serializedNodes[parentNodeGroup._objectPath]), file=file)
            print('%s.ExportedProperties = %s' %
                  (serializedNodes[obj.path], obj.obj.ExportedProperties), file=file)

    serializeGui(instance, file)
