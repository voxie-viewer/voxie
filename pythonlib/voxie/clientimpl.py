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
import dbus

# TODO: performance?


def registerNOCHandler(conn, uniqueConnectionName, client, context):
    if len(uniqueConnectionName) == 0 or uniqueConnectionName[0] != ':':
        raise Exception('Connection name %s is not a unique connection name' % (
            repr(uniqueConnectionName),))
    busName = "org.freedesktop.DBus"
    path = "/org/freedesktop/DBus"
    iface = voxie.DBusObject(conn.get_object(busName, path, introspect=False), [
                             'org.freedesktop.DBus'], context=context)
    handle = []

    def handler(name, oldOwner, newOwner):
        if name != uniqueConnectionName:
            return
        if newOwner != "":
            return
        print('Client died', flush=True)
        client.destroy()
        handle[0].remove()
    print('Register handler for %s' % uniqueConnectionName, flush=True)
    handle.append(iface.NameOwnerChanged(handler))
    if not iface.NameHasOwner(uniqueConnectionName):
        print('Client already disappeared')
        handler(uniqueConnectionName, '', '')
    return handle[0]


class ClientImpl(voxie.DBusExportObject):
    def __init__(self, manager, conn, uniqueConnectionName, context):
        self.manager = manager
        voxie.DBusExportObject.__init__(
            self, ['de.uni_stuttgart.Voxie.Client'], context=context)
        self.path = context.nextId('/de/uni_stuttgart/Voxie/Script/Client')
        self.uniqueConnectionName = uniqueConnectionName if uniqueConnectionName is not None else ''
        self.references = {}
        self.handler = None

        def onDisconnect(conn2):
            print('Connection died', flush=True)
            self.destroy()
        conn.call_on_disconnection(onDisconnect)

        if self.path != '' and self.uniqueConnectionName != '':
            self.handler = registerNOCHandler(
                conn, self.uniqueConnectionName, self, context)

        self.manager.clients[self.path] = self
        self.add_to_connection(conn, self.path)

    @property
    def UniqueConnectionName(self):
        return self.uniqueConnectionName

    @property
    def DBusConnectionName(self):
        return 'NotImplemented'

    def DecRefCount(self, o):
        if o in self.references:
            count = self.references[o]
            if count <= 1:
                del self.references[o]
                if o in self.manager.objects:
                    self.manager.objects[o].decRefCount()
            else:
                self.references[o] = count - 1
            return
        else:
            raise Exception('Could not find reference')

    def IncRefCount(self, o):
        if o not in self.manager.objects:
            raise Exception('Could not find object')
        obj = self.manager.objects[o]
        if o in self.references:
            self.references[o] = self.references[o] + 1
            return
        obj.incRefCount()
        self.references[o] = 1

    def GetReferencedObjects(self, o):
        raise Exception('Not implemented')

    def destroy(self):
        for o in list(self.references):
            del self.references[o]
            if o in self.manager.objects:
                self.manager.objects[o].decRefCount()
        del self.manager.clients[self.path]
        if self.handler is not None:
            self.handler.remove()
            self.handler = None

# TODO: Handle death of client / Voxie


class ClientManagerImpl(voxie.DBusExportObject):
    def __init__(self, context, conn):
        self.clients = {}
        self.objects = {}
        self.context = context
        self.path = "/de/uni_stuttgart/Voxie/ClientManager"
        self.conn = conn
        voxie.DBusExportObject.__init__(
            self, ['de.uni_stuttgart.Voxie.ClientManager'], context=context)
        self.add_to_connection(conn, self.path)

    def CreateClient(self, options, *, dbusServiceCallInfo):
        message = dbusServiceCallInfo.message
        # print('Sender: ' + repr(message.get_sender()) + '\n', flush=True)
        return dbus.ObjectPath(ClientImpl(self, self.conn, message.get_sender(), context=self.context).path)

    def CreateIndependentClient(self, options):
        return dbus.ObjectPath(ClientImpl(self, self.conn, '', context=self.context).path)

    def DestroyClient(self, client, options):
        if type(client) != dbus.ObjectPath:
            client = client._objectPath  # TODO: somehow prevent the creation of a wrapper here?
        if client in self.clients:
            self.clients[client].destroy()
            return
        else:
            raise Exception('Could not find client')
