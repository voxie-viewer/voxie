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
        (self.handle, self.offset, (self.typeName, self.typeSize,
                                    self.typeEndian), self.size, self.stride, self.metadata) = info

        self.size = numpy.array(self.size, dtype='uint64')
        if len(self.stride) != dim or len(self.size) != dim:
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
            stride = int(self.stride[i])
            size = int(self.size[i])
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

        self.array = numpy.ndarray(
            self.size,
            dtype=self.dtype,
            strides=self.stride,
            buffer=self.mmap,
            offset=self.offset)

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
