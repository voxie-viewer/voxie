# Implements a DBusContext where objects are reference counted and uses annotations like de.uni_stuttgart.Voxie.ParentInterface, de.uni_stuttgart.Voxie.Interface, de.uni_stuttgart.Voxie.ReferenceCountChange to look for parent interfaces and to find the correct wrapper object
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
import sys
import os
import xml.etree.ElementTree
from voxie import dbusobject

# Note: Nothing here is safe for multithreaded operation

printNotDestroyedWarnings = True

# TODO: Use ClientManager interface?


class Client(object):
    def __init__(self, bus, busName, clientManager, context, *, clientInterface, useCreateClientWithName=False):
        self.bus = bus
        self.clientManager = clientManager
        self.busName = busName
        self.busNameOrNone = self.busName if self.busName != '' else None
        self.path = None

        if useCreateClientWithName:
            self.path = clientManager.CreateClientWithName(
                self.bus.get_unique_name(), {})
        else:
            self.path = clientManager.CreateClient({})

        self.dbus = dbusobject.DBusObject(self.bus.get_object(
            self.busNameOrNone, self.path, introspect=False), [clientInterface], context=context)

    def destroy(self):
        if self.path is not None:
            path = self.path
            try:
                self.clientManager.DestroyClient(self.path, {})
            except dbus.exceptions.DBusException as e:
                name = e.get_dbus_name()
                # If the connection to the DBus daemon is gone or the remote service is already gone or goes away during the call, ignore that (should be cleaned up anyway)
                if name != 'org.freedesktop.DBus.Error.Disconnected' and name != 'org.freedesktop.DBus.Error.ServiceUnknown' and name != 'org.freedesktop.DBus.Error.NoReply':
                    raise
                # If the server is already gone, everything is fine
            self.path = None

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self.destroy()
        return False

    def __del__(self):
        if 'path' in dir(self) and self.path is not None:
            if printNotDestroyedWarnings:
                print('Warning: Client ' + self.path +
                      ' was not destroyed', file=sys.stderr, flush=True)
            self.destroy()


class RefCountObject(object):
    def __init__(self, client, path, incRef=False):
        if type(path) is RefCountObject:
            self.client = path.client
            self.path = path.path
            self.client.dbus.IncRefCount(self.path)
            return
        self.client = client
        self.path = path
        if incRef:
            self.client.dbus.IncRefCount(self.path)

    def clone(self):
        obj = RefCountObject(self.client, self.path, True)
        return obj

    def take(self):
        p = self.path
        self.path = None
        return p

    def destroy(self):
        if self.path is not None:
            path = self.path
            if self.client.path is not None:  # If the client was already destroyed, no need to do anything
                try:
                    self.client.dbus.DecRefCount(self.path)
                except dbus.exceptions.DBusException as e:
                    name = e.get_dbus_name()
                    # If the connection to the DBus daemon is gone or the remote service is already gone or goes away during the call, ignore that (should be cleaned up anyway)
                    if name != 'org.freedesktop.DBus.Error.Disconnected' and name != 'org.freedesktop.DBus.Error.ServiceUnknown' and name != 'org.freedesktop.DBus.Error.NoReply':
                        raise
                    # If the server is already gone, everything is fine
            self.path = None

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self.destroy()
        return False

    def __del__(self):
        if 'path' in dir(self) and self.path is not None:
            if printNotDestroyedWarnings:
                print('Warning: Object ' + self.path +
                      ' was not destroyed', file=sys.stderr, flush=True)
            self.destroy()


def addInterface(context, interfaceList, interfaceName):
    if interfaceName in interfaceList:
        return
    interfaceList.append(interfaceName)
    for child in context.interfaces[interfaceName]:
        if child.tag != 'annotation':
            continue
        if child.attrib['name'] != 'de.uni_stuttgart.Voxie.ParentInterface':
            continue
        addInterface(context, interfaceList, child.attrib['value'])


def withParentInterfaces(context, interfaces):
    li = []
    for interface in interfaces:
        addInterface(context, li, interface)
    return li


def getInterfaceAnnotation(context, xmlElement):
    if xmlElement is None:
        return []
    interfaces = []
    for child in xmlElement:
        if child.tag != 'annotation':
            continue
        if child.attrib['name'] != 'de.uni_stuttgart.Voxie.Interface':
            continue
        interfaceName = child.attrib['value']
        interfaces.append(interfaceName)
    return interfaces


def getRefCountChange(context, xmlElement):
    if xmlElement is None:
        return 0
    res = 0
    for child in xmlElement:
        if child.tag != 'annotation':
            continue
        if child.attrib['name'] != 'de.uni_stuttgart.Voxie.ReferenceCountChange':
            continue
        val = int(child.attrib['value'])
        res += val
    return res


class VoxieDBusCallContext(dbusobject.DBusCallContext):
    def __init__(self, context):
        dbusobject.DBusCallContext.__init__(self)
        self.context = context
        self.refCountArgumentsToClaim = []
        self.refCountArgumentsToRelease = []
    #    print('VoxieDBusCallContext()')
    # def __del__(self):
    #    print('~VoxieDBusCallContext()')

    def success(self):
        # print('success()')
        for obj in self.refCountArgumentsToRelease:
            if obj in self.refCountArgumentsToClaim:
                self.refCountArgumentsToClaim.remove(obj)
                continue
            rco = obj._referenceCountingObject
            if rco is None:
                raise Exception('ReferenceCountingObject is None')
            rco.take()
            obj._referenceCountingObject = None
        self.refCountArgumentsToRelease = []
        newRcos = []
        for i in range(len(self.refCountArgumentsToClaim)):
            obj = self.refCountArgumentsToClaim[i]
            path = obj._objectPath
            newRcos.append(RefCountObject(self.context.client, path))
        for i in range(len(self.refCountArgumentsToClaim)):
            obj = self.refCountArgumentsToClaim[i]
            newRco = newRcos[i]
            oldRco = obj._referenceCountingObject
            if oldRco is None:
                obj._referenceCountingObject = newRco
            else:
                # Object is already reference counted, destroy the new reference
                newRco.destroy()
        self.refCountArgumentsToClaim = []
        pass


class RefCountingContext(dbusobject.DBusObjectContext):
    def __init__(self, busFunction, bus_name, interfaces, *, mainloop=None, enableService=False, clientManagerPath='/de/uni_stuttgart/Voxie/ClientManager', clientManagerInterface='de.uni_stuttgart.Voxie.ClientManager', clientInterface='de.uni_stuttgart.Voxie.Client', useCreateClientWithName=False):
        self.__currentIds = {}

        self.contextSimple = dbusobject.DBusObjectContext(
            interfaces=interfaces)
        dbusobject.DBusObjectContext.__init__(self, interfaces=interfaces)

        if enableService and mainloop is None:
            # import voxie.dbus_mainloop as voxie_dbus_mainloop
            # mainloop = voxie.dbus_mainloop.mainloop
            # def iteration():
            #     voxie.dbus_mainloop.iter()

            import dbus.mainloop.glib as dbus_mainloop_glib
            import signal
            from gi.repository import GLib
            # Create mainloop, prevent glib from changing the SIGINT handler
            handler = signal.getsignal(signal.SIGINT)
            loop = GLib.MainLoop()
            if handler is not None:
                signal.signal(signal.SIGINT, handler)
            mainloop = dbus.mainloop.glib.DBusGMainLoop()

            def iteration():
                loop.get_context().iteration(True)

            self.mainloop = mainloop
            self.iteration = iteration

        self.bus = busFunction(mainloop)
        self.busName = bus_name
        self.busNameOrNone = self.busName if self.busName != '' else None
        self.isPeerToPeerConnection = self.busName == ''

        self.clientManagerObject = self.bus.get_object(
            self.busNameOrNone, clientManagerPath, introspect=False)
        self.clientManager = dbusobject.DBusObject(
            self.clientManagerObject, [clientManagerInterface], context=self.contextSimple)

        self.client = Client(self.bus, self.busName, self.clientManager, self.contextSimple,
                             clientInterface=clientInterface, useCreateClientWithName=useCreateClientWithName)

    def defaultParameters(self, parameterNames, parameterTypes):
        defaultValues = {}
        if len(parameterNames) > 0 and parameterNames[len(parameterNames) - 1] == 'options' and parameterTypes[len(parameterNames) - 1] == 'a{sv}':
            defaultValues[parameterNames[len(parameterNames) - 1]] = {}
        return defaultValues

    def implicitParameters(self, parameterNames, parameterTypes):
        implicitParameters = {}
        if len(parameterNames) > 0 and parameterNames[0] == 'client' and parameterTypes[0] == 'o':
            implicitParameters[parameterNames[0]] = self.client.path
        return implicitParameters

    def createCallContext(self, dbusObject, xmlElement):
        # print ('createCallContext()')
        callContext = VoxieDBusCallContext(self)
        if xmlElement is not None:
            value = dbusObject
            refCountChange = getRefCountChange(self, xmlElement)
            if refCountChange != 0 and refCountChange != 1 and refCountChange != -1:
                raise Exception(
                    'Invalid value for refCountChange: %d' % (refCountChange,))
            if refCountChange < 0:
                if value._referenceCountingObject is None:
                    raise Exception(
                        'Trying to pass object without reference count to method reducing the reference count')
                if value in callContext.refCountArgumentsToRelease:
                    raise Exception(
                        'Trying to reduce object reference count several times')
                callContext.refCountArgumentsToRelease.append(value)
            elif refCountChange > 0:
                callContext.refCountArgumentsToClaim.append(value)
        return callContext

    def getConverterToDBus(self, *, dbusType, dbusObject, dbusObjectInfo, xmlElement, variantLevel):
        if dbusType != 'o':
            return None
        interfaces = getInterfaceAnnotation(self, xmlElement)
        interfaces = withParentInterfaces(self, interfaces)
        refCountChange = getRefCountChange(self, xmlElement)
        # print('getConverterToDBus(%s, %s): %s' % (dbusType, xmlElement, interfaces))
        if refCountChange != 0 and refCountChange != 1 and refCountChange != -1:
            raise Exception('Invalid value for refCountChange: %d' %
                            (refCountChange,))
        isPeerToPeerConnection = self.isPeerToPeerConnection

        def convert(value, callContext):
            if getattr(callContext, 'info', False):
                info = callContext.info
                objBus = info.connection
                if isPeerToPeerConnection:  # TODO: ?
                    objBusName = ''
                else:
                    objBusName = info.connection.get_unique_name()  # TODO: ?
            else:
                # objBus = dbusObject._connection # might not be available for variants
                objBus = dbusObjectInfo['bus']
                # objBusName = dbusObject._busName
                objBusName = dbusObjectInfo['busName']
            if value is None:
                return dbus.ObjectPath('/', variant_level=variantLevel)
            if type(value) == dbus.ObjectPath:
                if value.variant_level != 0:
                    raise Exception('Got dbus.ObjectPath with non-zero variant level')
                return dbus.ObjectPath(value, variant_level=variantLevel)
            if not isinstance(value, dbusobject.DBusObject):
                raise Exception(
                    'Value passed for "o" argument is neither a dbus.ObjectPath nor a dbusobject.DBusObject')
            implementedInterfaces = set(value._interfaces)
            for iface in interfaces:
                if iface not in implementedInterfaces:
                    raise Exception('Expected interface %s is not implemented (Implemented: %s, expected: %s)' % (
                        iface, implementedInterfaces, interfaces))
            if refCountChange < 0:
                if value._referenceCountingObject is None:
                    raise Exception(
                        'Trying to pass object without reference count to method reducing the reference count')
                if value in callContext.refCountArgumentsToRelease:
                    raise Exception(
                        'Trying to reduce object reference count several times')
                callContext.refCountArgumentsToRelease.append(value)
            elif refCountChange > 0:
                callContext.refCountArgumentsToClaim.append(value)
            bus = value._connection
            busName = value._busName
            if bus != objBus:
                # TODO: throw this exception?
                raise Exception('Bus does not match')
            if busName != objBusName:
                raise Exception('Bus name does not match: "%s" "%s"' %
                                (busName, objBusName))
            path = value._objectPath
            if path.variant_level != 0:
                raise Exception('value._objectPath has non-zero variant level')
            return dbus.ObjectPath(path, variant_level=variantLevel)
        return convert

    def getConverterFromDBus(self, dbusType, dbusObject, dbusObjectInfo, xmlElement):
        if dbusType != 'o' and dbusType != '(so)':
            return None
        includesUniqueName = dbusType == '(so)'
        interfaces = getInterfaceAnnotation(self, xmlElement)
        refCountChange = getRefCountChange(self, xmlElement)
        # print('getConverterFromDBus(%s, %s): %s' % (dbusType, xmlElement, interfaces))
        if len(interfaces) == 0:
            # Value will be returned as dbus.ObjectPath / (string, dbus.ObjectPath) tuple
            return None
        # Values returned from DBus cannot reduce the reference count
        if refCountChange != 0 and refCountChange != 1:
            raise Exception('Invalid value for refCountChange: %d' %
                            (refCountChange,))
        isPeerToPeerConnection = self.isPeerToPeerConnection

        def convert(value, callContext):
            if getattr(callContext, 'info', False):
                info = callContext.info
                objBus = info.connection
                if isPeerToPeerConnection:  # TODO: ?
                    objBusName = ''
                else:
                    objBusName = info.connection.get_unique_name()  # TODO: ?
            else:
                # objBus = dbusObject._connection # might not be available for variants
                objBus = dbusObjectInfo['bus']
                # objBusName = dbusObject._busName
                objBusName = dbusObjectInfo['busName']
            if not includesUniqueName:
                if type(value) != dbus.ObjectPath:
                    raise Exception(
                        'Expected a dbus.ObjectPath, got a %s' % (type(value),))
                busName = objBusName
                path = value
            else:
                if type(value) != dbus.Struct:
                    raise Exception(
                        'Expected a dbus.Struct, got a %s' % (type(value),))
                if len(value) != 2:
                    raise Exception(
                        'Expected a dbus.Struct of length 2, got %d' % (len(value),))
                if type(value[0]) != dbus.String:
                    raise Exception(
                        'Expected a dbus.String, got a %s' % (type(value[0]),))
                if type(value[1]) != dbus.ObjectPath:
                    raise Exception(
                        'Expected a dbus.ObjectPath, got a %s' % (type(value[1]),))
                busName = str(value[0])
                path = value[1]
            if value == dbus.ObjectPath('/'):
                return None
            inst = self.makeObject(objBus, busName, path, interfaces)
            if refCountChange > 0:
                callContext.refCountArgumentsToClaim.append(inst)
            return inst
        return convert

    def getImplClass(self, interfaces):
        return dbusobject.DBusObject

    def makeObject(self, bus, busName, path, interfaces, referenceCountingObject=None):
        interfaces = withParentInterfaces(self, interfaces)
        cls = self.getImplClass(interfaces)
        busNameOrNone = busName if busName != '' else None
        obj = bus.get_object(busNameOrNone, path, introspect=False)
        inst = cls(obj, interfaces, context=self,
                   referenceCountingObject=referenceCountingObject)
        return inst

    def nextId(self, prefix):
        prefix = str(prefix)
        if prefix not in self.__currentIds:
            self.__currentIds[prefix] = 0
        id = self.__currentIds[prefix] = self.__currentIds[prefix] + 1
        return '%s/%d' % (prefix, id)


def cast(obj, newInterface):
    if not isinstance(obj, dbusobject.DBusObject):
        raise Exception('value is not a DBusObject')
    if isinstance(newInterface, list):
        interfaces = newInterface
    elif isinstance(newInterface, str):
        interfaces = [newInterface]
    else:
        raise Exception('newInterface is neither a list nor a string')
    oldInterfaces = set(obj._interfaces)
    if 'de.uni_stuttgart.Voxie.DynamicObject' not in oldInterfaces:
        raise Exception(
            'Attempting to perform a dynamic cast but de.uni_stuttgart.Voxie.DynamicObject is not in list of old interfaces (%s)' % (oldInterfaces,))
    isImplicit = True
    missingInterfaces = []
    for interface in interfaces:
        if interface not in oldInterfaces:
            isImplicit = False
            missingInterfaces.append(interface)
    if not isImplicit:
        # TODO: make sure that this method is called on de.uni_stuttgart.Voxie.DynamicObject?
        dynamicInterfaces = obj.SupportedInterfaces
        # dynamicInterfaces = withParentInterfaces(obj._context, dynamicInterfaces) # TODO: Should the parent interfaces of dynamic interfaces also be allowed?
        dynamicInterfacesSet = set(dynamicInterfaces)
        for interface in missingInterfaces:
            if interface not in dynamicInterfacesSet:
                raise Exception('Error casting from %s to %s with dynamic interfaces %s: %s not found' % (
                    repr(obj._interfaces), repr(interfaces), repr(dynamicInterfaces), repr(interface)))
    context = obj._context
    return context.makeObject(obj._connection, obj._busName, obj._objectPath, interfaces, referenceCountingObject=obj._referenceCountingObject)


def isInstance(obj, interface):
    if not isinstance(obj, dbusobject.DBusObject):
        raise Exception('value is not a DBusObject')
    if isinstance(interface, list):
        interfaces = interface
    elif isinstance(interface, str):
        interfaces = [interface]
    else:
        raise Exception('interface is neither a list nor a string')
    oldInterfaces = set(obj._interfaces)
    if 'de.uni_stuttgart.Voxie.DynamicObject' not in oldInterfaces:
        raise Exception(
            'Attempting to perform a dynamic cast but de.uni_stuttgart.Voxie.DynamicObject is not in list of old interfaces (%s)' % (oldInterfaces,))
    isImplicit = True
    missingInterfaces = []
    for interface in interfaces:
        if interface not in oldInterfaces:
            isImplicit = False
            missingInterfaces.append(interface)
    if not isImplicit:
        # TODO: make sure that this method is called on de.uni_stuttgart.Voxie.DynamicObject?
        dynamicInterfaces = obj.SupportedInterfaces
        # dynamicInterfaces = withParentInterfaces(obj._context, dynamicInterfaces) # TODO: Should the parent interfaces of dynamic interfaces also be allowed?
        dynamicInterfacesSet = set(dynamicInterfaces)
        for interface in missingInterfaces:
            if interface not in dynamicInterfacesSet:
                return False
    return True


def castImplicit(obj, newInterface):
    if not isinstance(obj, dbusobject.DBusObject):
        raise Exception('value is not a DBusObject')
    if isinstance(newInterface, list):
        interfaces = newInterface
    elif isinstance(newInterface, str):
        interfaces = [newInterface]
    else:
        raise Exception('newInterface is neither a list nor a string')
    oldInterfaces = set(obj._interfaces)
    for interface in interfaces:
        if interface not in oldInterfaces:
            raise Exception('Attempting to cast to %s which contains interface %s which is not in list of old interfaces (%s)' % (
                interfaces, interface, oldInterfaces))
    context = obj._context
    return context.makeObject(obj._connection, obj._busName, obj._objectPath, interfaces, referenceCountingObject=obj._referenceCountingObject)


def castStatic(obj, newInterface):
    if not isinstance(obj, dbusobject.DBusObject):
        raise Exception('value is not a DBusObject')
    if isinstance(newInterface, list):
        interfaces = newInterface
    elif isinstance(newInterface, str):
        interfaces = [newInterface]
    else:
        raise Exception('newInterface is neither a list nor a string')
    context = obj._context
    return context.makeObject(obj._connection, obj._busName, obj._objectPath, interfaces, referenceCountingObject=obj._referenceCountingObject)


def grabReference(obj):
    if not isinstance(obj, dbusobject.DBusObject):
        raise Exception('value is not a DBusObject')
    context = obj._context
    rco = RefCountObject(context.client, obj._objectPath, True)
    return context.makeObject(obj._connection, obj._busName, obj._objectPath, obj._interfaces, referenceCountingObject=rco)
