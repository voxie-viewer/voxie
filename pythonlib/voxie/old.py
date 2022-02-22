import sys
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
import voxie
from voxie.buffer import Buffer
import dbus

emptyOptions = dbus.Dictionary(signature='sv')


class VolumeObject(object):
    def __init__(self, voxie, path):
        self.voxie = voxie
        self.path = path
        self.dbus_properties = dbus.Interface(
            self.voxie.bus.get_object(
                self.voxie.bus_name,
                self.path),
            'org.freedesktop.DBus.Properties')
        self.dbus = dbus.Interface(
            self.voxie.bus.get_object(
                self.voxie.bus_name,
                self.path),
            'de.uni_stuttgart.Voxie.DataObject')

    def get(self, name):
        return self.dbus_properties.Get('de.uni_stuttgart.Voxie.DataObject', name)

    def createSlice(self):
        return Slice(self.voxie, self.dbus.CreateSlice(emptyOptions))

    def getVolumeDataVoxel(self):
        return VolumeDataVoxel(self.voxie, self.get('Data'))


class VolumeDataVoxel(object):
    def __init__(self, voxie, path, client=None):
        self.voxie = voxie
        self.path = path
        self.client = client
        self.update = None
        self.dbus_properties = dbus.Interface(
            self.voxie.bus.get_object(
                self.voxie.bus_name,
                self.path),
            'org.freedesktop.DBus.Properties')
        self.dbus = dbus.Interface(
            self.voxie.bus.get_object(
                self.voxie.bus_name,
                self.path),
            'de.uni_stuttgart.Voxie.VolumeDataVoxel')
        self.dbusData = dbus.Interface(
            self.voxie.bus.get_object(
                self.voxie.bus_name,
                self.path),
            'de.uni_stuttgart.Voxie.Data')

    def get(self, name):
        return self.dbus_properties.Get(
            'de.uni_stuttgart.Voxie.VolumeDataVoxel', name)

    def getDataReadonly(self):
        return Buffer(self.dbus.GetDataReadonly(emptyOptions), 3, False)

    def getDataWritable(self):
        # TODO: Update should actually be destroyed at some point
        if self.client is None:  # Another hack
            self.client = self.voxie.createClient()
        if self.update is None:
            self.update = self.dbusData.CreateUpdate(
                self.client.path, emptyOptions)
        return Buffer(self.dbus.GetDataWritable(self.update, emptyOptions), 3, True)

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        if self.client is not None:
            self.client.dbus.DecRefCount(self.path)
        return False


class LazyRawData2D(object):
    def __init__(self, voxie, path, client=None):
        self.voxie = voxie
        self.path = path
        self.client = client
        self.dbus_properties = dbus.Interface(
            self.voxie.bus.get_object(
                self.voxie.bus_name,
                self.path),
            'org.freedesktop.DBus.Properties')
        self.dbus = dbus.Interface(
            self.voxie.bus.get_object(
                self.voxie.bus_name,
                self.path),
            'de.uni_stuttgart.Voxie.LazyRawData2D')

    def get(self, name):
        return self.dbus_properties.Get(
            'de.uni_stuttgart.Voxie.VolumeDataVoxel', name)

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        if self.client is not None:
            self.client.dbus.DecRefCount(self.path)
        return False


class Slice(object):
    def __init__(self, voxie, path):
        self.voxie = voxie
        self.path = path
        self.dbus_properties = dbus.Interface(
            self.voxie.bus.get_object(
                self.voxie.bus_name,
                self.path),
            'org.freedesktop.DBus.Properties')
        self.dbus = dbus.Interface(
            self.voxie.bus.get_object(
                self.voxie.bus_name,
                self.path),
            'de.uni_stuttgart.Voxie.Slice')

    def get(self, name):
        return self.dbus_properties.Get('de.uni_stuttgart.Voxie.Slice', name)

    def set(self, name, value):
        return self.dbus_properties.Set(
            'de.uni_stuttgart.Voxie.Slice', name, value)

    def __build_plane(origin, rotation):
        or_val = dbus.Struct(
            (dbus.Double(
                origin[0]), dbus.Double(
                origin[1]), dbus.Double(
                origin[2])))
        quaternion = rotation.quaternion
        rot_val = dbus.Struct(
            (dbus.Double(
                quaternion.a), dbus.Double(
                quaternion.b), dbus.Double(
                quaternion.c), dbus.Double(
                    quaternion.d)))
        return dbus.Struct((or_val, rot_val))

    origin = property(
        lambda self: numpy.array(
            self.get('Plane')[0]), lambda self, value: self.set(
            'Plane', Slice.__build_plane(
                value, self.rotation)))
    rotation = property(
        lambda self: Rotation(
            self.get('Plane')[1]), lambda self, value: self.set(
            'Plane', Slice.__build_plane(
                self.origin, value)))

    def getVolumeObject(self):
        return VolumeObject(self.voxie, self.get('VolumeObject'))


class Plugin(object):
    def __init__(self, voxie, path):
        self.voxie = voxie
        self.path = path
        self.dbus_properties = dbus.Interface(
            self.voxie.bus.get_object(
                self.voxie.bus_name,
                self.path),
            'org.freedesktop.DBus.Properties')
        self.dbus = dbus.Interface(
            self.voxie.bus.get_object(
                self.voxie.bus_name,
                self.path),
            'de.uni_stuttgart.Voxie.Plugin')

    def get(self, name):
        return self.dbus_properties.Get('de.uni_stuttgart.Voxie.Plugin', name)

    def getMemberDBus(self, type, name):
        path = self.dbus.GetComponent(type, name)
        return self.voxie.bus.get_object(self.voxie.bus_name, path)


# TODO: This is broken by now, use new interface
class Image(object):
    def __init__(self, client, path):
        self.voxie = client.voxie
        self.client = client
        self.path = path
        self.dbus = dbus.Interface(
            self.voxie.bus.get_object(
                self.voxie.bus_name,
                self.path),
            'de.uni_stuttgart.Voxie.Image')

    def getDataReadonly(self):
        return Buffer(self.dbus.GetDataReadonly({}), 2, False)

    def getDataWritable(self):
        return Buffer(self.dbus.GetDataWritable({}), 2, True)

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self.client.dbus.DecRefCount(self.path)
        return False


class Client(object):
    def __init__(self, voxie, path):
        self.voxie = voxie
        self.path = path
        self.dbus = dbus.Interface(
            self.voxie.bus.get_object(
                self.voxie.bus_name,
                self.path),
            'de.uni_stuttgart.Voxie.Client')

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        try:
            self.voxie.dbus.DestroyClient(self.path, emptyOptions)
        except Exception:
            sys.stderr.write('Got exception calling DestroyClient()\n')
        return False


class ExternalOperation(object):
    def __init__(self, client, path):
        self.voxie = client.voxie
        self.client = client
        self.path = path
        self.dbus = dbus.Interface(
            self.voxie.bus.get_object(
                self.voxie.bus_name,
                self.path),
            'de.uni_stuttgart.Voxie.ExternalOperation')

    def interface(self, name):
        return dbus.Interface(
            self.voxie.bus.get_object(
                self.voxie.bus_name,
                self.path),
            'de.uni_stuttgart.Voxie.ExternalOperation' +
            name)

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        try:
            if type is not None:
                self.dbus.FinishError(type.__name__, str(value))
                return True
        finally:
            self.client.dbus.DecRefCount(self.path)
        return False


class CcaResult(object):
    def __init__(self, voxie, path):
        self.voxie = voxie
        self.path = path
        self.dbus_properties = dbus.Interface(self.voxie.bus.get_object(self.voxie.bus_name, self.path),
                                              'org.freedesktop.DBus.Properties')
        self.dbus = dbus.Interface(self.voxie.bus.get_object(self.voxie.bus_name, self.path),
                                   'de.uni_stuttgart.Voxie.CcaResult')

    def get(self, name):
        return self.dbus_properties.Get('de.uni_stuttgart.Voxie.CcaResult', name)

    def addLabelAttribute(self, labelID, numberOfVoxel, boundingBox, centerOfMass,
                          weightedCenterOfMass, sumOfValues, average):
        self.dbus.addLabelAttribute(labelID, numberOfVoxel,
                                    boundingBox[0][0], boundingBox[0][1],
                                    boundingBox[1][0], boundingBox[1][1],
                                    boundingBox[2][0], boundingBox[2][1],
                                    centerOfMass[0], centerOfMass[1], centerOfMass[2],
                                    weightedCenterOfMass[0], weightedCenterOfMass[1],
                                    weightedCenterOfMass[2],
                                    sumOfValues, average)


class Voxie(object):
    def __init__(self, args, *, mainloop=None):
        if args is None:
            args = voxie.parse.parser.parse_args()
        if hasattr(args, 'object_path'):
            self.object_path = args.object_path
        if args.voxie_bus_address is not None:
            self.bus = dbus.bus.BusConnection(
                args.voxie_bus_address, mainloop=mainloop)
        else:
            self.bus = dbus.SessionBus(mainloop=mainloop)
        bus_name_un = args.voxie_bus_name

        self.args = args
        self.path = '/de/uni_stuttgart/Voxie'
        self.dbus = dbus.Interface(
            self.bus.get_object(
                bus_name_un,
                self.path),
            'de.uni_stuttgart.Voxie.Instance')
        self.bus_name = self.dbus.bus_name
        self.dbus_properties = dbus.Interface(
            self.bus.get_object(
                self.bus_name,
                self.path),
            'org.freedesktop.DBus.Properties')
        self.path_gui = self.dbus_properties.Get(
            'de.uni_stuttgart.Voxie.Instance', 'Gui')
        self.dbus_gui = dbus.Interface(
            self.bus.get_object(
                self.bus_name,
                self.path_gui),
            'de.uni_stuttgart.Voxie.Gui')
        self.dbus_gui_properties = dbus.Interface(
            self.bus.get_object(
                self.bus_name,
                self.path_gui),
            'org.freedesktop.DBus.Properties')
        self.dbus_clientManager = dbus.Interface(
            self.bus.get_object(
                bus_name_un,
                '/de/uni_stuttgart/Voxie/ClientManager'),
            'de.uni_stuttgart.Voxie.ClientManager')

    # TODO: Remove and use SetProperty?!?
    def setAttribute(self, path, classPath, attributeName, value):
        """
        :param path: Dbus path for object.
        :param classPath: Something like ``de.uni_stuttgart.Voxie.Segmentation``
        :param attributeName: Something like ``Threshold``
        :param value: The value to set.
        :return:
        """
        if classPath == 'de.uni_stuttgart.Voxie.Object' and attributeName == 'DisplayName':  # TODO: Who should set the DisplayName and how?
            return
        dbus_object = self.bus.get_object(self.bus_name, path)
        dbus_properties = dbus.Interface(
            dbus_object, 'org.freedesktop.DBus.Properties')
        dbus_properties.Set(classPath, attributeName, value)

    # TODO: Remove and use GetProperty?!?
    def getAttribute(self, path, classPath, attributeName):
        """
        :param path: Dbus path for object.
        :param classPath: Something like ``de.uni_stuttgart.Voxie.Segmentation``
        :param attributeName: Something like ``Threshold``
        :return:
        """
        dbus_object = self.bus.get_object(self.bus_name, path)
        dbus_properties = dbus.Interface(
            dbus_object, 'org.freedesktop.DBus.Properties')
        return dbus_properties.Get(classPath, attributeName)

    def getVolumeObject(self, number=0):
        objectName = getattr(self.args, "voxie_input_dataset%d" % number, None)
        if objectName is None:
            vis_path = self.dbus_gui_properties.Get(
                'de.uni_stuttgart.Voxie.Gui', 'ActiveVisualizer')
            if vis_path == '/':
                raise Exception('No active visualizer')
            vis = self.bus.get_object(self.bus_name, vis_path)
            vis_properties = dbus.Interface(
                vis, 'org.freedesktop.DBus.Properties')
            # TODO: search for VolumeObject parent of current vis?
            # objectName = vis_properties.Get('de.uni_stuttgart.Voxie.VolumeDataVisualizer', 'VolumeObject')
            raise Exception('TODO: rewrite script / provide dataset')
        return VolumeObject(self, objectName)

    def getSlice(self, args=None):
        raise Exception('TODO: rewrite script with new DBus wrapper')

    def getOutputVolumeObject(self):
        """
        Returns the dataset to save the output from a filter object.
        :return:
        """
        objectName = getattr(self.args, "voxie_output_dataset", None)
        if objectName is None:
            raise Exception(
                "Reference to output dataset was not given. Specify it or use createVolumeObject.")
        return VolumeObject(self, objectName)

    def getPreviewVolumeObject(self):
        """
        Returns the dataset to save the output from a filter object.
        :return:
        """
        objectName = getattr(self.args, "voxie_preview_dataset", None)
        if objectName is None:
            raise Exception(
                "Reference to output dataset was not given. Specify it or use createVolumeObject.")
        return VolumeObject(self, objectName)

    def calculatePreviewVolumeObject(self):
        inputArray = self.getVolumeObject().getVolumeDataVoxel().getDataReadonly().array
        start_point = self.getAttribute(
            self.object_path,
            "de.uni_stuttgart.Voxie.FilterObject",
            "PreviewPoint")
        previewBoxActive = self.getAttribute(
            self.object_path,
            "de.uni_stuttgart.Voxie.FilterObject",
            "PreviewActive")
        if (previewBoxActive):
            previewVolumeObject = self.getPreviewVolumeObject()
            with previewVolumeObject.getVolumeDataVoxel().getDataWritable() as outputBuffer:
                output_size = (
                    numpy.size(outputBuffer.array, 0),
                    numpy.size(outputBuffer.array, 1),
                    numpy.size(outputBuffer.array, 2)
                )
                input_size = inputArray.shape

                # check whether a fast calculation without checking boundaries is possible
                if (start_point[0] >= 0 and start_point[1] >= 0 and start_point[2] >= 0 and
                        start_point[0] + output_size[0] <= input_size[0] and start_point[1] + output_size[1] <=
                        input_size[
                            1] and start_point[2] + output_size[2] <= input_size[2]):
                    print("calculating without resizing boundaries (fast)")
                    outputBuffer.array[:] = inputArray[
                        (start_point[0]):(start_point[0] + output_size[0]),
                        (start_point[1]):(start_point[1] + output_size[1]),
                        (start_point[2]):(
                            start_point[2] + output_size[2])
                    ]
                else:
                    print("calculating with resizing boundaries (slow)")
                    for x in range(output_size[0] - 1):
                        for y in range(output_size[1] - 1):
                            for z in range(output_size[2] - 1):
                                if (0 <= start_point[0] + x < input_size[0] and 0 <= start_point[1] + y < input_size[
                                    1] and 0 <= start_point[2] + z < input_size[2] and
                                        start_point[0] + x < output_size[0] and start_point[1] + y < output_size[1] and
                                        start_point[2] + z < output_size[2]):
                                    outputBuffer.array[x][y][z] = inputArray[start_point[0] + x][start_point[1] + y][
                                        start_point[2] + z]
                                else:
                                    outputBuffer.array[x][y][z] = 0
            return previewVolumeObject
        else:
            return self.getVolumeObject()

    def getCcaResult(self):
        objectName = getattr(self.args, "voxie_output_dataset", None)
        if objectName is None:
            raise Exception("Reference to output Label was not given.")
        return CcaResult(self, objectName)

    def createClient(self, options=emptyOptions):
        return voxie.old.Client(self, self.dbus_clientManager.CreateClient(options))

    def createImage(self, client, size, options=emptyOptions):
        return Image(client, self.dbus.CreateImage(client.path, size, ('float', 32, 'native'), options))

    def createVolumeDataVoxel(self, client, size, options=emptyOptions):
        # TODO: Clean up or remove this stuff
        options2 = dict(options)
        del options2['Origin']
        del options2['Spacing']
        return VolumeDataVoxel(
            self, self.dbus.CreateVolumeDataVoxel(
                client.path, size, ('float', 32, 'native'), options['Origin'], options['Spacing'], options2), client)

    def createLazyRawData2D(self, client, provider, options=emptyOptions):
        return LazyRawData2D(
            self, self.dbus.CreateLazyRawData2D(
                client.path, provider, options), client)

    def createVolumeObject(self, name, data, options=emptyOptions):
        # return VolumeObject(self, self.dbus.CreateVolumeObject(name, data.path, options))
        options2 = dict(options)
        options2['Data'] = data.path
        options2['ManualDisplayName'] = (True, name)
        volObjPath = self.dbus.GetComponent(
            'de.uni_stuttgart.Voxie.ComponentType.NodePrototype', 'de.uni_stuttgart.Voxie.VolumeObject', {})
        volObj = dbus.Interface(
            self.bus.get_object(
                self.dbus.bus_name,
                volObjPath),
            'de.uni_stuttgart.Voxie.ObjectPrototype')
        return VolumeObject(self, volObj.CreateObject({}, options2))

    def getPlugin(self, name):
        return Plugin(self, self.dbus.GetPluginByName(name))

    def claimExternalOperation(self, client, path):
        exop_dbus = dbus.Interface(
            self.bus.get_object(
                self.bus_name,
                path),
            'de.uni_stuttgart.Voxie.ExternalOperation')
        exop_dbus.ClaimOperation(client.path)
        return ExternalOperation(client, path)
