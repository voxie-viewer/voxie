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

import os
import sys
import subprocess
import io
import shutil
import re

import xml.etree.ElementTree

import property_types
import codegen_utils
import dbus_xml

os.chdir(os.path.join(os.path.dirname(sys.argv[0]), '..'))

srcPrefix = 'src/'
adaptorsHeaderFilenameShort = 'VoxieClient/DBusAdaptors.hpp'
proxiesHeaderFilenameShort = 'VoxieClient/DBusProxies.hpp'
proxiesFilenameShort = 'VoxieClient/DBusProxies.cpp'

adaptorsHeaderFilename = srcPrefix + adaptorsHeaderFilenameShort
proxiesHeaderFilename = srcPrefix + proxiesHeaderFilenameShort
proxiesFilename = srcPrefix + proxiesFilenameShort

# dbusXmlFilename = 'de.uni_stuttgart.Voxie.xml'
dbusXmlData = subprocess.run(['tools/get-dbus-xml.py'], check=True, stdout=subprocess.PIPE).stdout

doc = xml.etree.ElementTree.parse(io.BytesIO(dbusXmlData))

dbus_xml.iterTypes(doc=doc, showMissingOptions=False, typeCallback=lambda type: None)

xml = io.BytesIO()
doc.write(xml, encoding='utf-8', xml_declaration=True)
xml = xml.getvalue()

if os.path.exists('tmp-dbus'):
    shutil.rmtree('tmp-dbus')
os.mkdir('tmp-dbus')
subprocess.run(['qdbusxml2cpp', '-p', 'tmp-dbus/DBusProxies', '-a', 'tmp-dbus/DBusAdaptors'], check=True, input=xml)
proxiesHeader = open('tmp-dbus/DBusProxies.h', 'r', encoding='utf-8').read()
proxies = open('tmp-dbus/DBusProxies.cpp', 'r', encoding='utf-8').read()
adaptorsHeader = open('tmp-dbus/DBusAdaptors.h', 'r', encoding='utf-8').read()
# Don't use adaptors .cpp file
shutil.rmtree('tmp-dbus')

proxiesHeader = (
    '#include <VoxieClient/DBusTypeList.hpp>\n' +
    '#include <VoxieClient/QDBusPendingReplyWrapper.hpp>\n' +
    proxiesHeader
)
# proxiesHeader = re.sub('DBUSPROXIES_H_[0-9]*', 'VOXIECLIENT_DBUSPROXIES_H', proxiesHeader)
# proxiesHeader = re.sub('class ', 'class VOXIECLIENT_EXPORT ', proxiesHeader)
proxiesHeader = '\n'.join([re.sub('^class ', 'class VOXIECLIENT_EXPORT ', s) if ': public' in s else s for s in proxiesHeader.split('\n')])

proxies = proxies.replace('tmp-dbus/DBusProxies.h', proxiesHeaderFilenameShort)

# Remove inline to make sure <https://github.com/mesonbuild/meson/pull/8139> or workaround is not needed. (inline should be implied anyway by this method being defined inside the class.)
proxiesHeader = proxiesHeader.replace(' QDBusPendingReply<', ' Q_REQUIRED_RESULT vx::QDBusPendingReplyWrapper<')
# proxiesHeader = proxiesHeader.replace('inline Q_REQUIRED_RESULT', 'Q_REQUIRED_RESULT inline')
proxiesHeader = proxiesHeader.replace('inline Q_REQUIRED_RESULT', 'Q_REQUIRED_RESULT')

# Fix compilation on gcc 12, also remove "inline"
proxiesHeader = proxiesHeader.replace('inline Q_DECL_DEPRECATED Q_REQUIRED_RESULT', 'Q_DECL_DEPRECATED Q_REQUIRED_RESULT')

adaptorsHeaderNew = '#include <VoxieClient/DBusTypeList.hpp>\n'
inCtor = False
inPropMethod = False
for line in adaptorsHeader.split('\n'):
    line = line.strip()

    if line.endswith('public:'):
        inCtor = True
    elif line.endswith(' // PROPERTIES'):
        inCtor = False
        inPropMethod = True
    elif line.endswith(' // SIGNALS'):
        inPropMethod = False

    if inCtor and line.endswith(';') and line.startswith('virtual'):
        adaptorsHeaderNew += line[:-1] + ' {}\n'
    elif inCtor and line.endswith(';'):
        adaptorsHeaderNew += line[:-1] + ' : QDBusAbstractAdaptor (parent) {}\n'
    elif inPropMethod and line.endswith(';'):
        adaptorsHeaderNew += 'virtual ' + line[:-1] + ' = 0;\n'
    else:
        adaptorsHeaderNew += line + '\n'
adaptorsHeader = adaptorsHeaderNew
adaptorsHeader = '\n'.join([re.sub('^class ', 'class VOXIECLIENT_EXPORT ', s) if ': public' in s else s for s in adaptorsHeader.split('\n')])

with open(proxiesHeaderFilename + '.new', 'w', encoding='utf-8') as file:
    file.write(proxiesHeader)
with open(proxiesFilename + '.new', 'w', encoding='utf-8') as file:
    file.write(proxies)
with open(adaptorsHeaderFilename + '.new', 'w', encoding='utf-8') as file:
    file.write(adaptorsHeader)

codegen_utils.formatAndRename(proxiesHeaderFilename)
codegen_utils.formatAndRename(proxiesFilename)
codegen_utils.formatAndRename(adaptorsHeaderFilename)
