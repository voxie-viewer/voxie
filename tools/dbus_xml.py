import xml.etree.ElementTree
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

import property_types


def getQtType(sig):
    if len(sig) == 1:
        return None
    elif sig == 'ao' or sig == 'as' or sig == 'ay':
        return None
    # elif sig in property_types.dbusToQtType:
    #     return property_types.dbusToQtType[sig]
    # else:
    #     sys.stderr.write('Warning: unknown signature: %s\n' % sig)
    #     return None
    else:
        return property_types.dbusToCppRawType(sig)


def escapeQtType(type):
    if ',' in type:
        return 'VX_IDENTITY_TYPE((' + type + '))'
    return type


def iterTypes(doc, showMissingOptions, typeCallback):
    for interface in doc.getroot():
        if interface.tag != 'interface':
            continue
        for element in interface:
            # print (element.tag)
            argNrIn = 0
            argNrOut = 0
            children = list(element)
            for child in children:
                if child.tag == 'annotation' and (child.attrib['name'] == 'org.qtproject.QtDBus.QtTypeName' or child.attrib['name'].startswith('org.qtproject.QtDBus.QtTypeName')):
                    element.remove(child)
                    continue
            index = 0
            haveOptions = False
            for child in children:
                index = index + 1
                if child.tag != 'arg':
                    continue
                direction = ''
                if element.tag == 'method':
                    direction = 'in'
                elif element.tag == 'signal':
                    direction = 'out'
                if 'direction' in child.attrib:
                    direction = child.attrib['direction']
                id = None
                if direction == 'in':
                    id = 'In%d' % argNrIn
                    argNrIn = argNrIn + 1
                elif direction == 'out':
                    id = 'Out%d' % argNrOut
                    argNrOut = argNrOut + 1
                if (element.tag == 'method' and direction == 'in') or (element.tag == 'signal' and direction == 'out'):
                    if 'name' in child.attrib and child.attrib['name'] == 'options':
                        haveOptions = True
                typeCallback(child.attrib['type'])
                qtType = getQtType(child.attrib['type'])
                if qtType is not None:
                    qtType = escapeQtType(qtType)
                    if (element.tag == 'method' and direction == 'in') or (element.tag == 'signal' and direction == 'out'):
                        qtType = 'const ' + qtType + '&'
                    element.insert(index, xml.etree.ElementTree.Element('annotation', {
                                   'name': 'org.qtproject.QtDBus.QtTypeName.%s' % id, 'value': qtType}))
                    # Workaround bug in old qdbusxml2cpp (Ubuntu 16.04): Expects an In* argument for signals
                    if element.tag == 'signal':
                        element.insert(index, xml.etree.ElementTree.Element('annotation', {
                                       'name': 'org.qtproject.QtDBus.QtTypeName.In%d' % (argNrOut - 1), 'value': qtType}))
                    index = index + 1
            if showMissingOptions:
                if (element.tag == 'method' or element.tag == 'signal') and not haveOptions:
                    print(
                        interface.attrib['name'], element.attrib['name'], '<' + element.tag + '>')
            # print (element.attrib['name'], argNrIn, argNrOut)
            if element.tag == 'property':
                typeCallback(element.attrib['type'])
                qtType = getQtType(element.attrib['type'])
                if qtType is not None:
                    qtType = escapeQtType(qtType)
                    element.insert(len(element), xml.etree.ElementTree.Element(
                        'annotation', {'name': 'org.qtproject.QtDBus.QtTypeName', 'value': qtType}))
