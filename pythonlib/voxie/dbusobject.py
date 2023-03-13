import dbus
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
import dbus.service

import xml.etree.ElementTree

import sys
import inspect
import io
import os

# Raise an exception if value is not a valid value with type signature


def verifyValue(signature, value):
    signature = dbus.Signature(signature)
    if len(list(signature)) != 1:
        raise Exception('Expected a single complete type')

    if signature == 'y' or signature == 'n' or signature == 'q' or signature == 'i' or signature == 'u' or signature == 'x' or signature == 't':
        if type(value) != int:
            raise Exception('Expected an int, got a %s' % repr(type(value)))
    elif signature == 'd':
        if type(value) != float and type(value) != int:
            raise Exception('Expected a float, got a %s' % repr(type(value)))
    elif signature == 'b':
        if type(value) != bool:
            raise Exception('Expected a bool, got a %s' % repr(type(value)))
    elif signature == 'h':
        if type(value) != dbus.types.UnixFd:
            raise Exception(
                'Expected a dbus.types.UnixFd, got a %s' % repr(type(value)))
        if 'variant_level' in dir(value):
            # Starting with dbus-python 1.2.10 UnixFd has a variant_level
            if value.variant_level != 0:
                raise Exception('Got dbus.ObjectPath with non-zero variant level')
    elif signature == 's':
        if type(value) != str:
            raise Exception('Expected a str, got a %s' % repr(type(value)))
    elif signature == 'o':
        if type(value) != dbus.ObjectPath:
            raise Exception('Expected a dbus.ObjectPath, got a %s' %
                            repr(type(value)))
        if value.variant_level != 0:
            raise Exception('Got dbus.ObjectPath with non-zero variant level')
    elif signature == 'g':
        if type(value) != dbus.Signature:
            raise Exception('Expected a dbus.Signature, got a %s' %
                            repr(type(value)))
        if value.variant_level != 0:
            raise Exception('Got dbus.Signature with non-zero variant level')
    elif signature == 'v':
        if type(value) != Variant:
            raise Exception('Expected a Variant, got a %s' % repr(type(value)))
    elif signature[0:2] == 'a{':
        if signature[-1] != '}':
            raise Exception("signature[-1] != '}'")
        types = list(dbus.Signature(signature[2:-1]))
        if len(types) != 2:
            raise Exception('len(types) != 2')
        keyType = types[0]
        valueType = types[1]

        if type(value) != dict:
            raise Exception('Expected a dict, got a %s' % repr(type(value)))

        for key in value:
            verifyValue(keyType, key)
            verifyValue(valueType, value[key])
    elif signature[0] == 'a':
        valueType = signature[1:]

        if type(value) != list:
            raise Exception('Expected a list, got a %s' % repr(type(value)))

        for member in value:
            verifyValue(valueType, member)
    elif signature[0] == '(':
        types = list(dbus.Signature(signature[1:-1]))

        if type(value) != tuple:
            raise Exception('Expected a tuple, got a %s' % repr(type(value)))
        if len(types) != len(value):
            raise Exception('Expected a tuple with %d elements, got %d' % (
                len(types), len(value)))

        for i in range(len(types)):
            verifyValue(types[i], value[i])
    else:
        raise Exception('Unknown signature: %s' % signature)

# TODO: This is specific for VoxieContext


def convertObjectPath(val):
    if val is None:
        return dbus.ObjectPath('/')
    if isinstance(val, DBusObject):
        return val._objectPath
    return val


class Variant:
    def __init__(self, signature, value):
        self.__signature = dbus.Signature(signature)
        self.__value = value

        if len(list(self.__signature)) != 1:
            raise Exception(
                'Signature is not a single complete type: ' + str(self.__signature))

        # TODO: convert self.__value here?
        if self.__signature == 'o':
            self.__value = convertObjectPath(self.__value)
        elif self.__signature == 'ao':
            self.__value = list(map(convertObjectPath, self.__value))

        verifyValue(signature, self.__value)

    @property
    def signature(self):
        return self.__signature

    @property
    def value(self):
        return self.__value

    def getValue(self, expectedSignature):
        if self.signature != expectedSignature:
            raise Exception("Expected signature '%s', got '%s'" %
                            (expectedSignature, self.signature))
        return self.value


def check_id(id):
    for c in id:
        if ord(c) >= 128:
            continue
        if c >= '0' and c <= '9':
            continue
        if c >= 'A' and c <= 'Z':
            continue
        if c >= 'a' and c <= 'z':
            continue
        if c == '_':
            continue
        raise Exception('Invalid character ' + c + ' in argument name ' + repr(id))

# Just setting __signature__ (PEP-0362) won't be picked up by Spyder


def fake_arglist(realfunc, name, args, defValues={}, kwonlyArgs=[], makeKWOnlyArgsNormal=False):
    # http://stackoverflow.com/questions/1409295/set-function-signature-in-python/1409496#1409496
    name = str(name)
    check_id(name)
    args_checked_f = []
    args_checked = []
    reserved_names = frozenset(['class'])
    for arg in args:
        s = str(arg)
        check_id(s)
        if s in reserved_names:
            s += '_'
        args_checked.append(s)
        if s in defValues:
            s = s + ' = ' + defValues[s]
        args_checked_f.append(s)
    if len(kwonlyArgs) != 0 and not makeKWOnlyArgsNormal:
        args_checked_f.append("*")
    for arg in kwonlyArgs:
        s = str(arg)
        check_id(s)
        args_checked.append(s + ' = ' + s)
        args_checked_f.append(s)
    args_checked.append("**kwargs")
    args_checked_f.append("**kwargs")

    argstr = ", ".join(args_checked)
    argstr_f = ", ".join(args_checked_f)
    fakefunc = "class DBusObjectFakeClass:\n    def %s(self, %s):\n        return real_func.__get__(self, None)(%s)\n" % (
        name, argstr_f, argstr)
    # print (fakefunc)
    fakefunc_code = compile(fakefunc, "fakesource", "exec")
    fakeglobals = {}
    eval(fakefunc_code, {"real_func": realfunc}, fakeglobals)
    return fakeglobals['DBusObjectFakeClass'].__dict__[name]


def get_variant_level(val):
    if type(val) == dbus.types.UnixFd and 'variant_level' not in dir(val):
        return 1  # Why does dbus.types.UnixFd not have a variant_level? Fixed in dbus-python 1.2.10
    return val.variant_level


def reduce_variant_level(val, amount):
    if type(val) == dbus.types.UnixFd and 'variant_level' not in dir(val):
        return val  # Why does dbus.types.UnixFd not have a variant_level? Fixed in dbus-python 1.2.10
    kwargs = {}
    if hasattr(val, 'signature'):
        kwargs['signature'] = val.signature
    if type(val) == dbus.types.UnixFd:
        # A UnixFd constructor does not accept another UnixFd as parameter
        # Note: Calling take() will invalidate this original val
        val2 = val.take()
        try:
            return type(val)(val2, variant_level=get_variant_level(val) - amount, **kwargs)
        finally:
            # Because the constructor calls dup(), the original FD has to be closed
            os.close(val2)
    else:
        return type(val)(val, variant_level=get_variant_level(val) - amount, **kwargs)


def get_variant_sig(val, *, addToLevel=0):
    if get_variant_level(val) + addToLevel < 0:
        raise Exception('get_variant_level(val) + addToLevel < 0')
    if get_variant_level(val) + addToLevel == 0:
        raise Exception('value is not a variant')
    if get_variant_level(val) + addToLevel > 1:
        return 'v'
    t = type(val)

    if t == dbus.Byte:
        return 'y'
    if t == dbus.Int16:
        return 'n'
    if t == dbus.UInt16:
        return 'q'
    if t == dbus.Int32:
        return 'i'
    if t == dbus.UInt32:
        return 'u'
    if t == dbus.Int64:
        return 'x'
    if t == dbus.UInt64:
        return 't'
    if t == dbus.Double:
        return 'd'
    if t == dbus.Boolean:
        return 'b'
    if t == dbus.String:
        return 's'
    if t == dbus.ObjectPath:
        return 'o'
    if t == dbus.Signature:
        return 'g'
    if t == dbus.types.UnixFd:
        return 'h'

    if t == dbus.Array:
        return 'a' + val.signature
    if t == dbus.Dictionary:
        return 'a{' + val.signature + '}'
    if t == dbus.Struct:
        if val.signature is not None:
            return '(' + val.signature + ')'
        else:
            s = '('
            for v in val:
                s += get_variant_sig(v, addToLevel=1)
            s += ')'
            return s

    raise Exception('Unknown type: ' + str(t))


def add_arg(f):
    return lambda value, callContext: f(value)

# TODO: Clean up, get dbusObject for variant from call 'self' parameter?
# TODO: Clean up in general, use more classes


def get_to_dbus_cast(sig, *, context, dbusObject, dbusObjectInfo, xmlElement, variant_level=0):
    if context is not None and 'getConverterToDBus' in dir(context):
        retVal = context.getConverterToDBus(
            dbusType=sig, xmlElement=xmlElement, dbusObject=dbusObject, dbusObjectInfo=dbusObjectInfo, variantLevel=variant_level)
        if retVal is not None:
            return retVal

    if sig == 'y':
        return lambda value, callContext: dbus.Byte(value, variant_level=variant_level)
    if sig == 'n':
        return lambda value, callContext: dbus.Int16(value, variant_level=variant_level)
    if sig == 'q':
        return lambda value, callContext: dbus.UInt16(value, variant_level=variant_level)
    if sig == 'i':
        return lambda value, callContext: dbus.Int32(value, variant_level=variant_level)
    if sig == 'u':
        return lambda value, callContext: dbus.UInt32(value, variant_level=variant_level)
    if sig == 'x':
        return lambda value, callContext: dbus.Int64(value, variant_level=variant_level)
    if sig == 't':
        return lambda value, callContext: dbus.UInt64(value, variant_level=variant_level)
    if sig == 'd':
        return lambda value, callContext: dbus.Double(value, variant_level=variant_level)
    if sig == 'b':
        return lambda value, callContext: dbus.Boolean(value, variant_level=variant_level)
    if sig == 's':
        return lambda value, callContext: dbus.String(value, variant_level=variant_level)
    if sig == 'o':
        return lambda value, callContext: dbus.ObjectPath(value, variant_level=variant_level)
    if sig == 'g':
        return lambda value, callContext: dbus.Signature(value, variant_level=variant_level)

    # A UnixFd parameter is expected to already be a dbus.types.UnixFd object
    if sig == 'h':
        def convert(value, callContext):
            # TODO: should verify that variant_level is 0
            if type(value) != dbus.types.UnixFd:
                raise Exception(
                    'Expected a dbus.types.UnixFd, got a %s', (type(value),))
            return value
        return convert

    if sig == 'v':
        def convertVariant(value, callContext):
            ty = type(value)
            if ty != Variant:
                raise Exception('Expected a %s, got a %s' % (Variant, ty))
            val = value.value
            ty = type(val)
            # TODO: Should xmlElement be set to None here? (Will prevent annotations from having an effect on values passed as variants) # Set dbusObject to None here to prevent object cycles which break deterministic cleanup
            cast = get_to_dbus_cast(value.signature, context=context, dbusObject=None,
                                    dbusObjectInfo=dbusObjectInfo, xmlElement=None, variant_level=variant_level + 1)
            dval = cast(val, callContext=callContext)
            return dval
            # return ty (dval, variant_level = get_variant_level(dval) + variant_level)
        return convertVariant

    if sig[0] == '(':
        if sig[-1] != ')':
            raise Exception("sig[-1] != ')'")
        casts = []
        for elem in dbus.Signature(sig[1:-1]):
            casts.append(get_to_dbus_cast(elem, context=context, dbusObject=dbusObject,
                                          dbusObjectInfo=dbusObjectInfo, xmlElement=xmlElement, variant_level=0))

        def fun(value, callContext):
            lval = len(value)
            if lval != len(casts):
                raise Exception("Invalid number of values for '%s' argument, expected %d, got %d" % (
                    sig, len(casts), lval))
            res = []
            for i in range(len(casts)):
                res.append(casts[i](value[i], callContext=callContext))
            return dbus.Struct(res, signature=sig[1:-1], variant_level=variant_level)
        return fun

    if sig[0:2] == 'a{':
        if sig[-1] != '}':
            raise Exception("sig[-1] != ')'")
        t = list(dbus.Signature(sig[2:-1]))
        if len(t) != 2:
            raise Exception('len (t) != 2')
        castn = get_to_dbus_cast(t[0], context=context, dbusObject=dbusObject,
                                 dbusObjectInfo=dbusObjectInfo, xmlElement=xmlElement)
        castv = get_to_dbus_cast(t[1], context=context, dbusObject=dbusObject,
                                 dbusObjectInfo=dbusObjectInfo, xmlElement=xmlElement)

        def fun(value, callContext):
            res = {}
            for name in value:
                res[castn(name, callContext=callContext)] = castv(
                    value[name], callContext=callContext)
            return dbus.Dictionary(res, signature=sig[2:-1], variant_level=variant_level)
        return fun

    if sig[0] == 'a':
        cast = get_to_dbus_cast(sig[1:], context=context, dbusObject=dbusObject,
                                dbusObjectInfo=dbusObjectInfo, xmlElement=xmlElement)

        def fun(value, callContext):
            res = []
            for i in value:
                res.append(cast(i, callContext=callContext))
            return dbus.Array(res, signature=sig[1:], variant_level=variant_level)
        return fun

    raise Exception('Unknown signature: ' + sig)


def get_from_dbus_cast(sig, *, context, dbusObject, dbusObjectInfo, xmlElement, byte_arrays=None, ignore_variant_levels=0):
    # TODO: Clean this up? Should the caller of the converter take care of this?
    if ignore_variant_levels != 0:
        converter = get_from_dbus_cast(sig, context=context, dbusObject=dbusObject, dbusObjectInfo=dbusObjectInfo, xmlElement=xmlElement, byte_arrays=byte_arrays, ignore_variant_levels=0)

        def convertIgnoreVariantLevels(value, callContext):
            return converter(reduce_variant_level(value, ignore_variant_levels), callContext)
        return convertIgnoreVariantLevels

    if context is not None and 'getConverterFromDBus' in dir(context):
        retVal = context.getConverterFromDBus(
            dbusType=sig, xmlElement=xmlElement, dbusObject=dbusObject, dbusObjectInfo=dbusObjectInfo)
        if retVal is not None:
            return retVal

    if sig == 'ay' and byte_arrays:
        return add_arg(bytes)

    if sig == 'y':
        return add_arg(int)
    if sig == 'n':
        return add_arg(int)
    if sig == 'q':
        return add_arg(int)
    if sig == 'i':
        return add_arg(int)
    if sig == 'u':
        return add_arg(int)
    if sig == 'x':
        return add_arg(int)
    if sig == 't':
        return add_arg(int)
    if sig == 'd':
        return add_arg(float)
    if sig == 'b':
        return add_arg(bool)
    if sig == 's':
        return add_arg(str)
    if sig == 'o':
        return add_arg(dbus.ObjectPath)
    if sig == 'g':
        return add_arg(dbus.Signature)
    if sig == 'h':
        # return add_arg(dbus.types.UnixFd) # Does not work, cannot pass a UnixFd to dbus.types.UnixFd
        return lambda value, callContext: value

    if sig == 'v':
        def convertVariant(value, callContext):
            sig = get_variant_sig(value)
            # TODO: Should xmlElement be set to None here? (Will prevent annotations from having an effect on values passed as variants) # Set dbusObject to None here to prevent object cycles which break deterministic cleanup
            cast = get_from_dbus_cast(
                sig, context=context, dbusObject=None, dbusObjectInfo=dbusObjectInfo, xmlElement=None, ignore_variant_levels=1)
            dval = cast(value, callContext=callContext)
            return Variant(sig, dval)
        return convertVariant

    if sig[0] == '(':
        if sig[-1] != ')':
            raise Exception("sig[-1] != ')'")
        casts = []
        for elem in dbus.Signature(sig[1:-1]):
            casts.append(get_from_dbus_cast(elem, context=context, dbusObject=dbusObject,
                                            dbusObjectInfo=dbusObjectInfo, xmlElement=xmlElement, byte_arrays=byte_arrays))

        def fun(value, callContext):
            lval = len(value)
            if lval != len(casts):
                raise Exception("Invalid number of values for '%s' argument, expected %d, got %d" % (
                    sig, len(casts), lval))
            res = []
            for i in range(len(casts)):
                res.append(casts[i](value[i], callContext=callContext))
            return tuple(res)
        return fun

    if sig[0:2] == 'a{':
        if sig[-1] != '}':
            raise Exception("sig[-1] != ')'")
        t = list(dbus.Signature(sig[2:-1]))
        if len(t) != 2:
            raise Exception('len (t) != 2')
        castn = get_from_dbus_cast(t[0], context=context, dbusObject=dbusObject,
                                   dbusObjectInfo=dbusObjectInfo, xmlElement=xmlElement, byte_arrays=byte_arrays)
        castv = get_from_dbus_cast(t[1], context=context, dbusObject=dbusObject,
                                   dbusObjectInfo=dbusObjectInfo, xmlElement=xmlElement, byte_arrays=byte_arrays)

        def fun(value, callContext):
            res = {}
            for name in value:
                # print ('castv', t[1])
                res[castn(name, callContext=callContext)] = castv(
                    value[name], callContext=callContext)
            return res
        return fun

    if sig == 'a{sv}':
        return lambda value, callContext: dict(list(map(lambda val: (str(val), value[val]), value)))
    if sig == 'a{ss}':
        return lambda value, callContext: dict(list(map(lambda val: (str(val), str(value[val])), value)))

    if sig[0] == 'a':
        cast = get_from_dbus_cast(sig[1:], context=context, dbusObject=dbusObject,
                                  dbusObjectInfo=dbusObjectInfo, xmlElement=xmlElement, byte_arrays=byte_arrays)

        def fun(value, callContext):
            res = []
            for i in value:
                res.append(cast(i, callContext=callContext))
            return res
        return fun

    raise Exception('Unknown signature: ' + sig)


class DBusObjectContext(object):
    def __init__(self, interfaces):
        self.handleMessagesDefault = False
        self.interfaces = {}
        for interface in interfaces:
            if interface.tag == 'interface':
                name = interface.attrib['name']
                # print(name)
                self.interfaces[name] = interface


class DBusCallContext(object):
    def success(self):
        pass

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        return False


class DBusServiceCallContext(object):
    def __init__(self, info):
        self.info = info

    def success(self):
        pass

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        return False


class DBusServiceCallInfo(object):
    pass


class DBusObject(object):
    def __init__(self, obj, interfaces, context=None, referenceCountingObject=None):
        if type(interfaces) != list:
            raise Exception('interfaces is not a list but a %s' %
                            (type(interfaces),))
        self.__busObject = obj
        self.__bus = obj._bus
        self.__busName = str(obj.bus_name) if obj.bus_name is not None else ''
        # Make sure self._objectPath has a variant level of 0
        self.__objectPath = dbus.ObjectPath(str(obj.object_path))
        self.__interfaces = interfaces
        self.__propObj = dbus.Interface(obj, 'org.freedesktop.DBus.Properties')
        self.__propget = {}
        self.__methods = {}
        self.__propset = {}
        self.__context = context
        self.__referenceCountingObject = referenceCountingObject
        self.__dbusObjectInfo = {'bus': self.__bus, 'busName': self.__busName,
                                 'objectPath': self.__objectPath, 'interfaces': self.__interfaces}
        self.__names = []
        self.__names = list(object.__dir__(self))
        introspectionResult = None
        introspectionResultDoc = None
        for interfaceName in interfaces:
            if self.__context is None:
                if introspectionResultDoc is None:
                    introspectable = dbus.Interface(
                        obj, 'org.freedesktop.DBus.Introspectable')
                    introspectionResult = introspectable.Introspect()
                    introspectionResultDoc = xml.etree.ElementTree.fromstring(
                        introspectionResult)
                interface = None
                for child in introspectionResultDoc:
                    if child.tag == 'interface' and child.attrib['name'] == interfaceName:
                        # print (child)
                        interface = child
                if interface is None:
                    raise Exception('Could not find interface ' +
                                    interfaceName + ' in reflection data')
            else:
                interface = self.__context.interfaces[interfaceName]
            # print (interface)
            for child in interface:
                if child.tag == 'method':
                    name = child.attrib['name']
                    if name in self.__methods:
                        continue
                    if name in self.__names:
                        addToPropget = False
                    else:
                        addToPropget = True
                        self.__names += [name]

                    cnt = 0
                    icnt = 0
                    rsig = None
                    rXmlElement = None
                    parnames = []
                    types = []
                    argXmlElements = []
                    typesd = {}
                    argXmlElementsD = {}
                    for arg in child:
                        if arg.tag != 'arg':
                            continue
                        if arg.attrib['direction'] != 'out':
                            if 'name' in arg.attrib:
                                paramName = arg.attrib['name']
                            else:
                                paramName = 'arg%d' % icnt
                            parnames.append(paramName)
                            types.append(arg.attrib['type'])
                            argXmlElements.append(arg)
                            typesd[paramName] = arg.attrib['type']
                            argXmlElementsD[paramName] = arg
                            icnt = icnt + 1
                            continue
                        cnt += 1
                        if cnt != 1:
                            rsig = None
                            rXmlElement = None
                        else:
                            rsig = arg.attrib['type']
                            rXmlElement = arg
                    defValues = {}
                    defValuesVal = {}
                    if self.__context is not None and 'defaultParameters' in dir(self.__context):
                        defPars = self.__context.defaultParameters(
                            parameterNames=parnames, parameterTypes=types)
                        for pname in defPars:
                            defValues[pname] = 'None'
                            defValuesVal[pname] = defPars[pname]
                    implicitParameters = {}
                    if self.__context is not None and 'implicitParameters' in dir(self.__context):
                        implicitParameters = self.__context.implicitParameters(
                            parameterNames=parnames, parameterTypes=types)
                    implicitParameterData = []
                    for i in range(len(parnames)):
                        if parnames[i] in implicitParameters:
                            implicitParameterData.append(
                                (i, implicitParameters[parnames[i]]))
                    for dat in reversed(implicitParameterData):
                        i = dat[0]
                        del parnames[i]
                        del types[i]
                        del argXmlElements[i]
                        icnt = icnt - 1

                    def cast(value, callContext):
                        return value

                    def castb(value, callContext):
                        return value
                    # print (name, rsig)
                    if rsig is not None:
                        cast = get_from_dbus_cast(rsig, context=self.__context, dbusObject=self,
                                                  dbusObjectInfo=self.__dbusObjectInfo, xmlElement=rXmlElement, byte_arrays=False)
                        castb = get_from_dbus_cast(rsig, context=self.__context, dbusObject=self,
                                                   dbusObjectInfo=self.__dbusObjectInfo, xmlElement=rXmlElement, byte_arrays=True)

                    inCast = []
                    inCastD = {}
                    # for t in types:
                    for i in range(len(types)):
                        t = types[i]
                        inCast.append(get_to_dbus_cast(t, context=self.__context, dbusObject=self,
                                                       dbusObjectInfo=self.__dbusObjectInfo, xmlElement=argXmlElements[i], variant_level=0))
                    for nm in typesd:
                        inCastD[nm] = get_to_dbus_cast(typesd[nm], context=self.__context, dbusObject=self,
                                                       dbusObjectInfo=self.__dbusObjectInfo, xmlElement=argXmlElementsD[nm], variant_level=0)

                    method = getattr(dbus.Interface(
                        self.__busObject, interfaceName), name)

                    # Note: self.__context (and anything else using self) must not be used inside the closure to avoid circular references which would prevent deterministic cleanup
                    def make_closure(method=method, cast=cast, castb=castb, types=types, typesd=typesd, inCast=inCast, inCastD=inCastD, parnames=parnames, defValuesVal=defValuesVal, context=self.__context, implicitParameterData=implicitParameterData, methodXml=child):
                        def dbusFunctionWrapper(self, *args, **kwargs):
                            # print(self)
                            # print ('Called %s with %s %s' % (method, args, kwargs))
                            args = list(args)
                            kwargs = dict(kwargs)
                            handleMessages = False
                            timeout = None
                            if context is not None:
                                context.handleMessagesDefault
                            if 'DBusObject_timeout' in kwargs:
                                if kwargs['DBusObject_timeout'] is not None:
                                    timeout = float(
                                        kwargs['DBusObject_timeout'])
                                del kwargs['DBusObject_timeout']
                            if 'DBusObject_handleMessages' in kwargs:
                                handleMessages = bool(
                                    kwargs['DBusObject_handleMessages'])
                                del kwargs['DBusObject_handleMessages']
                            if handleMessages and not getattr(context, 'iteration', False):
                                raise Exception(
                                    'DBusObject_handleMessages is True but context object has no "iteration" member')
                            with (context.createCallContext(dbusObject=self, xmlElement=methodXml) if context is not None and 'createCallContext' in dir(context) else DBusCallContext()) as callContext:
                                for i in range(len(args)):
                                    val = args[i]
                                    if val is None and parnames[i] in defValuesVal:
                                        val = defValuesVal[parnames[i]]
                                    args[i] = inCast[i](
                                        val, callContext=callContext)
                                while len(args) < len(parnames):
                                    i = len(args)
                                    name = parnames[i]
                                    val = kwargs[name]
                                    if val is None and name in defValuesVal:
                                        val = defValuesVal[name]
                                    val = inCastD[name](
                                        val, callContext=callContext)
                                    del kwargs[name]
                                    args.append(val)
                                if len(kwargs) != 0:
                                    raise Exception(
                                        'Got leftover keyword arguments: %s' % (repr(kwargs),))
                                for dat in implicitParameterData:
                                    args.insert(dat[0], dat[1])
                                if timeout is not None:
                                    kwargs['timeout'] = timeout
                                # print ('Calling %s with %s %s' % (method, args, kwargs))
                                if not handleMessages:
                                    res0 = method(*args, **kwargs)
                                else:
                                    retVal = []
                                    errorVal = []

                                    def lazyDataReply(data=None):
                                        retVal.append(data)

                                    def lazyDataError(error):
                                        errorVal.append(error)
                                    method(
                                        *args, **kwargs, reply_handler=lazyDataReply, error_handler=lazyDataError)
                                    while len(retVal) == 0 and len(errorVal) == 0:
                                        context.iteration()
                                    if len(errorVal) != 0:
                                        raise errorVal[0]
                                    res0 = retVal[0]
                                res = (castb if kwargs.get('byte_arrays') else cast)(
                                    res0, callContext=callContext)
                                callContext.success()
                                return res
                        return dbusFunctionWrapper
                    # func = make_closure ()
                    func = fake_arglist(
                        make_closure(), name, parnames, defValues)
                    if addToPropget:
                        # For tab completion
                        object.__setattr__(self, name, None)
                        self.__propget[name] = (
                            lambda func=func: lambda newSelf: func.__get__(newSelf, None))()
                    self.__methods[name] = (
                        lambda func=func: lambda newSelf: func.__get__(newSelf, None))()
                elif child.tag == 'property':
                    # TODO: use context.handleMessagesDefault for properties
                    name = child.attrib['name']
                    if name in self.__names:
                        continue
                    self.__names += [name]
                    # print (name)
                    object.__setattr__(self, name, None)  # For tab completion
                    # TODO: Check signature of variant?
                    cast = get_from_dbus_cast(
                        child.attrib['type'], context=self.__context, dbusObject=self, dbusObjectInfo=self.__dbusObjectInfo, xmlElement=child, ignore_variant_levels=1)

                    # Note: self.__context and self.__propObj (and anything else using self) must not be used inside the closure to avoid circular references which would prevent deterministic cleanup
                    def make_closure(self, interfaceName, name, cast, context=self.__context, propObj=self.__propObj):
                        def getter(newSelf):
                            with (context.createCallContext(dbusObject=newSelf, xmlElement=None) if context is not None and 'createCallContext' in dir(context) else DBusCallContext()) as callContext:
                                res = cast(propObj.Get(
                                    interfaceName, name), callContext=callContext)
                                callContext.success()
                                return res
                        return getter
                    self.__propget[name] = make_closure(
                        self, interfaceName, name, cast)
                    cast = get_to_dbus_cast(child.attrib['type'], context=self.__context, dbusObject=self,
                                            dbusObjectInfo=self.__dbusObjectInfo, xmlElement=child, variant_level=1)

                    # Note: self.__context and self.__propObj (and anything else using self) must not be used inside the closure to avoid circular references which would prevent deterministic cleanup
                    def make_closure(self, interfaceName, name, cast, context=self.__context, propObj=self.__propObj):
                        def setter(newSelf, value):
                            with (context.createCallContext(dbusObject=newSelf, xmlElement=None) if context is not None and 'createCallContext' in dir(context) else DBusCallContext()) as callContext:
                                propObj.Set(interfaceName, name, cast(
                                    value, callContext=callContext))
                                callContext.success()
                        return setter
                    self.__propset[name] = make_closure(
                        self, interfaceName, name, cast)
                elif child.tag == 'signal':
                    name = child.attrib['name']
                    if name in self.__names:
                        continue
                    self.__names += [name]
                    # print (name)
                    object.__setattr__(self, name, None)  # For tab completion
                    # cast = get_from_dbus_cast (child.attrib['type'], context = self.__context, dbusObject = self, dbusObjectInfo = self.__dbusObjectInfo, xmlElement = )

                    # Note: self.__context and self.__propObj (and anything else using self) must not be used inside the closure to avoid circular references which would prevent deterministic cleanup
                    def make_closure(self, interfaceName, name, context=self.__context, interfaceObj=dbus.Interface(self.__busObject, interfaceName)):
                        return lambda newSelf: lambda handler: interfaceObj.connect_to_signal(name, handler, dbus_interface=interfaceName)
                    self.__propget[name] = make_closure(
                        self, interfaceName, name)

    def __dir__(self):
        return self.__names

    def __getattribute__(self, name):
        if not name.startswith('_') and name in self.__propget:
            return self.__propget[name](self)
        return object.__getattribute__(self, name)

    def __setattr__(self, name, value):
        if not name.startswith('_'):
            self.__propset[name](self, value)
            return
        object.__setattr__(self, name, value)

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        if self.__referenceCountingObject is not None:
            self.__referenceCountingObject.destroy()
        return False

    def __repr__(self):
        contextStr = ''
        if self.__context is not None:
            contextStr = ', context = ' + repr(self.__context)
        referenceCountingObjectStr = ''
        if self.__referenceCountingObject is not None:
            referenceCountingObjectStr = ', referenceCountingObject = ' + \
                repr(self.__referenceCountingObject)
        return 'DBusObject((%s, %s, %s), %s%s%s)' % (repr(self.__bus), repr(self.__busName), repr(str(self.__objectPath)), repr(self.__interfaces), contextStr, referenceCountingObjectStr)

    @property
    def _context(self):
        return self.__context

    @property
    def _connection(self):
        return self.__bus

    @property
    def _busName(self):
        return self.__busName

    @property
    def _interfaces(self):
        return self.__interfaces

    @property
    def _objectPath(self):
        return self.__objectPath

    @property
    def _referenceCountingObject(self):
        return self.__referenceCountingObject

    @_referenceCountingObject.setter
    def _referenceCountingObject(self, value):
        self.__referenceCountingObject = value

    def _getDBusMethod(self, name):
        return self.__methods[name](self)

    def _clone(self):
        oldRefObj = self._referenceCountingObject
        if self._referenceCountingObject is None:
            newRefObj = None
        else:
            newRefObj = self._referenceCountingObject.clone()
        context = self._context
        if hasattr(context, 'makeObject'):
            return context.makeObject(self._connection, self._busName, self._objectPath, self._interfaces, referenceCountingObject=newRefObj)
        else:
            return DBusObject(self._connection, self._busName, self._objectPath, self._interfaces, referenceCountingObject=newRefObj)


class DBusExportObject(dbus.service.Object):
    def __init__(self, interfaces, *, context):
        dbus.service.Object.__init__(self)
        if type(interfaces) != list:
            raise Exception('interfaces is not a list but a %s' %
                            (type(interfaces),))
        interfaces = list(interfaces)
        if 'org.freedesktop.DBus.Properties' not in interfaces:
            interfaces.append('org.freedesktop.DBus.Properties')
        if 'org.freedesktop.DBus.Introspectable' not in interfaces:
            interfaces.append('org.freedesktop.DBus.Introspectable')
        self.__interfaces = interfaces
        self.__context = context
        self.__dbusMethods = []
        self.__propgetimpl = {}
        self.__propsetimpl = {}
        self.__proptype = {}
        names = set()
        impls = dir(self)
        newClassDict = {}
        introspectionResult = None
        introspectionResultDoc = None
        for interfaceName in interfaces:
            interface = self.__context.interfaces[interfaceName]
            # print (interface)
            for child in interface:
                if child.tag == 'method':
                    name = child.attrib['name']
                    if name in names:
                        print('Warning: DBusExportObject: Ignoring hidden method %s.%s' % (
                            interfaceName, name), file=sys.stderr)
                        continue
                    names.add(name)

                    if name not in impls:
                        print('Warning: DBusExportObject: Missing implementation for method %s.%s' % (
                            interfaceName, name), file=sys.stderr)
                        continue

                    cnt = 0
                    icnt = 0
                    rsig = None
                    rXmlElement = None
                    parnames = []
                    types = []
                    argXmlElements = []
                    typesd = {}
                    argXmlElementsD = {}
                    inSig = ''
                    outSig = ''
                    for arg in child:
                        if arg.tag != 'arg':
                            continue
                        if arg.attrib['direction'] != 'out':
                            inSig += arg.attrib['type']
                            if 'name' in arg.attrib:
                                paramName = arg.attrib['name']
                            else:
                                paramName = 'arg%d' % icnt
                            parnames.append(paramName)
                            types.append(arg.attrib['type'])
                            argXmlElements.append(arg)
                            typesd[paramName] = arg.attrib['type']
                            argXmlElementsD[paramName] = arg
                            icnt = icnt + 1
                            continue
                        outSig += arg.attrib['type']
                        cnt += 1
                        if cnt != 1:
                            rsig = None
                            rXmlElement = None
                        else:
                            rsig = arg.attrib['type']
                            rXmlElement = arg
                    defValues = {}
                    defValuesVal = {}

                    def cast(value, callContext):
                        return value
                    # print (name, rsig)
                    if rsig is not None:
                        cast = get_to_dbus_cast(rsig, context=self.__context, dbusObject=None,
                                                dbusObjectInfo=None, xmlElement=rXmlElement, variant_level=0)

                    inCast = []
                    inCastD = {}
                    # for t in types:
                    for i in range(len(types)):
                        t = types[i]
                        inCast.append(get_from_dbus_cast(t, context=self.__context, dbusObject=None,
                                                         dbusObjectInfo=None, xmlElement=argXmlElements[i], byte_arrays=False))
                    for nm in typesd:
                        inCastD[nm] = get_from_dbus_cast(
                            typesd[nm], context=self.__context, dbusObject=None, dbusObjectInfo=None, xmlElement=argXmlElementsD[nm], byte_arrays=False)

                    method = getattr(type(self), name)
                    if isinstance(method, property):
                        raise Exception('Method %s is a property' % (name,))
                    # print(method)
                    methodSig = inspect.signature(method)
                    methodKwOnlyArguments = set()
                    for arg in methodSig.parameters.values():
                        if arg.kind == inspect.Parameter.KEYWORD_ONLY:
                            methodKwOnlyArguments.add(arg.name)
                    # print (method, methodSig, methodKwOnlyArguments)
                    addInfoArg = 'dbusServiceCallInfo' in methodKwOnlyArguments

                    def make_closure(method=method, cast=cast, types=types, typesd=typesd, inCast=inCast, inCastD=inCastD, parnames=parnames, defValuesVal=defValuesVal, context=self.__context, methodXml=child, addInfoArg=addInfoArg):
                        def dbusFunctionWrapper(self, *args, _DBusExportObject_info_sender, _DBusExportObject_info_path, _DBusExportObject_info_destination, _DBusExportObject_info_message, _DBusExportObject_info_connection, _DBusExportObject_info_rel_path, **kwargs):
                            # print ('Called %s on %s with %s %s' % (method, self, args, kwargs))
                            info = DBusServiceCallInfo()
                            info.sender = _DBusExportObject_info_sender
                            info.object_path = _DBusExportObject_info_path
                            info.destination = _DBusExportObject_info_destination
                            info.message = _DBusExportObject_info_message
                            info.connection = _DBusExportObject_info_connection
                            info.rel_path = _DBusExportObject_info_rel_path
                            args = list(args)
                            kwargs = dict(kwargs)
                            with (context.createServiceCallContext(dbusObject=self, xmlElement=methodXml, info=info) if context is not None and 'createServiceCallContext' in dir(context) else DBusServiceCallContext(info=info)) as callContext:
                                for i in range(len(args)):
                                    val = args[i]
                                    if val is None and parnames[i] in defValuesVal:
                                        val = defValuesVal[parnames[i]]
                                    args[i] = inCast[i](
                                        val, callContext=callContext)
                                while len(args) < len(parnames):
                                    i = len(args)
                                    name = parnames[i]
                                    val = kwargs[name]
                                    if val is None and name in defValuesVal:
                                        val = defValuesVal[name]
                                    val = inCastD[name](
                                        val, callContext=callContext)
                                    del kwargs[name]
                                    args.append(val)
                                if len(kwargs) != 0:
                                    raise Exception(
                                        'Got leftover keyword arguments: %s' % (repr(kwargs),))
                                if addInfoArg:
                                    kwargs['dbusServiceCallInfo'] = info
                                # print ('Calling %s with %s %s' % (method, args, kwargs))
                                res = cast(method(self, *args, **kwargs),
                                           callContext=callContext)
                                callContext.success()
                                return res
                        return dbusFunctionWrapper
                    func = make_closure()
                    func.__wrapped__ = method
                    kwonlyArgs = [
                        '_DBusExportObject_info_sender',
                        '_DBusExportObject_info_path',
                        '_DBusExportObject_info_destination',
                        '_DBusExportObject_info_message',
                        '_DBusExportObject_info_connection',
                        '_DBusExportObject_info_rel_path',
                    ]
                    # Should work without this in newer dbus-python versions
                    func = fake_arglist(
                        func, name, parnames, kwonlyArgs=kwonlyArgs, makeKWOnlyArgsNormal=True)
                    # func = method
                    # print (inSig, outSig, func, inspect.signature(func))
                    dbusMethod = dbus.service.method(dbus_interface=interfaceName, in_signature=inSig, out_signature=outSig, sender_keyword='_DBusExportObject_info_sender', path_keyword='_DBusExportObject_info_path',
                                                     destination_keyword='_DBusExportObject_info_destination', message_keyword='_DBusExportObject_info_message', connection_keyword='_DBusExportObject_info_connection', rel_path_keyword='_DBusExportObject_info_rel_path')(func)
                    self.__dbusMethods.append(dbusMethod)
                    # setattr(self, name, dbusMethod)
                    newClassDict[name] = dbusMethod
                elif child.tag == 'property':
                    name = child.attrib['name']
                    if name in names:
                        print('Warning: DBusExportObject: Ignoring hidden property %s.%s' % (
                            interfaceName, name), file=sys.stderr)
                        continue
                    names.add(name)

                    if name not in impls:
                        print('Warning: DBusExportObject: Missing implementation for method %s.%s' % (
                            interfaceName, name), file=sys.stderr)
                        continue

                    access = child.attrib['access']
                    if access not in ['readwrite', 'read', 'write']:
                        raise Exception(
                            'Invalid "access" value: ' + repr(access))

                    prop = getattr(type(self), name)
                    if not isinstance(prop, property):
                        raise Exception(
                            'Property %s is not a property' % (name,))

                    # print (name)
                    self.__proptype[interfaceName +
                                    '.' + name] = child.attrib['type']
                    if access in ['read', 'readwrite']:
                        if prop.fget is None:
                            raise Exception(
                                'Property %s is not readable' % name)
                        cast = get_to_dbus_cast(child.attrib['type'], context=self.__context, dbusObject=None,
                                                dbusObjectInfo=None, xmlElement=child, variant_level=0)  # TODO: variant_level?

                        def make_closure(self, interfaceName, name, cast, context=self.__context, prop=prop):
                            def getter(newSelf, info):
                                with (context.createServiceCallContext(dbusObject=newSelf, xmlElement=None, info=info) if context is not None and 'createServiceCallContext' in dir(context) else DBusServiceCallContext(info=info)) as callContext:
                                    # TODO: remove cast because Get() already does the cast. What should happen to annotations, how should they be forwarded to Get()?
                                    # res = cast (prop.fget(newSelf), callContext = callContext)
                                    res = prop.fget(newSelf)
                                    callContext.success()
                                    return res
                            return getter
                        self.__propgetimpl[interfaceName + '.' +
                                           name] = make_closure(self, interfaceName, name, cast)
                    if access in ['write', 'readwrite']:
                        if prop.fset is None:
                            raise Exception(
                                'Property %s is not writable' % name)
                        # TODO: Check signature of variant?
                        cast = get_from_dbus_cast(
                            child.attrib['type'], context=self.__context, dbusObject=None, dbusObjectInfo=None, xmlElement=child, ignore_variant_levels=1)

                        def make_closure(self, interfaceName, name, cast, context=self.__context, prop=prop):
                            def setter(newSelf, value, info):
                                with (context.createServiceCallContext(dbusObject=newSelf, xmlElement=None, info=info) if context is not None and 'createServiceCallContext' in dir(context) else DBusServiceCallContext(info=info)) as callContext:
                                    prop.fset(newSelf, cast(
                                        value, callContext=callContext))
                                    callContext.success()
                            return setter
                        self.__propsetimpl[interfaceName + '.' +
                                           name] = make_closure(self, interfaceName, name, cast)
                elif child.tag == 'signal':
                    name = child.attrib['name']
                    if name in names:
                        print('Warning: DBusExportObject: Ignoring hidden signal %s.%s' % (
                            interfaceName, name), file=sys.stderr)
                        continue
                    names.add(name)

                    raise Exception('TODO: not implemented')
                    # print (name)
                    object.__setattr__(self, name, None)  # For tab completion
                    # cast = get_from_dbus_cast (child.attrib['type'], context = self.__context, dbusObject = None, dbusObjectInfo = None, xmlElement = )

                    # Note: self.__context and self.__propObj (and anything else using self) must not be used inside the closure to avoid circular references which would prevent deterministic cleanup
                    def make_closure(self, interfaceName, name, context=self.__context, interfaceObj=dbus.Interface(self.__busObject, interfaceName)):
                        return lambda newSelf: lambda handler: interfaceObj.connect_to_signal(name, handler, dbus_interface=interfaceName)
                    self.__propget[name] = make_closure(
                        self, interfaceName, name)
        newClass = type('DBusExportObjectClass_' +
                        self.__class__.__name__, (self.__class__,), newClassDict)
        # print(self.__class__, newClass)
        self.__class__ = newClass

    def Introspect(self, *, dbusServiceCallInfo):
        data = '<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"\n"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd">\n'
        data += '<node name="%s">\n' % dbusServiceCallInfo.object_path

        for interfaceName in self.__interfaces:
            data += '  '
            interfaceData = io.StringIO()
            xml.etree.ElementTree.ElementTree(self.__context.interfaces[interfaceName]).write(
                interfaceData, encoding='unicode')
            data += interfaceData.getvalue().strip()
            data += '\n'

        for name in dbusServiceCallInfo.connection.list_exported_child_objects(dbusServiceCallInfo.object_path):
            data += '  <node name="%s"/>\n' % name

        data += '</node>\n'

        return data

    def Get(self, interface_name, property_name, *, dbusServiceCallInfo):
        if interface_name == "":
            raise Exception(
                'Getting property values without interface name is not supported')
        name = interface_name + '.' + property_name
        if '.' in property_name:
            raise Exception(
                'Getting property values without interface name is not supported')
        if name not in self.__propgetimpl:
            if name in self.__propsetimpl:
                raise Exception(
                    'Property %s in interface %s is a write-only property' % (interface_name, property_name))
            raise Exception('Property %s in interface %s not found' %
                            (interface_name, property_name))
        return Variant(self.__proptype[name], self.__propgetimpl[name](self, info=dbusServiceCallInfo))

    def GetAll(self, interface_name, *, dbusServiceCallInfo):
        if interface_name == "":
            raise Exception(
                'Getting property values without interface name is not supported')
        result = {}
        prefix = interface_name + '.'
        for name in self.__propgetimpl:
            if not name.startswith(prefix):
                continue
            pname = name[len(prefix):]
            result[pname] = Variant(self.__proptype[name], self.__propgetimpl[name](
                self, info=dbusServiceCallInfo))
        return result

    def Set(self, interface_name, property_name, value, *, dbusServiceCallInfo):
        if interface_name == "":
            raise Exception(
                'Getting property values without interface name is not supported')
        name = interface_name + '.' + property_name
        if '.' in property_name:
            raise Exception(
                'Getting property values without interface name is not supported')
        if name not in self.__propsetimpl:
            if name in self.__propgetimpl:
                raise Exception(
                    'Property %s in interface %s is a read-only property' % (interface_name, property_name))
            raise Exception('Property %s in interface %s not found' %
                            (interface_name, property_name))
        # TODO: from_dbus converter will be called twice here?
        self.__propsetimpl[name](self, value, info=dbusServiceCallInfo)
