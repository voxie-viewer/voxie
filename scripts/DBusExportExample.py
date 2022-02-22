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

import numpy
import voxie
import dbus
import dbus.service
import time
import sys
import signal

# PYTHONPATH=pythonlib/ scripts/DBusExportExample.py

# qdbus test.test /de/uni_stuttgart/Voxie/Script/ImportRawTest/Client/1

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args, enableService=True)

nameRes = dbus.service.BusName('test.test', bus=context.bus)


class ClientImpl(voxie.DBusExportObject):
    def __init__(self, context):
        voxie.DBusExportObject.__init__(
            self, ['de.uni_stuttgart.Voxie.Client'], context=context)
        self.path = context.nextId(
            '/de/uni_stuttgart/Voxie/Script/ImportRawTest/Client')
        self.add_to_connection(context.bus, self.path)

    def DecRefCount(self, o):
        print('DecRefCount', o)

    def IncRefCount(self, o):
        print('IncRefCount', o)

    def GetReferencedObjects(self):
        return {dbus.ObjectPath('/'): 22.3}

    @property
    def UniqueConnectionName(self):
        return '123'


c = ClientImpl(context=context)

print(context.bus.get_unique_name())

while True:
    context.iteration()
