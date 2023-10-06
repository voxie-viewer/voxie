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

import property_types
import codegen_utils

os.chdir(os.path.join(os.path.dirname(sys.argv[0]), '..'))

files = [
    ('src/Voxie/Data/Prototypes',
     glob.glob('src/Voxie/Data/Prototypes/*.json') +
     glob.glob('src/Voxie/PropertyObjects/Prototypes/*.json') +
     glob.glob('src/Voxie/Node/Prototypes/*.json'), True),
    ('src/PluginFilter/Prototypes',
     glob.glob('src/PluginFilter/Prototypes/*.json'), False),
    ('src/PluginVisSlice/Prototypes',
     glob.glob('src/PluginVisSlice/Prototypes/*.json'), False),
    ('src/PluginVisRaw/Prototypes',
     glob.glob('src/PluginVisRaw/Prototypes/*.json'), False),
    ('src/PluginVisTable/Prototypes',
     glob.glob('src/PluginVisTable/Prototypes/*.json'), False),
    ('src/PluginVis3D/Prototypes',
     glob.glob('src/PluginVis3D/Prototypes/*.json'), False),
    ('src/PluginFitting/Prototypes',
     glob.glob('src/PluginFitting/Prototypes/*.json'), False),
    ('src/PluginExample/Prototypes',
     glob.glob('src/PluginExample/Prototypes/*.json'), False),
    ('src/PluginSegmentation/Prototypes',
     glob.glob('src/PluginSegmentation/Prototypes/*.json'), False),
]


def escapeCppString(str):
    b = bytes(str, 'utf-8')
    s = '"'
    for c in b:
        if c >= 32 and c < 127 and c != '"':
            s += chr(c)
        else:
            s += '\\{0:03o}'.format(c)
    s += '"'
    return s


def getCppType(name):
    if name not in property_types.typesWithCompat:
        raise Exception('Unknown property type name: ' + repr(name))
    ptype = property_types.typesWithCompat[name]
    if 'QtType' in ptype:
        return ptype['QtType']
    else:
        # return property_types.dbusToQtTypeAll[ptype['DBusSignature']]
        return property_types.dbusToCppRawType(ptype['DBusSignature'])


def getCppTypeRaw(name):
    if name not in property_types.typesWithCompat:
        raise Exception('Unknown property type name: ' + repr(name))
    ptype = property_types.typesWithCompat[name]
    if 'RawType' in ptype:
        return ptype['RawType']
    else:
        # return property_types.dbusToQtTypeAll[ptype['DBusSignature']]
        return property_types.dbusToCppRawType(ptype['DBusSignature'])


def getShortName(name):
    if name not in property_types.typesWithCompat:
        raise Exception('Unknown property type name: ' + repr(name))
    ptype = property_types.typesWithCompat[name]

    sname = ptype['Name']
    if '.' in sname:
        sname = sname[sname.rindex('.') + 1:]
    if 'ShortName' in ptype:
        sname = ptype['ShortName']

    return sname


for (outPrefix, inputFiles, doExport) in files:
    exportStr = ''
    if doExport:
        exportStr = 'VOXIECORESHARED_EXPORT '
    inputFiles.sort()
    allPrototypes = []
    for path in inputFiles:
        with open(path, 'rb') as file:
            data = json.load(codecs.getreader('utf-8')(file))
        # print (data)
        found = False
        if 'Prototypes' in data:
            allPrototypes += data['Prototypes']
            print('Warning: Got "Prototypes" entry in ' + repr(path))
            found = True
        if 'ObjectPrototype' in data:
            allPrototypes += data['ObjectPrototype']
            print('Warning: Got "ObjectPrototype" entry in ' + repr(path))
            found = True
        if 'NodePrototype' in data:
            allPrototypes += data['NodePrototype']
            found = True
        if not found:
            print('Warning: No node prototypes found in ' + repr(path))
    allPrototypes.sort(key=lambda p: p['Name'])

    # print (allPrototypes)

    with open(outPrefix + '.forward.hpp.new', 'w') as hppf, open(outPrefix + '.hpp.new', 'w') as hpp, open(outPrefix + '.cpp.new', 'w') as cpp:
        for file in [hppf, hpp, cpp]:
            file.write(
                '// This file was automatically generated by tools/update-node-prototypes.py\n')
            file.write('// All changes to this file will be lost\n')
            file.write('\n')
        hppf.write('#pragma once\n')
        hppf.write('\n')
        hpp.write('#pragma once\n')
        hpp.write('\n')
        hpp.write('#include <Voxie/Node/Node.hpp>\n')
        hpp.write('#include <QtCore/QObject>\n')
        hpp.write('#include <QtCore/QPointer>\n')
        hpp.write('#include <QtCore/QList>\n')
        hpp.write('#include <QtCore/QJsonObject>\n')
        # hpp.write('#include <QtCore/QDebug>\n')
        hpp.write('#include <QtGui/QVector3D>\n')
        hpp.write('#include <QtGui/QQuaternion>\n')
        hpp.write('#include <Voxie/Data/Color.hpp>\n')
        hpp.write('#include <Voxie/Data/ColorizerEntry.hpp>\n')
        hpp.write('#include <VoxieBackend/Data/DataType.hpp>\n')
        hpp.write('#include <Voxie/Node/Types.hpp>\n')  # TODO: Add this include?
        hpp.write('\n')
        cpp.write('#include "Prototypes.hpp"\n')
        cpp.write('\n')
        cpp.write('#include <Voxie/Node/PropertyValueConvertRaw.hpp>\n')
        cpp.write('#include <Voxie/Node/PropertyValueConvertDBus.hpp>\n')
        cpp.write('#include <Voxie/Node/NodePrototype.hpp>\n')
        hppf.write('namespace vx {\n')
        hpp.write('namespace vx {\n')
        cpp.write('namespace vx {\n')

        allPropNames = set()
        for prototype in allPrototypes:
            for pname in prototype['Properties']:
                allPropNames.add(pname[pname.rfind('.') + 1:])
        allPropNames = list(allPropNames)
        allPropNames.sort()
        for pnameBare in allPropNames:
            hpp.write('#ifndef VOXIE_PROP_DEFINED_%s\n' % (pnameBare,))
            hpp.write('#define VOXIE_PROP_DEFINED_%s\n' % (pnameBare,))
            hpp.write('namespace PropType {\n')
            hpp.write('class %s : public vx::PropTypeBase {};\n' % (pnameBare,))
            hpp.write('}\n')
            hpp.write('namespace Prop {\n')
            hpp.write('constexpr vx::PropType::%s %s = {};\n' % (pnameBare, pnameBare))
            hpp.write('}\n')
            hpp.write('#endif\n')

        for prototype in allPrototypes:
            # print (prototype)
            name = prototype['Name']
            nameBare = name[name.rfind('.') + 1:]
            namePropertiesEntry = nameBare + 'PropertiesEntry'
            namePropertiesBase = nameBare + 'PropertiesBase'
            namePropertiesCopy = nameBare + 'PropertiesCopy'
            nameProperties = nameBare + 'Properties'
            propertyKeys = list(prototype['Properties'])
            propertyKeys.sort()

            # Make sure that symbols don't clash when e.g. for de.uni_stuttgart.Voxie.Data.TomographyRawData and de.uni_stuttgart.Voxie.Visualizer.TomographyRawData
            # TODO: Put this into non-inline namespaces?
            parBare = name[:name.rfind('.')]
            parBare = parBare[parBare.rfind('.') + 1:]
            namespace = parBare.lower() + '_prop'
            hppf.write('inline namespace ' + namespace + ' {\n')
            hpp.write('inline namespace ' + namespace + ' {\n')
            cpp.write('inline namespace ' + namespace + ' {\n')

            # Indicates for each pname whether the non-raw property should be in *Base and *Copy
            is_in_base = {}

            hppf.write('class %s;\n' % (namePropertiesEntry,))
            hpp.write('class %s%s : public vx::PropertiesEntryBase {\n' % (exportStr, namePropertiesEntry,))
            hpp.write('%s() = delete;\n' % (namePropertiesEntry,))
            hpp.write('public:\n')
            hpp.write('~%s();\n' % (namePropertiesEntry,))
            cpp.write('%s::~%s(){}\n' %
                      (namePropertiesEntry, namePropertiesEntry,))
            for pname in propertyKeys:
                property = prototype['Properties'][pname]
                pnameBare = pname[pname.rfind('.') + 1:]
                ptype = property['Type']
                is_in_base[pname] = ptype not in ['de.uni_stuttgart.Voxie.PropertyType.NodeReference', 'de.uni_stuttgart.Voxie.PropertyType.NodeReferenceList', 'de.uni_stuttgart.Voxie.PropertyType.OutputNodeReference']
                ptypeCpp = getCppType(ptype)
                ptypeCppRaw2 = getCppTypeRaw(ptype)
                hpp.write('%s(vx::PropType::%s, %s);\n' % (namePropertiesEntry, pnameBare, ptypeCpp))
                cpp.write('%s::%s(vx::PropType::%s, %s value_) : vx::PropertiesEntryBase(%s, QVariant::fromValue<%s>(vx::PropertyValueConvertRaw<%s, %s>::toRaw(value_))) {}\n' % (namePropertiesEntry, namePropertiesEntry, pnameBare, ptypeCpp, escapeCppString(pname), ptypeCppRaw2, ptypeCppRaw2, ptypeCpp))
            hpp.write('};\n')

            hppf.write('class %s;\n' % (namePropertiesBase,))
            hpp.write('class %s%s {\n' % (exportStr, namePropertiesBase,))
            hpp.write('public:\n')
            hpp.write('virtual ~%s();\n' % (namePropertiesBase,))
            cpp.write('%s::~%s(){}\n' %
                      (namePropertiesBase, namePropertiesBase,))
            for pname in propertyKeys:
                property = prototype['Properties'][pname]
                pnameBare = pname[pname.rfind('.') + 1:]
                ptype = property['Type']
                ptypeCpp = getCppType(ptype)
                ptypeCppRaw2 = getCppTypeRaw(ptype)
                getterName = pnameBare[0].lower() + pnameBare[1:]
                if is_in_base[pname]:
                    hpp.write('virtual %s %s() = 0;\n' % (ptypeCpp, getterName))
                hpp.write('virtual %s %sRaw() = 0;\n' %
                          (ptypeCppRaw2, getterName))
            hpp.write('};\n')

            hppf.write('class %s;\n' % (namePropertiesCopy,))
            hpp.write('class %s%s : public %s {\n' % (
                exportStr, namePropertiesCopy, namePropertiesBase,))
            hpp.write(
                'QSharedPointer<const QMap<QString, QVariant>> _properties;\n')
            hpp.write('public:\n')
            hpp.write('%s(const QSharedPointer<const QMap<QString, QVariant>>& properties);\n' % (
                namePropertiesCopy,))
            cpp.write('%s::%s(const QSharedPointer<const QMap<QString, QVariant>>& properties) : _properties(properties) {}\n' % (
                namePropertiesCopy, namePropertiesCopy,))
            for pname in propertyKeys:
                property = prototype['Properties'][pname]
                pnameBare = pname[pname.rfind('.') + 1:]
                ptype = property['Type']
                ptypeCpp = getCppType(ptype)
                ptypeCppRaw2 = getCppTypeRaw(ptype)
                getterName = pnameBare[0].lower() + pnameBare[1:]
                if is_in_base[pname]:
                    hpp.write('%s %s() override final;\n' % (ptypeCpp, getterName))
                    cpp.write('%s %s::%s() {\n' % (
                        ptypeCpp, namePropertiesCopy, getterName))
                    cpp.write('return vx::PropertyValueConvertRaw<%s, %s>::fromRaw(vx::Node::parseVariant<%s>((*_properties)[%s]));\n' % (
                        ptypeCppRaw2, ptypeCpp, ptypeCppRaw2, escapeCppString(pname),))
                    cpp.write('}\n')
                hpp.write('%s %sRaw() override final;\n' %
                          (ptypeCppRaw2, getterName))
                cpp.write('%s %s::%sRaw() {\n' % (
                    ptypeCppRaw2, namePropertiesCopy, getterName))
                cpp.write('return vx::Node::parseVariant<%s>((*_properties)[%s]);\n' % (
                    ptypeCppRaw2, escapeCppString(pname),))
                cpp.write('}\n')
            hpp.write('};\n')

            hppf.write('class %s;\n' % (nameProperties,))
            hpp.write('class %s%s : public QObject, public %s {\n' % (
                exportStr, nameProperties, namePropertiesBase,))
            hpp.write('Q_OBJECT\n')
            hpp.write('vx::Node* _node;\n')
            hpp.write('public:\n')

            namePrototypeJson = '_prototype_' + nameBare + '_'
            hpp.write('static const char* _getPrototypeJson();\n')
            hpp.write('static QSharedPointer<vx::NodePrototype> getNodePrototype();\n')
            cpp.write('static const char ' + namePrototypeJson + '[] = {\n')
            f = io.StringIO()
            json.dump(prototype, f, allow_nan=False,
                      sort_keys=True, ensure_ascii=False)
            s = bytes(f.getvalue(), 'utf-8')
            for c in s:
                cpp.write('%d, ' % (c,))
            cpp.write('0')
            cpp.write('\n')
            cpp.write('};\n')
            cpp.write(
                'const char* %s::_getPrototypeJson() {\n' % (nameProperties,))
            cpp.write('return ' + namePrototypeJson + ';\n')
            cpp.write('}\n')
            cpp.write('\n')

            hpp.write('%s(vx::Node* parent);\n' % (nameProperties,))
            # cpp is written later

            hpp.write('~%s();\n' % (nameProperties,))
            cpp.write('%s::~%s() {\n' % (nameProperties, nameProperties))
            cpp.write('}\n')
            cpp.write('\n')

            ctorCode = ''
            for pname in propertyKeys:
                property = prototype['Properties'][pname]
                pnameBare = pname[pname.rfind('.') + 1:]
                ptype = property['Type']
                ptypeCpp = getCppType(ptype)
                ptypeCppRaw2 = getCppTypeRaw(ptype)
                ptypeShort = getShortName(ptype)
                getterName = pnameBare[0].lower() + pnameBare[1:]
                setterName = 'set' + pnameBare[0].upper() + pnameBare[1:]
                eventName = pnameBare[0].lower() + pnameBare[1:] + 'Changed'
                propName = pnameBare[0].upper() + pnameBare[1:]
                hpp.write('\n')

                non_raw_modifies = ''
                if is_in_base[pname]:
                    non_raw_modifies = ' override final'

                hpp.write('%s %s()%s;\n' % (ptypeCpp, getterName, non_raw_modifies))
                cpp.write('%s %s::%s() {\n' %
                          (ptypeCpp, nameProperties, getterName))
                cpp.write('return vx::PropertyValueConvertRaw<%s, %s>::fromRaw(_node->getNodePropertyTyped<%s>(%s));\n' %
                          (ptypeCppRaw2, ptypeCpp, ptypeCppRaw2, escapeCppString(pname),))
                cpp.write('}\n')

                hpp.write('%s %sRaw() override final;\n' %
                          (ptypeCppRaw2, getterName))
                cpp.write('%s %s::%sRaw() {\n' % (
                    ptypeCppRaw2, nameProperties, getterName))
                cpp.write('return _node->getNodePropertyTyped<%s>(%s);\n' %
                          (ptypeCppRaw2, escapeCppString(pname),))
                cpp.write('}\n')

                hpp.write(
                    'static QSharedPointer<NodeProperty> %sProperty();\n' % (getterName))
                hpp.write(
                    'static NodePropertyTyped<vx::types::%s> %sPropertyTyped();\n' % (ptypeShort, getterName))
                cpp.write('QSharedPointer<NodeProperty> %s::%sProperty() {\n' % (
                    nameProperties, getterName))
                cpp.write('return %s::getNodePrototype()->getProperty(%s, false);\n' %
                          (nameProperties, escapeCppString(pname),))
                cpp.write('}\n')
                cpp.write('NodePropertyTyped<vx::types::%s> %s::%sPropertyTyped() {\n' % (ptypeShort, nameProperties, getterName))
                cpp.write('return NodePropertyTyped<vx::types::%s>(%sProperty());\n' % (ptypeShort, getterName,))
                cpp.write('}\n')

                hpp.write('void %s(%s value);\n' % (setterName, ptypeCpp))
                cpp.write('void %s::%s(%s value) {\n' % (
                    nameProperties, setterName, ptypeCpp))
                cpp.write('_node->setNodePropertyTyped<%s>(%s, vx::PropertyValueConvertRaw<%s, %s>::toRaw(value));\n' %
                          (ptypeCppRaw2, escapeCppString(pname), ptypeCppRaw2, ptypeCpp,))
                cpp.write('}\n')
                hpp.write('Q_SIGNALS:\n')
                hpp.write('void %s(%s value);\n' % (eventName, ptypeCpp))
                hpp.write('public:\n')
                # TODO: More efficient way than connecting once for every property?
                propVariable = '_prop_' + pnameBare
                ctorCode += 'auto %s = this->_node->prototype()->getProperty(%s, false);\n' % (propVariable,
                                                                                               escapeCppString(pname))
                ctorCode += 'QObject::connect(this->_node, &vx::Node::propertyChanged, this, [this, %s](const QSharedPointer<NodeProperty>& property, const QVariant& value) {\n' % (
                    propVariable,)
                ctorCode += 'if (property != %s)\n' % (propVariable,)
                ctorCode += 'return;\n'
                ctorCode += '%s valueCasted;\n' % (ptypeCpp,)
                ctorCode += 'try {\n'
                ctorCode += 'valueCasted = vx::PropertyValueConvertRaw<%s, %s>::fromRaw(Node::parseVariant<%s>(value));\n' % (
                    ptypeCppRaw2, ptypeCpp, ptypeCppRaw2,)
                ctorCode += '} catch (vx::Exception& e) {\n'
                ctorCode += 'qCritical() << "Error while parsing property value for event handler for property \\"%s\\":" << e.what();\n' % (pname,)
                ctorCode += 'return;\n'
                ctorCode += '}\n'
                ctorCode += 'Q_EMIT this->%s(valueCasted);\n' % (eventName,)
                ctorCode += '});\n'
                hpp.write('//Q_PROPERTY(%s %s READ %s WRITE %s NOTIFY %s)\n' %
                          (ptypeCpp, propName, getterName, setterName, eventName))

            hpp.write('};\n')
            hpp.write('\n')

            # cpp for ctor
            cpp.write(
                '%s::%s(vx::Node* parent) : QObject (parent) {\n' % (nameProperties, nameProperties))
            cpp.write('this->_node = parent;\n')
            cpp.write(ctorCode)
            cpp.write('}\n')
            cpp.write('\n')

            cpp.write('\n')

            # Close namespace
            hppf.write('}\n')
            hpp.write('}\n')
            cpp.write('}\n')
        hppf.write('}\n')
        hpp.write('}\n')
        cpp.write('}\n')
    codegen_utils.formatAndRename(outPrefix + '.forward.hpp')
    codegen_utils.formatAndRename(outPrefix + '.hpp')
    codegen_utils.formatAndRename(outPrefix + '.cpp')
