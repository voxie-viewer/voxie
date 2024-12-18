import os
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
import mmap
import voxie
import voxie.json_dbus
import voxie.json_util

typeNameMap = {
    "float": "f",
    "uint": "u",
    "int": "i",
    "bool": "b",
}
endianMap = {
    "none": "|",
    "little": "<",
    "big": ">",
    # TODO: ?
    "native": "",
}

# TODO: Remove when all scripts are switched to new interface


def getValue(obj, sig):
    if type(obj) == voxie.Variant:
        return obj.getValue(sig)
    if sig == 's':
        return str(obj)
    if sig == 'u':
        return int(obj)
    return obj


class BufferTypeBase:
    pass


class BufferTypePrimitive(BufferTypeBase):
    def __init__(self, typeName, typeSize, typeEndian):
        self.typeName = typeName
        self.typeSize = typeSize
        self.typeEndian = typeEndian

        if self.typeSize < 0 or int(self.typeSize) % 8 != 0:
            raise Exception('Invalid size')
        self.dtype = numpy.dtype(endianMap[self.typeEndian] + typeNameMap[self.typeName] + str(int(self.typeSize) // 8))
        # print (self.dtype)

        self.start_byte = 0
        self.end_byte = self.dtype.itemsize

        self.has_struct = False

    def get_dtype(self):
        return self.dtype


class BufferTypeArray(BufferTypeBase):
    def __init__(self, shape, strides_bytes, element_type):
        self.shape = shape
        self.strides_bytes = strides_bytes
        self.element_type = element_type

        if len(self.shape) != len(self.strides_bytes):
            raise Exception('len(self.shape) != len(self.strides_bytes)')
        self.dim = len(self.shape)

        self.start_byte = self.element_type.start_byte
        self.end_byte = self.element_type.end_byte
        for i in range(self.dim):
            stride = int(self.strides_bytes[i])
            size = int(self.shape[i])
            if size < 0:
                raise Exception('size < 0')
            if size == 0:
                self.start_byte = None
                self.end_byte = None
            if stride > 0:
                if self.end_byte is not None:
                    self.end_byte += (size - 1) * stride
            else:
                if self.start_byte is not None:
                    self.start_byte += (size - 1) * stride

        self.has_struct = self.element_type.has_struct

    def get_dtype(self):
        element_dtype = self.element_type.get_dtype()

        stride = element_dtype.itemsize
        for i in reversed(range(len(self.shape))):
            # Check stride
            if self.strides_bytes[i] != stride:
                raise Exception('Got unexpected stride in array for dimension {}: Got {}, expected {}'.format(i, self.strides_bytes[i], stride))
            stride *= self.shape[i]

        return numpy.dtype((element_dtype, tuple(self.shape)))


class BufferTypeStruct(BufferTypeBase):
    def __init__(self, members):
        self.members = members

        self.start_byte = None
        self.end_byte = None
        for member in self.members:
            if member.type.start_byte is not None:
                member_start = member.type.start_byte + member.offset
                if self.start_byte is None or member_start < self.start_byte:
                    self.start_byte = member_start
            if member.type.end_byte is not None:
                member_end = member.type.end_byte + member.offset
                if self.end_byte is None or member_end > self.end_byte:
                    self.end_byte = member_end

        self.has_struct = True

    def get_dtype(self):
        args = {
            'names': [],
            'formats': [],
            'offsets': [],
        }
        for member in self.members:
            name = member.name
            if name is None:
                name = ''
            args['names'].append(name)
            args['formats'].append(member.type.get_dtype())
            args['offsets'].append(member.offset)
        dtype = numpy.dtype(args)
        return dtype


class BufferTypeStructMember:
    def __init__(self, name, offset, type):
        self.name = name
        self.offset = offset
        self.type = type


def parse_buffer_type(json_data):
    voxie.json_util.expect_array(json_data)
    if len(json_data) == 0:
        raise Exception("Got invalid buffer type: Got zero-length array")
    kind = voxie.json_util.expect_string(json_data[0])
    if kind == "primitive":
        if len(json_data) != 4:
            raise Exception("Got invalid buffer type: Got unexpected array size for primitive")
        return BufferTypePrimitive(voxie.json_util.expect_string(json_data[1]), voxie.json_util.expect_int(json_data[2]), voxie.json_util.expect_string(json_data[3]))
    elif kind == "array":
        if len(json_data) != 4:
            raise Exception("Got invalid buffer type: Got unexpected array size for array")

        shapeAr = voxie.json_util.expect_array(json_data[1])
        for val in shapeAr:
            if voxie.json_util.expect_int(val) < 0:
                raise Exception("Got invalid buffer type: Got negative size")

        stridesAr = voxie.json_util.expect_array(json_data[2])
        if len(stridesAr) != len(shapeAr):
            raise Exception("Got invalid buffer type: Got dimension mismatch for array")
        for val in stridesAr:
            voxie.json_util.expect_int(val)

        return BufferTypeArray(shapeAr, stridesAr, parse_buffer_type(json_data[3]))
    elif kind == "struct":
        if len(json_data) != 2:
            raise Exception("Got invalid buffer type: Got unexpected array size for struct")

        members = []
        for memberJson in voxie.json_util.expect_array(json_data[1]):
            voxie.json_util.expect_array(memberJson)
            if len(memberJson) != 3:
                raise Exception("Got invalid buffer type: Got unexpected array size for struct member")

            name = None
            if memberJson[0] is not None:
                name = voxie.json_util.expect_string(memberJson[0])

            members.append(BufferTypeStructMember(name, voxie.json_util.expect_int(memberJson[1]), parse_buffer_type(memberJson[2])))

        return BufferTypeStruct(members)
    else:
        raise Exception("Got invalid buffer type: Got unknown type kind: {!r}".format(kind))


# TODO: Buffer probably should own the used ExternalDataUpdate object (for writable buffers) and render itself unusable when the Finish() method is called


class Buffer(object):
    def win_dbus_get_local_machine_id():
        import ctypes
        import ctypes.util
        import ctypes.wintypes

        lib = ctypes.CDLL(ctypes.util.find_library('dbus-1'))
        lib.dbus_get_local_machine_id.restype = ctypes.c_voidp
        lib.dbus_free.argtypes = [ctypes.c_voidp]
        ptr = lib.dbus_get_local_machine_id()
        uuid = ctypes.c_char_p(ptr).value
        lib.dbus_free(ptr)
        return str(uuid, 'utf-8')

    def __init__(self, info, dim, writable):
        self.info = info
        if dim is None:
            (self.handle, self.offset, buffer_type_dbus, self.metadata) = info
            self.buffer_type_json = voxie.json_dbus.dbus_to_json(buffer_type_dbus)
            self.buffer_type = parse_buffer_type(self.buffer_type_json)
            # Note: seems to work the same way without the first branch, but the first branch can also handle different strides
            if isinstance(self.buffer_type, BufferTypeArray):
                self.shape = self.buffer_type.shape
                self.strides_bytes = self.buffer_type.strides_bytes
                self.dtype = self.buffer_type.element_type.get_dtype()
            else:
                self.shape = []
                self.strides_bytes = []
                self.dtype = self.buffer_type.get_dtype()
            if self.buffer_type.start_byte is not None:
                if self.offset + self.buffer_type.start_byte < 0:
                    raise Exception('self.offset + self.buffer_type.start_byte < 0')
            if self.buffer_type.end_byte is not None and self.offset + self.buffer_type.end_byte >= 0:
                bytes = self.offset + self.buffer_type.end_byte
            else:
                bytes = 0
        else:
            (self.handle, self.offset, (self.typeName, self.typeSize,
                                        self.typeEndian), self.shape, self.strides_bytes, self.metadata) = info

            self.shape = numpy.array(self.shape, dtype='uint64')
            if len(self.strides_bytes) != dim or len(self.shape) != dim:
                raise Exception('Invalid dimension')

            if int(self.typeSize) % 8 != 0:
                raise Exception('Invalid size')
            self.dtype = endianMap[self.typeEndian] + \
                typeNameMap[self.typeName] + str(int(self.typeSize) // 8)
            # print (dtype)

            bytes = int(self.offset)
            pos0 = int(self.offset)
            bytes += int(self.typeSize) // 8
            for i in range(0, dim):
                stride = int(self.strides_bytes[i])
                size = int(self.shape[i])
                if size == 0:
                    bytes = 0
                    pos0 = 0
                    break
                if stride > 0:
                    bytes += (size - 1) * stride
                else:
                    pos0 += (size - 1) * stride
            # print (pos0, bytes)
            if int(self.offset) < 0:
                raise Exception('self.offset < 0')
            if int(pos0) < 0:
                raise Exception('pos0 < 0')

        self.handleType = getValue(self.handle["Type"], 's')
        if self.handleType == "UnixFileDescriptor":
            fd = getValue(self.handle["FileDescriptor"], 'h').take()
            prot = mmap.PROT_READ
            if writable:
                prot = prot | mmap.PROT_WRITE
            try:
                self.mmap = mmap.mmap(fd, bytes, mmap.MAP_SHARED, prot=prot)
            finally:
                os.close(fd)
        elif self.handleType == "WindowsNamedFileMapping":
            import ctypes
            import ctypes.util
            import ctypes.wintypes

            # print (self.handle)

            localMachineIDVoxie = getValue(self.handle["LocalMachineID"], 's')
            localMachineIDLocal = Buffer.win_dbus_get_local_machine_id()
            if localMachineIDVoxie != localMachineIDLocal:
                raise Exception(
                    "Machine ID mismatch: '" +
                    localMachineIDVoxie +
                    "' (Voxie) != '" +
                    localMachineIDLocal +
                    "' (local)")

            pid = os.getpid()
            sessionIDLocal = ctypes.wintypes.DWORD()
            if not ctypes.windll.kernel32.ProcessIdToSessionId(
                    ctypes.wintypes.DWORD(pid), ctypes.byref(sessionIDLocal)):
                raise Exception("Call to ProcessIdToSessionId() failed")
            sessionIDLocal = sessionIDLocal.value
            sessionIDVoxie = getValue(self.handle["SessionID"], 'u')
            if sessionIDVoxie != sessionIDLocal:
                raise Exception(
                    "Session ID mismatch: '%s' (Voxie) != '%s' (local)" %
                    (sessionIDVoxie, sessionIDLocal))

            objectName = getValue(self.handle["MappingObjectName"], 's')

            access = mmap.ACCESS_READ
            if writable:
                access = mmap.ACCESS_WRITE
            self.mmap = mmap.mmap(0, bytes, objectName, access=access)
        else:
            raise Exception('Unknown handle type: "' + self.handleType + '"')

        # print(self.shape)
        # print(self.dtype)
        # print(self.strides_bytes)
        # print(self.offset)
        # print(bytes)
        self.array = numpy.ndarray(
            self.shape,
            dtype=self.dtype,
            strides=self.strides_bytes,
            buffer=self.mmap,
            offset=self.offset)
        if dim is None and self.buffer_type.has_struct:
            self.array = self.array.view(numpy.recarray)

        self.mmap = None

    def __del__(self):
        # print('del %s' % self)
        pass

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        # self.mmap.close()
        return False

    def __array__(self, *args, **kwargs):
        return self.array.__array__(*args, **kwargs)

    def __getitem__(self, *args, **kwargs):
        return self.array.__getitem__(*args, **kwargs)

    def __setitem__(self, *args, **kwargs):
        return self.array.__setitem__(*args, **kwargs)
