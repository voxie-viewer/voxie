import os
import sys
import dbus
import mmap
import numpy
import argparse
import math

parser = argparse.ArgumentParser()
parser.add_argument('--voxie-bus-address')
parser.add_argument('--voxie-bus-name')
parser.add_argument('--voxie-slice-object')

emptyOptions = dbus.Dictionary(signature='sv')

class Voxie:
    def __init__(self, args = None):
        if args is not None and args.voxie_bus_address is not None:
            self.bus = dbus.bus.BusConnection(args.voxie_bus_address)
        else:
            self.bus = dbus.SessionBus()
        if args is not None and args.voxie_bus_name is not None:
            bus_name_un = args.voxie_bus_name
        else:
            bus_name_un = "de.uni_stuttgart.Voxie"

        self.voxie = self
        self.path = '/de/uni_stuttgart/Voxie'
        self.dbus = dbus.Interface (self.bus.get_object(bus_name_un, self.path), 'de.uni_stuttgart.Voxie.Voxie')
        self.bus_name = self.dbus.bus_name
        self.dbus_properties = dbus.Interface (self.voxie.bus.get_object(self.voxie.bus_name, self.path), 'org.freedesktop.DBus.Properties')
        self.path_gui = self.dbus_properties.Get('de.uni_stuttgart.Voxie.Voxie', 'Gui')
        self.dbus_gui = dbus.Interface (self.bus.get_object(self.bus_name, self.path_gui), 'de.uni_stuttgart.Voxie.Gui')
        self.dbus_gui_properties = dbus.Interface (self.bus.get_object(self.bus_name, self.path_gui), 'org.freedesktop.DBus.Properties')

    def getSlice(self, args = None):
        if args is not None and args.voxie_slice_object is not None:
            objectName = args.voxie_slice_object
        else:
            vis_path = self.dbus_gui_properties.Get('de.uni_stuttgart.Voxie.Gui', 'ActiveVisualizer')
            if vis_path == '/':
                raise Exception('No active visualizer')
            vis = self.bus.get_object(self.bus_name, vis_path)
            vis_properties = dbus.Interface (vis, 'org.freedesktop.DBus.Properties')
            objectName = vis_properties.Get('de.uni_stuttgart.Voxie.SliceDataVisualizer', 'Slice')
        return Slice (self, objectName)

    def createClient(self, options = emptyOptions):
        return Client (self, self.dbus.CreateClient(options))

    def createImage(self, client, size, options = emptyOptions):
        return Image (client, self.dbus.CreateImage(client.path, size, options))

    def createVoxelData(self, client, size, options = emptyOptions):
        return VoxelData (self, self.dbus.CreateVoxelData(client.path, size, options), client)

    def createDataSet(self, name, data, options = emptyOptions):
        return DataSet (self, self.dbus.CreateDataSet(name, data.path, options))

    def getPlugin(self, name):
        return Plugin (self, self.dbus.GetPluginByName(name))

class DataSet:
    def __init__(self, voxie, path):
        self.voxie = voxie
        self.path = path
        self.dbus_properties = dbus.Interface (self.voxie.bus.get_object(self.voxie.bus_name, self.path), 'org.freedesktop.DBus.Properties')
        self.dbus = dbus.Interface (self.voxie.bus.get_object(self.voxie.bus_name, self.path), 'de.uni_stuttgart.Voxie.DataSet')
    def get(self, name):
        return self.dbus_properties.Get ('de.uni_stuttgart.Voxie.DataSet', name)
    def createSlice(self):
        return Slice (self.voxie, self.dbus.CreateSlice(emptyOptions))
    def getOriginalData(self):
        return VoxelData (self.voxie, self.get ('OriginalData'))
    def getFilteredData(self):
        return VoxelData (self.voxie, self.get ('FilteredData'))

class VoxelData:
    def __init__(self, voxie, path, client = None):
        self.voxie = voxie
        self.path = path
        self.client = client
        self.dbus_properties = dbus.Interface (self.voxie.bus.get_object(self.voxie.bus_name, self.path), 'org.freedesktop.DBus.Properties')
        self.dbus = dbus.Interface (self.voxie.bus.get_object(self.voxie.bus_name, self.path), 'de.uni_stuttgart.Voxie.VoxelData')
    def get(self, name):
        return self.dbus_properties.Get ('de.uni_stuttgart.Voxie.VoxelData', name)
    def getDataReadonly(self):
        return Buffer (self.dbus.GetDataReadonly(), 3, False)
    def getDataWritable(self):
        return Buffer (self.dbus.GetDataWritable(), 3, True)
    def __enter__(self):
        return self
    def __exit__(self, type, value, traceback):
        if self.client is not None:
            self.client.dbus.DecRefCount(self.path)
        return False

class Slice:
    def __init__(self, voxie, path):
        self.voxie = voxie
        self.path = path
        self.dbus_properties = dbus.Interface (self.voxie.bus.get_object(self.voxie.bus_name, self.path), 'org.freedesktop.DBus.Properties')
        self.dbus = dbus.Interface (self.voxie.bus.get_object(self.voxie.bus_name, self.path), 'de.uni_stuttgart.Voxie.Slice')

    def get(self, name):
        return self.dbus_properties.Get ('de.uni_stuttgart.Voxie.Slice', name)
    def set(self, name, value):
        return self.dbus_properties.Set ('de.uni_stuttgart.Voxie.Slice', name, value)

    def __build_plane(origin, rotation):
        or_val = dbus.Struct((dbus.Double(origin[0]), dbus.Double(origin[1]), dbus.Double(origin[2])))
        quaternion = rotation.quaternion
        rot_val = dbus.Struct((dbus.Double(quaternion.a), dbus.Double(quaternion.b), dbus.Double(quaternion.c), dbus.Double(quaternion.d)))
        return dbus.Struct((or_val, rot_val))

    origin = property (lambda self: numpy.array(self.get('Plane')[0]), lambda self, value: self.set('Plane', Slice.__build_plane (value, self.rotation)))
    rotation = property (lambda self: Rotation(self.get('Plane')[1]), lambda self, value: self.set('Plane', Slice.__build_plane (self.origin, value)))

    def getDataSet(self):
        return DataSet(self.voxie, self.get('DataSet'))

class Plugin:
    def __init__(self, voxie, path):
        self.voxie = voxie
        self.path = path
        self.dbus_properties = dbus.Interface (self.voxie.bus.get_object(self.voxie.bus_name, self.path), 'org.freedesktop.DBus.Properties')
        self.dbus = dbus.Interface (self.voxie.bus.get_object(self.voxie.bus_name, self.path), 'de.uni_stuttgart.Voxie.Plugin')

    def get(self, name):
        return self.dbus_properties.Get ('de.uni_stuttgart.Voxie.Plugin', name)

    def getMemberDBus(self, type, name):
        path = self.dbus.GetMemberByName (type, name)
        return self.voxie.bus.get_object(self.voxie.bus_name, path)

class Image:
    def __init__(self, client, path):
        self.voxie = client.voxie
        self.client = client
        self.path = path
        self.dbus = dbus.Interface (self.voxie.bus.get_object(self.voxie.bus_name, self.path), 'de.uni_stuttgart.Voxie.Image')
    def getDataReadonly(self):
        return Buffer (self.dbus.GetDataReadonly(), 2, False)
    def __enter__(self):
        return self
    def __exit__(self, type, value, traceback):
        self.client.dbus.DecRefCount(self.path)
        return False

class Client:
    def __init__(self, voxie, path):
        self.voxie = voxie
        self.path = path
        self.dbus = dbus.Interface (self.voxie.bus.get_object(self.voxie.bus_name, self.path), 'de.uni_stuttgart.Voxie.Client')
    def __enter__(self):
        return self
    def __exit__(self, type, value, traceback):
        self.voxie.dbus.DestroyClient(self.path, emptyOptions)
        return False

typeNameMap = {
    "float": "f",
    "uint": "u",
    "int": "i",
}
endianMap = {
    "none": "|",
    "little": "<",
    "big": ">",
}
class Buffer:
    def __init__(self, info, dim, writable):
        self.info = info
        (self.handle, self.offset, (self.typeName, self.typeSize, self.typeEndian), self.size, self.stride, self.metadata) = info

        self.handleType = self.handle["Type"]
        if self.handleType == "UnixFileDescriptor":
            fd = self.handle["FileDescriptor"].take()
            prot = mmap.PROT_READ
            if writable:
                prot = prot | mmap.PROT_WRITE
            try:
                self.mmap = mmap.mmap(fd, 0, prot = prot)
            finally:
                os.close(fd)
        else:
            raise Exception('Unknown handle type')

        self.size = numpy.array(self.size, dtype='uint64')
        if len(self.stride) != dim or len(self.size) != dim:
            raise Exception('Invalid dimension')

        if int (self.typeSize) % 8 != 0:
            raise Exception('Invalid size')
        self.dtype = endianMap[self.typeEndian] + typeNameMap[self.typeName] + str (int (self.typeSize) // 8)
        #print (dtype)

        self.array = numpy.ndarray(self.size, dtype=self.dtype, strides=self.stride, buffer=self.mmap, offset=self.offset)

    def __enter__(self):
        return self
    def __exit__(self, type, value, traceback):
        self.mmap.close()
        return False

class Quaternion:
    def __init__(self, value):
        self.value = numpy.array (value, dtype=numpy.double)
    def __mul__(self, other):
        if isinstance (other, Quaternion):
            a = self.a * other.a - self.b * other.b - self.c * other.c - self.d * other.d
            b = self.a * other.b + self.b * other.a + self.c * other.d - self.d * other.c
            c = self.a * other.c - self.b * other.d + self.c * other.a + self.d * other.b
            d = self.a * other.d + self.b * other.c - self.c * other.b + self.d * other.a
            return Quaternion((a, b, c, d))
        else:
            return Quaternion(self.value * other)
    def absolute(self):
        return numpy.sqrt(numpy.sum(self.value * self.value))
    def conjugate(self):
        return Quaternion((self.a, -self.b, -self.c, -self.d))
    def __str__(self):
        return '(%s, %s, %s, %s)' % (self.a, self.b, self.c, self.d)
    a = property(lambda self: self.value[0])
    b = property(lambda self: self.value[1])
    c = property(lambda self: self.value[2])
    d = property(lambda self: self.value[3])

class Rotation:
    def __init__(self, quaternion):
        if not isinstance(quaternion, Quaternion):
            quaternion = Quaternion(quaternion)
        absolute = quaternion.absolute()
        if absolute < 0.99 or absolute > 1.01:
            raise Exception('Absolute value of quaternion %s (%s) is outside range 0.99-1.01' % (quaternion, absolute))
        self.quaternion = quaternion * (1 / absolute)
    def __mul__(self, other):
        if isinstance(other, Rotation):
            return Rotation(self.quaternion * other.quaternion)
        else:
            (x, y, z) = other
            res = self.quaternion * Quaternion((0, x, y, z)) * self.quaternion.conjugate()
            return res.value[1], res.value[2], res.value[3]
    @property
    def inverse(self):
        return Rotation(self.quaternion.conjugate())
    def __str__(self):
        return 'Rotation%s' % (self.quaternion)
    def fromAxisAngle(axis, rad):
        sin = math.sin (rad / 2);
        cos = math.cos (rad / 2);
        axis = numpy.array (axis)
        axis = axis / numpy.sqrt(numpy.sum(axis*axis))
        (x, y, z) = axis
        return Rotation ((cos, x * sin, y * sin, z * sin));
    def fromAxisAngleDeg(axis, deg):
        return Rotation.fromAxisAngle(axis, deg * math.pi / 180)
