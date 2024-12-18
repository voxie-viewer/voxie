from voxie.buffer import Buffer
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
import voxie.json_dbus
import dbus
import numpy as np
import sys
import os
import xml.etree.ElementTree
import glob
from . import refcountingcontext
from .refcountingcontext import RefCountingContext

# Note: Nothing here is safe for multithreaded operation

# python3 -c 'import gc, numpy as np; import voxie; args = voxie.parser.parse_args(); context = voxie.VoxieContext(args); instance = context.createInstance(); instanceOld = voxie.Voxie(args); print([type(o) for o in gc.get_referrers(context.createInstance())])'

# python3 -c 'import gc, numpy as np; import voxie; args = voxie.parser.parse_args(); context = voxie.VoxieContext(args); instance = context.createInstance(); instanceOld = voxie.Voxie(args); print([type(o) for o in gc.get_referrers(context.createInstance().CreateVolumeDataVoxel((5, 6, 7), ("float", 32, "native"), (0, 0, 0), (1, 1, 1)))])'

# import numpy as np; import voxie; args = voxie.parser.parse_args(); context = voxie.VoxieContext(args); instance = context.createInstance();
# vx = voxie.cast(voxie.cast(instance.Gui.SelectedObjects[0], 'de.uni_stuttgart.Voxie.DataObject').Data, 'de.uni_stuttgart.Voxie.VolumeDataVoxel')
# upd = vx.CreateUpdate(); vxwr = vx.GetBufferWritable(upd)

# TODO: Which value should be used here?
timeoutValue = 2000000


class DebugOptionsImpl:
    def __init__(self, instance):
        self.__list = instance.ListDebugOptions()
        self.__opt_dict = {}
        for entry in self.__list:
            name = entry.Name
            dbusSig = entry.DBusSignature
            name = name.replace('.', '_')
            data = {
                'Name': name,
                'Signature': dbusSig,
                'Object': entry,
            }
            if data['Name'] in self.__opt_dict:
                raise Exception('Duplicate debug option name: {!r}'.format(data['Name']))
            self.__opt_dict[data['Name']] = data
            # For tab completion
            object.__setattr__(self, data['Name'], None)

    def __getattribute__(self, name):
        if name.startswith('_'):
            return object.__getattribute__(self, name)
        if name not in self.__opt_dict:
            raise Exception('Could not find debug option {!r}'.format(name))
        entry = self.__opt_dict[name]
        return entry['Object'].GetValue().getValue(entry['Signature'])

    def __setattr__(self, name, value):
        if name.startswith('_'):
            return object.__setattr__(self, name, value)
        if name not in self.__opt_dict:
            raise Exception('Could not find debug option {!r}'.format(name))
        entry = self.__opt_dict[name]
        entry['Object'].SetValue(voxie.Variant(entry['Signature'], value))


class InstanceImpl(voxie.DBusObject):
    def __init__(self, obj, interfaces, context=None, referenceCountingObject=None):
        voxie.DBusObject.__init__(
            self, obj, interfaces, context=context, referenceCountingObject=referenceCountingObject)
        self.__DebugOptions = None

    def GetPrototype(self, name, options={}):
        # TODO: Drop ObjectPrototype here? Might break compatibility with old scripts
        return self.Components.GetComponent('de.uni_stuttgart.Voxie.ComponentType.NodePrototype', name, options).CastTo(['de.uni_stuttgart.Voxie.NodePrototype', 'de.uni_stuttgart.Voxie.ObjectPrototype'])

    # Same as GetPrototype().CreateNode(), but if there is an error while creating the node, print a warning to stderr and return None, and if an error while setting a property occurs, print a warning to stderr and continue
    def CreateNodeChecked(self, name, properties, options={}):
        options = dict(options)
        # TODO: Continue on errors while setting properties
        try:
            prototype = self.GetPrototype(name)
            return prototype.CreateNode(properties, options)
        except dbus.exceptions.DBusException as e:
            # TODO: More details in error message
            print('Error while creating node: ' + str(e), file=sys.stderr)
            return None

    # Same as GetPrototype().CreateObject(), but if there is an error while creating the object, print a warning to stderr and return None, and if an error while setting a property occurs, print a warning to stderr and continue
    def CreateObjectChecked(self, name, properties, options={}):
        options = dict(options)
        # TODO: Continue on errors while setting properties
        try:
            prototype = self.GetPrototype(name)
            return prototype.CreateObject(properties, options)
        except dbus.exceptions.DBusException as e:
            # TODO: More details in error message
            print('Error while creating object: ' + str(e), file=sys.stderr)
            return None

    # Automatically set the timeout for OpenFile
    def OpenFile(self, file, options={}, *, DBusObject_timeout=timeoutValue):
        return self._getDBusMethod('OpenFile')(file, options, DBusObject_timeout=DBusObject_timeout)

    # Same as OpenFile(), but if there is an error while loading the file, print a warning to stderr and return None
    def OpenFileChecked(self, file, options={}):
        options = dict(options)
        try:
            return self.OpenFile(file, options)
        except dbus.exceptions.DBusException as e:
            # TODO: More details in error message
            print('Error while opening file: ' + str(e), file=sys.stderr)
            return None

    # Automatically set the timeout for Import
    def Import(self, fileName, options={}, *, DBusObject_timeout=timeoutValue):
        return self._getDBusMethod('Import')(fileName, options, DBusObject_timeout=DBusObject_timeout)

    @property
    def DebugOptions(self):
        if self.__DebugOptions is not None:
            return self.__DebugOptions
        self.__DebugOptions = DebugOptionsImpl(self)
        return self.__DebugOptions

    # Automatically set the timeout for CreateVolumeDataVoxel (creating a large volume can be slow)
    def CreateVolumeDataVoxel(self, arrayShape, dataType, volumeOrigin, gridSpacing, options={}, *, DBusObject_timeout=timeoutValue):
        return self._getDBusMethod('CreateVolumeDataVoxel')(arrayShape, dataType, volumeOrigin, gridSpacing, options, DBusObject_timeout=DBusObject_timeout)


class DynamicObjectImpl(voxie.DBusObject):
    def __init__(self, obj, interfaces, context=None, referenceCountingObject=None):
        voxie.DBusObject.__init__(
            self, obj, interfaces, context=context, referenceCountingObject=referenceCountingObject)

    def CastTo(self, newInterface):
        return refcountingcontext.cast(self, newInterface)

    def IsInstance(self, interface):
        return refcountingcontext.isInstance(self, interface)


class ComponentContainerImpl(voxie.DBusObject):
    def __init__(self, obj, interfaces, context=None, referenceCountingObject=None):
        voxie.DBusObject.__init__(
            self, obj, interfaces, context=context, referenceCountingObject=referenceCountingObject)

    # Don't automatically cast to target type because the component type name might not match the DBus interface name

    # # Do a static cast to the target type
    # def GetComponent(self, type, name, options={}):
    #     return refcountingcontext.castStatic(self._getDBusMethod('GetComponent')(type, name, options), type)

    # # Do static casts to the target type
    # def ListComponents(self, type, options={}):
    #     result = []
    #     for comp in self._getDBusMethod('ListComponents')(type, options):
    #         result.append(refcountingcontext.castStatic(comp, type))
    #     return result


class NodePrototypeImpl(DynamicObjectImpl):
    def __init__(self, obj, interfaces, context=None, referenceCountingObject=None):
        voxie.DBusObject.__init__(
            self, obj, interfaces, context=context, referenceCountingObject=referenceCountingObject)

    # TODO: remove or merge with InstanceImpl.CreateNodeChecked()?
    # Same as CreateNode(), but if there is an error while creating the node, print a warning to stderr and return None, and if an error while setting a property occurs, print a warning to stderr and continue
    def CreateNodeChecked(self, properties, options={}):
        options = dict(options)
        # TODO: Continue on errors while setting properties
        try:
            return self.CreateNode(self, properties, options)
        except dbus.exceptions.DBusException as e:
            # TODO: More details in error message
            print('Error while creating node: ' + str(e), file=sys.stderr)
            return None

    # TODO: remove or merge with InstanceImpl.CreateObjectChecked()?
    # Same as CreateObject(), but if there is an error while creating the object, print a warning to stderr and return None, and if an error while setting a property occurs, print a warning to stderr and continue
    def CreateObjectChecked(self, properties, options={}):
        options = dict(options)
        # TODO: Continue on errors while setting properties
        try:
            return self.CreateObject(self, properties, options)
        except dbus.exceptions.DBusException as e:
            # TODO: More details in error message
            print('Error while creating object: ' + str(e), file=sys.stderr)
            return None


class ImporterImpl(voxie.DBusObject):
    def __init__(self, obj, interfaces, context=None, referenceCountingObject=None):
        voxie.DBusObject.__init__(
            self, obj, interfaces, context=context, referenceCountingObject=referenceCountingObject)

    # Returns a Data
    def Import(self, fileName, options={}):
        options = dict(options)
        with self.StartImport(fileName, options) as result:
            result.Operation.WaitFor()
            # TODO: This does not work because it does not grab a reference
            # return result.Data
            return refcountingcontext.grabReference(result.Data)

    # Returns a DataNode
    def ImportNode(self, fileName, options={}):
        options = dict(options)
        options['CreateNode'] = voxie.Variant('b', True)
        with self.StartImport(fileName, options) as result:
            result.Operation.WaitFor()
            return result.CastTo('de.uni_stuttgart.Voxie.OperationResultImportNode').DataNode

    # Returns a DataObject
    def ImportObject(self, fileName, options={}):
        options = dict(options)
        options['CreateNode'] = voxie.Variant('b', True)
        with self.StartImport(fileName, options) as result:
            result.Operation.WaitFor()
            return result.CastTo('de.uni_stuttgart.Voxie.OperationResultImportObject').DataObject


class OperationImpl(voxie.DBusObject):
    def __init__(self, obj, interfaces, context=None, referenceCountingObject=None):
        voxie.DBusObject.__init__(
            self, obj, interfaces, context=context, referenceCountingObject=referenceCountingObject)

    # Only works if there is a DBus main loop, but avoids problems with DBus timeouts
    def WaitForNoTimeout(self):
        if not hasattr(self._context, 'mainloop'):
            raise Exception(
                'Operation.WaitForNoTimeout only works for connections with enableService=True')
        if not hasattr(self._context, 'iteration'):
            raise Exception(
                'Operation.WaitForNoTimeout: Connection has an unknown main loop')
        done = [False]

        def handler(*args):
            done[0] = True
        signal = self.Finished(handler)
        try:
            if not self.IsFinished:
                while not done[0]:
                    # print('iteration')
                    self._context.iteration()
            # Get any errors
            self._getDBusMethod('WaitFor')({'Timeout': voxie.Variant('d', 0)})
        finally:
            signal.remove()

    # Automatically set the timeout for WaitFor
    def WaitFor(self, options={}, *, DBusObject_timeout=timeoutValue):
        return self._getDBusMethod('WaitFor')(options, DBusObject_timeout=DBusObject_timeout)

    # # Override WaitFor to avoid DBus timeout issues if there is a main loop
    # def WaitFor(self, options={}):
    #     options = dict(options)
    #     if len(options) != 0 or not hasattr(self._context, 'mainloop') or not hasattr(self._context, 'iteration'):
    #         # TODO: Handle options (e.g. Timeout)?
    #         self._getDBusMethod('WaitFor')(options)
    #     else:
    #         self.WaitForNoTimeout()


class NodeImpl(DynamicObjectImpl):
    def __init__(self, obj, interfaces, context=None, referenceCountingObject=None):
        DynamicObjectImpl.__init__(
            self, obj, interfaces, context=context, referenceCountingObject=referenceCountingObject)

    # Same as SetPropertiesChecked(), but if there is an error while loading the file, print a warning to stderr and return None
    # TODO: support continuing to set properties if there is an error with some property
    # TODO: Automatically look up property type if no voxie.Variant value is passed?
    def SetPropertiesChecked(self, values, options={}):
        options = dict(options)
        try:
            return self.SetProperties(values, options)
        except dbus.exceptions.DBusException as e:
            # TODO: More details in error message
            print('Error while setting properties: ' + str(e), file=sys.stderr)
            return None


class BufferImpl(DynamicObjectImpl):
    def __init__(self, obj, interfaces, context=None, referenceCountingObject=None):
        DynamicObjectImpl.__init__(
            self, obj, interfaces, context=context, referenceCountingObject=referenceCountingObject)
        self.__cacheBufferReadonly = None
        self.__cacheBufferWritable = None

    def GetBufferReadonly(self):
        if self.__cacheBufferReadonly is not None:
            return self.__cacheBufferReadonly
        self.__cacheBufferReadonly = Buffer(self.GetDataReadonly(), None, False)
        return self.__cacheBufferReadonly

    def GetBufferWritable(self, update=None):
        # Note: This is cached regardless of the 'update' value because currently the update value is ignored anyway
        # return Buffer(self.GetDataWritable(update), None, True)
        if self.__cacheBufferWritable is not None:
            return self.__cacheBufferWritable
        self.__cacheBufferWritable = Buffer(self.GetDataWritable(update), None, True)
        return self.__cacheBufferWritable

    def __array__(self, *args, **kwargs):
        return self.GetBufferWritable().__array__(*args, **kwargs)

    def __getitem__(self, *args, **kwargs):
        return self.GetBufferWritable().__getitem__(*args, **kwargs)

    def __setitem__(self, *args, **kwargs):
        return self.GetBufferWritable().__setitem__(*args, **kwargs)


class RawData2DRegularImpl(DynamicObjectImpl):
    def __init__(self, obj, interfaces, context=None, referenceCountingObject=None):
        DynamicObjectImpl.__init__(
            self, obj, interfaces, context=context, referenceCountingObject=referenceCountingObject)
        self.__cacheBufferReadonly = None
        self.__cacheBufferWritable = None

    def GetBufferReadonly(self):
        if self.__cacheBufferReadonly is not None:
            return self.__cacheBufferReadonly
        self.__cacheBufferReadonly = Buffer(self.GetDataReadonly(), 3, False)
        return self.__cacheBufferReadonly

    def GetBufferWritable(self, update):
        return Buffer(self.GetDataWritable(update), 3, True)
        # if self.__cacheBufferWritable is not None:
        #     return self.__cacheBufferWritable
        # self.__cacheBufferWritable = Buffer(self.GetDataWritable(), 3, True)
        # return self.__cacheBufferWritable

    def __array__(self, *args, **kwargs):
        return self.GetBufferReadonly().__array__(*args, **kwargs)

    def __getitem__(self, *args, **kwargs):
        return self.GetBufferReadonly().__getitem__(*args, **kwargs)

    def __setitem__(self, *args, **kwargs):
        return self.GetBufferReadonly().__setitem__(*args, **kwargs)


class VolumeDataVoxelImpl(DynamicObjectImpl):
    def __init__(self, obj, interfaces, context=None, referenceCountingObject=None):
        DynamicObjectImpl.__init__(
            self, obj, interfaces, context=context, referenceCountingObject=referenceCountingObject)
        self.__cacheBufferReadonly = None
        self.__cacheBufferWritable = None

    def GetBufferReadonly(self):
        if self.__cacheBufferReadonly is not None:
            return self.__cacheBufferReadonly
        self.__cacheBufferReadonly = Buffer(self.GetDataReadonly(), 3, False)
        return self.__cacheBufferReadonly

    def GetBufferWritable(self, update):
        return Buffer(self.GetDataWritable(update), 3, True)
        # if self.__cacheBufferWritable is not None:
        #     return self.__cacheBufferWritable
        # self.__cacheBufferWritable = Buffer(self.GetDataWritable(), 3, True)
        # return self.__cacheBufferWritable

    def __array__(self, *args, **kwargs):
        return self.GetBufferReadonly().__array__(*args, **kwargs)

    def __getitem__(self, *args, **kwargs):
        return self.GetBufferReadonly().__getitem__(*args, **kwargs)

    def __setitem__(self, *args, **kwargs):
        return self.GetBufferReadonly().__setitem__(*args, **kwargs)


class ImageDataPixelImpl(voxie.DBusObject):
    def __init__(self, obj, interfaces, context=None, referenceCountingObject=None):
        voxie.DBusObject.__init__(
            self, obj, interfaces, context=context, referenceCountingObject=referenceCountingObject)
        self.__cacheBufferReadonly = None
        self.__cacheBufferWritable = None

    def GetBufferReadonly(self):
        if self.__cacheBufferReadonly is not None:
            return self.__cacheBufferReadonly
        self.__cacheBufferReadonly = Buffer(self.GetDataReadonly(), 3, False)
        return self.__cacheBufferReadonly

    def GetBufferWritable(self, update):
        return Buffer(self.GetDataWritable(update), 3, True)
        # if self.__cacheBufferWritable is not None:
        #     return self.__cacheBufferWritable
        # self.__cacheBufferWritable = Buffer(self.GetDataWritable(), 3, True)
        # return self.__cacheBufferWritable

    def __array__(self, *args, **kwargs):
        return self.GetBufferReadonly().__array__(*args, **kwargs)

    def __getitem__(self, *args, **kwargs):
        return self.GetBufferReadonly().__getitem__(*args, **kwargs)

    def __setitem__(self, *args, **kwargs):
        return self.GetBufferReadonly().__setitem__(*args, **kwargs)

    def GetMatplotlibImage(self):
        array = self.__array__()

        # Voxie assumes that (0,0) is lower left and the first coordinate is the x coordinate, matplotlib and PIL assume that (0,0) is upper left and the first coordinate is the y coordinate
        array = array[:, ::-1].transpose([1, 0, 2])

        return array

    def GetPILImage(self):
        import PIL.Image

        array = self.GetMatplotlibImage()
        components = array.shape[2]
        type = array.dtype

        # TODO: More combinations?

        if components == 1 and type == np.dtype(np.uint8):
            # No conversion needed, mode will be 'L'
            return PIL.Image.fromarray(array[:, :, 0])
        if components == 3 and type == np.dtype(np.uint8):
            # No conversion needed, mode will be 'RGB'
            return PIL.Image.fromarray(array)
        if components == 4 and type == np.dtype(np.uint8):
            # No conversion needed, mode will be 'RGBA'
            return PIL.Image.fromarray(array)

        if components == 1 and type == np.dtype(np.uint16):
            # No conversion needed, mode will be 'I;16'
            return PIL.Image.fromarray(array[:, :, 0])

        if components == 1 and type == np.dtype(np.int32):
            # No conversion needed, mode will be 'I'
            return PIL.Image.fromarray(array[:, :, 0])

        if components == 1 and type == np.dtype(np.float32):
            # No conversion, return 32-bit float image ('F')
            return PIL.Image.fromarray(array[:, :, 0])

        if components == 3 and type == np.dtype(np.float32):
            # Return 8-bit RGB image
            return PIL.Image.fromarray(np.uint8(np.clip(np.round(array * 255), 0, 255)))
        if components == 4 and type == np.dtype(np.float32):
            # Return 8-bit RGBA image
            return PIL.Image.fromarray(np.uint8(np.clip(np.round(array * 255), 0, 255)))

        raise Exceptions(
            "Don't know how to create PIL image object with %d components and dtype %s" % (components, dtype))


class FileDataByteStreamImpl(DynamicObjectImpl):
    def __init__(self, obj, interfaces, context=None, referenceCountingObject=None):
        DynamicObjectImpl.__init__(self, obj, interfaces, context=context, referenceCountingObject=referenceCountingObject)
        self.__cacheBufferReadonly = None

    def GetBufferReadonly(self):
        if self.__cacheBufferReadonly is not None:
            return self.__cacheBufferReadonly
        self.__cacheBufferReadonly = Buffer(self.GetContentReadonly(), 1, False)
        return self.__cacheBufferReadonly

    def GetBufferWritable(self, update):
        return Buffer(self.GetContentWritable(update), 1, True)

    def __array__(self, *args, **kwargs):
        return self.GetBufferReadonly().__array__(*args, **kwargs)

    def __getitem__(self, *args, **kwargs):
        return self.GetBufferReadonly().__getitem__(*args, **kwargs)

    def __setitem__(self, *args, **kwargs):
        return self.GetBufferReadonly().__setitem__(*args, **kwargs)


class OperationCancelledException(Exception):
    voxieErrorName = 'de.uni_stuttgart.Voxie.OperationCancelled'

    def __str__(self):
        return 'The operation has been cancelled'

# Class which will catch exceptions in a 'with' block and send them over DBus to voxie


class ExternalOperationWrapper:
    def __init__(self, operation):
        self.__operation = operation

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        try:
            if type is not None:
                if hasattr(value, 'voxieErrorName'):
                    name = value.voxieErrorName
                else:
                    name = type.__name__
                # print(name, str(value))
                # Flush output stream before FinishError() so all messages are shown in error dialog
                sys.stdout.flush()
                sys.stderr.flush()
                self.__operation.FinishError(name, str(value))
                return False  # Show the error on the command line
                # return True
        finally:
            self.__operation.__exit__(type, value, traceback)
        return False

    def __dir__(self):
        return self.__operation.__dir__()

    def __getattribute__(self, name):
        if name != '_ExternalOperationWrapper__operation' and name != '__enter__' and name != '__exit__':
            return self.__operation.__getattribute__(name)
        return object.__getattribute__(self, name)

    def __setattr__(self, name, value):
        if name != '_ExternalOperationWrapper__operation':
            return self.__operation.__setattribute__(name, value)
        return object.__setattr__(self, name, value)


class ExternalOperationImpl(voxie.DBusObject):
    def __init__(self, obj, interfaces, context=None, referenceCountingObject=None):
        voxie.DBusObject.__init__(
            self, obj, interfaces, context=context, referenceCountingObject=referenceCountingObject)

    def ClaimOperationAndCatch(self):
        self.ClaimOperation()
        return ExternalOperationWrapper(self)

    def ThrowIfCancelled(self):
        if self.IsCancelled:  # TODO: Use signal to avoid roundtrip here?
            raise OperationCancelledException()

    def progress_wrapper(self, collection, min_progress=0, max_progress=1):
        length = len(collection)
        self.SetProgress(min_progress)
        self.ThrowIfCancelled()
        pos = 0
        for obj in collection:
            pos += 1
            if pos > length:
                raise Exception('pos > length')
            yield obj
            self.SetProgress(min_progress + (pos / length) * (max_progress - min_progress))
            self.ThrowIfCancelled()
        if pos != length:
            raise Exception('pos != length')


class ExternalOperationRunFilterImpl(ExternalOperationImpl):
    def __init__(self, obj, interfaces, context=None, referenceCountingObject=None):
        ExternalOperationImpl.__init__(
            self, obj, interfaces, context=context, referenceCountingObject=referenceCountingObject)
        self.__parameters = None
        self.__filterObject = None
        self.__properties = None
        self.__filterPrototypeName = None

    @property
    def FilterObjectCached(self):
        if self.__filterObject is None:
            self.__filterObject = self.FilterObject
        return self.__filterObject

    @property
    def ParametersCached(self):
        if self.__parameters is None:
            self.__parameters = self.Parameters
        return self.__parameters

    @property
    def Properties(self):
        if self.__properties is None:
            self.__properties = self.ParametersCached[self.FilterObjectCached._objectPath]['Properties'].getValue(
                'a{sv}')
        return self.__properties

    @property
    def FilterPrototypeName(self):
        if self.__filterPrototypeName is None:
            self.__filterPrototypeName = self.ParametersCached[self.FilterObjectCached._objectPath]['PrototypeName'].getValue(
                's')
        return self.__filterPrototypeName

    # Get the de.uni_stuttgart.Voxie.Data object for a given property name. Throws an exception if there is no such object and optional is False (the default), or returns None if optional is True.
    def GetInputData(self, propertyName, optional=False):
        inputPath = self.Properties[propertyName].getValue('o')
        if inputPath == dbus.ObjectPath('/'):
            if optional:
                return None
            raise Exception('Property ' + repr(propertyName) +
                            ' does not point to any object')
        inputDataPath = self.ParametersCached[inputPath]['Data'].getValue('o')
        if inputDataPath == dbus.ObjectPath('/'):
            return None
        inputData = self._context.makeObject(
            self._context.bus, self._context.busName, inputDataPath, ['de.uni_stuttgart.Voxie.Data'])
        return inputData

    # Create a new output data object or reuse the existing one
    def GetOutputVolumeDataVoxelLike(self, outputPath, inputData, type=None, volume_structure_type=None):
        inputData = refcountingcontext.castImplicit(inputData, 'de.uni_stuttgart.Voxie.VolumeData')

        is_block = False
        if 'de.uni_stuttgart.Voxie.VolumeDataVoxel' in inputData.SupportedInterfaces:
            inputData = refcountingcontext.cast(inputData, 'de.uni_stuttgart.Voxie.VolumeDataVoxel')
        elif 'de.uni_stuttgart.Voxie.VolumeDataBlock' in inputData.SupportedInterfaces:
            # inputData = refcountingcontext.cast(inputData, 'de.uni_stuttgart.Voxie.VolumeDataBlockJpeg')
            inputData = refcountingcontext.cast(inputData, 'de.uni_stuttgart.Voxie.VolumeDataBlock')
            # is_block = True
            # blockShape = inputData.BlockShape
        else:
            raise Exception('Unknown volume interface: ' + repr(inputData.SupportedInterfaces))

        inputOrigin = inputData.VolumeOrigin
        inputSpacing = inputData.GridSpacing
        inputSize = inputData.ArrayShape
        inputType = inputData.DataType
        if type is None:
            type = inputType

        outputDataPath = self.ParametersCached[outputPath]['Data'].getValue(
            'o')
        if outputDataPath != dbus.ObjectPath('/'):
            outputData = self._context.makeObject(self._context.bus, self._context.busName, outputDataPath, ['de.uni_stuttgart.Voxie.Data']).CastTo('de.uni_stuttgart.Voxie.VolumeData')
            # TODO: Also reuse VolumeDataBlock
            if outputData.IsInstance('de.uni_stuttgart.Voxie.VolumeDataVoxel') and not is_block:
                outputDataVoxel = outputData.CastTo('de.uni_stuttgart.Voxie.VolumeDataVoxel')
                outputOrigin = outputDataVoxel.VolumeOrigin
                outputSpacing = outputDataVoxel.GridSpacing
                outputSize = outputDataVoxel.ArrayShape
                outputType = outputDataVoxel.DataType

                if inputOrigin == outputOrigin and inputSpacing == outputSpacing and inputSize == outputSize and type == outputType:
                    # Reuse existing output data
                    # print ('Reusing data')
                    return outputDataVoxel
            # print ('Creating new data')

        if is_block:
            # TODO: Check whether input is really JPEG?
            return self._context.instance.CreateVolumeDataBlockJpeg(inputSize, blockShape, inputOrigin, inputSpacing, inputData.ValueOffset, inputData.ValueScalingFactor, inputData.SamplePrecision, inputData.HuffmanTableDC, inputData.HuffmanTableAC, inputData.QuantizationTable)
        else:
            return self._context.instance.CreateVolumeDataVoxel(inputSize, type, inputOrigin, inputSpacing)


class ExternalOperationRunSegmentationStepImpl(ExternalOperationImpl):
    def __init__(self, obj, interfaces, context=None, referenceCountingObject=None):
        ExternalOperationImpl.__init__(
            self, obj, interfaces, context=context, referenceCountingObject=referenceCountingObject)
        self.__parameters = None
        self.__SegmentationStepNode = None
        self.__properties = None
        self.__SegmentationStepPrototypeName = None
        self.__InputNodeCached = None
        self.__LabelDataCached = None
        self.__ContainerDataCached = None

    @property
    def SegmentationStepNodeCached(self):
        if self.__SegmentationStepNode is None:
            self.__SegmentationStepNode = self.SegmentationStepNode
        return self.__SegmentationStepNode

    @property
    def InputNodeCached(self):
        if self.__InputNodeCached is None:
            self.__InputNodeCached = self.InputNode
        return self.__InputNodeCached

    @property
    def LabelDataCached(self):
        if self.__LabelDataCached is None:
            self.__LabelDataCached = self.LabelData
        return self.__LabelDataCached

    @property
    def ContainerDataCached(self):
        if self.__ContainerDataCached is None:
            self.__ContainerDataCached = self.ContainerData
        return self.__ContainerDataCached

    @property
    def ParametersCached(self):
        if self.__parameters is None:
            self.__parameters = self.Parameters
        return self.__parameters

    @property
    def Properties(self):
        if self.__properties is None:
            self.__properties = self.ParametersCached[self.SegmentationStepNodeCached._objectPath]['Properties'].getValue(
                'a{sv}')
        return self.__properties

    @property
    def SegmentationStepPrototypeName(self):
        if self.__SegmentationStepPrototypeName is None:
            self.__SegmentationStepPrototypeName = self.ParametersCached[self.SegmentationStepNodeCached._objectPath]['PrototypeName'].getValue(
                's')
        return self.__SegmentationStepPrototypeName

    # Get the de.uni_stuttgart.Voxie.Data object for a given property name. Throws an exception if there is no such object and optional is False (the default), or returns None if optional is True.
    def GetInputData(self, propertyName, optional=False):
        inputPath = self.Properties[propertyName].getValue('o')
        if inputPath == dbus.ObjectPath('/'):
            if optional:
                return None
            raise Exception('Property ' + repr(propertyName) +
                            ' does not point to any object')
        inputDataPath = self.ParametersCached[inputPath]['Data'].getValue('o')
        if inputDataPath == dbus.ObjectPath('/'):
            return None
        inputData = self._context.makeObject(
            self._context.bus, self._context.busName, inputDataPath, ['de.uni_stuttgart.Voxie.Data'])
        return inputData

    def GetInputDataFallback(self, propertyName, optional=False):
        inputPath = self.Properties[propertyName].getValue('o')
        if inputPath == dbus.ObjectPath('/'):
            inputPath = self.InputNodeCached
        if inputPath == dbus.ObjectPath('/'):
            if optional:
                return None
            raise Exception('Property ' + repr(propertyName) +
                            ' does not point to any object and there is no SegmentationFilter input object')
        inputDataPath = self.ParametersCached[inputPath]['Data'].getValue('o')
        if inputDataPath == dbus.ObjectPath('/'):
            return None
        inputData = self._context.makeObject(
            self._context.bus, self._context.busName, inputDataPath, ['de.uni_stuttgart.Voxie.Data'])
        return inputData


class VoxieContext(RefCountingContext):
    # TODO: Rename enableService to enableMainLoop
    def __init__(self, args, *, mainloop=None, enableService=False):
        try:
            import voxie_full.instance
            self.getImplClassIntf2 = voxie_full.instance.getImplClassIntf
        except ImportError:
            self.getImplClassIntf2 = None

        path = os.path.dirname(sys.modules[VoxieContext.__module__].__file__)
        if not os.path.exists(os.path.join(path, 'de.uni_stuttgart.Voxie.xml')) and os.path.exists(os.path.join(path, '..', '..', 'de.uni_stuttgart.Voxie.xml')):
            docVoxie = xml.etree.ElementTree.parse(os.path.join(
                path, '..', '..', 'de.uni_stuttgart.Voxie.xml'))
            docVoxieAdd = glob.glob(glob.escape(os.path.join(path, '..', '..', 'intern')) + '/*/de.uni_stuttgart.Voxie.*.xml')
        else:
            docVoxie = xml.etree.ElementTree.parse(
                os.path.join(path, 'de.uni_stuttgart.Voxie.xml'))
            docVoxieAdd = glob.glob(glob.escape(os.path.join(path, 'de.uni_stuttgart.Voxie')) + '.*.xml')
        interfaces = []
        for f in ['org.freedesktop.DBus.Introspectable.xml', 'org.freedesktop.DBus.Properties.xml', 'org.freedesktop.DBus.xml']:
            docDBus = xml.etree.ElementTree.parse(os.path.join(path, f))
            interfaces += docDBus.getroot()
        interfaces += docVoxie.getroot()
        for f in docVoxieAdd:
            docDBus = xml.etree.ElementTree.parse(os.path.join(path, f))
            interfaces += docDBus.getroot()
        self.contextSimple = voxie.DBusObjectContext(interfaces=interfaces)

        # if args is None:
        #    args = voxie.parse.parser.parse_args()

        busName = args.voxie_bus_name
        if args.voxie_bus_address is not None:
            def busFunction(mainloop):
                return dbus.bus.BusConnection(
                    args.voxie_bus_address, mainloop=mainloop)
            if busName == '':
                raise Exception('Bus name is empty')
        elif args.voxie_peer_address is not None:
            def busFunction(mainloop):
                return dbus.connection.Connection(
                    args.voxie_peer_address, mainloop=mainloop)
            busName = ''
        else:
            def busFunction(mainloop):
                return dbus.SessionBus(
                    mainloop=mainloop)
            if busName == '':
                raise Exception('Bus name is empty')

        # RefCountingContext.__init__(self, busFunction, busName, interfaces = interfaces, mainloop = mainloop, enableService = enableService, clientManagerPath = '/de/uni_stuttgart/Voxie', clientManagerInterface = 'de.uni_stuttgart.Voxie.Instance')
        RefCountingContext.__init__(
            self, busFunction, busName, interfaces=interfaces, mainloop=mainloop, enableService=enableService)

    def __isJSONAsDBusVariant(self, dbusType, xmlElement):
        if not (dbusType == 'a{sv}' or dbusType == 'v'):
            return False
        if xmlElement is None:
            return False
        isJSONAsDBusVariant = False
        for elem in xmlElement:
            if elem.tag != 'annotation':
                continue
            name = elem.attrib['name']
            if name != 'de.uni_stuttgart.Voxie.IsJSONAsDBusVariant':
                continue
            val = elem.attrib['value']
            if val == 'false':
                isJSONAsDBusVariant = False
            elif val == 'true':
                isJSONAsDBusVariant = True
            else:
                raise Exception(
                    'Invalid value for annotation de.uni_stuttgart.Voxie.IsJSONAsDBusVariant: ' + repr(val))
        return isJSONAsDBusVariant

    def getConverterToDBus(self, *, dbusType, dbusObject, dbusObjectInfo, xmlElement, variantLevel):
        if not self.__isJSONAsDBusVariant(dbusType=dbusType, xmlElement=xmlElement):
            return RefCountingContext.getConverterToDBus(self, dbusType=dbusType, dbusObject=dbusObject, dbusObjectInfo=dbusObjectInfo, xmlElement=xmlElement, variantLevel=variantLevel)

        # Get the original converter (set xmlElement to None to avoid recursion)
        origConverter = voxie.dbusobject.get_to_dbus_cast(
            dbusType, context=self, dbusObject=dbusObject, dbusObjectInfo=dbusObjectInfo, xmlElement=None, variant_level=variantLevel)

        def convert(value, callContext):
            if dbusType == 'v':
                value = voxie.json_dbus.json_to_dbus(value)
            else:
                value = voxie.json_dbus.json_to_dbus_dict(value)
            value = origConverter(value, callContext)
            return value
        return convert

    def getConverterFromDBus(self, dbusType, dbusObject, dbusObjectInfo, xmlElement):
        if not self.__isJSONAsDBusVariant(dbusType=dbusType, xmlElement=xmlElement):
            return RefCountingContext.getConverterFromDBus(self, dbusType, dbusObject, dbusObjectInfo, xmlElement)

        # Get the original converter (set xmlElement to None to avoid recursion)
        origConverter = voxie.dbusobject.get_from_dbus_cast(
            dbusType, context=self, dbusObject=dbusObject, dbusObjectInfo=dbusObjectInfo, xmlElement=None)

        def convert(value, callContext):
            value = origConverter(value, callContext)
            if dbusType == 'v':
                value = voxie.json_dbus.dbus_to_json(value)
            else:
                value = voxie.json_dbus.dbus_to_json_dict(value)
            return value
        return convert

    def getImplClass(self, interfaces):
        for interface in interfaces:
            if interface == 'de.uni_stuttgart.Voxie.Instance':
                return InstanceImpl
            elif interface == 'de.uni_stuttgart.Voxie.DynamicObject':
                return DynamicObjectImpl
            elif interface == 'de.uni_stuttgart.Voxie.ComponentContainer':
                return ComponentContainerImpl
            elif interface == 'de.uni_stuttgart.Voxie.Node' or interface == 'de.uni_stuttgart.Voxie.Object':
                return NodeImpl
            elif interface == 'de.uni_stuttgart.Voxie.NodePrototype' or interface == 'de.uni_stuttgart.Voxie.ObjectPrototype':
                return NodePrototypeImpl
            elif interface == 'de.uni_stuttgart.Voxie.Operation':
                return OperationImpl
            elif interface == 'de.uni_stuttgart.Voxie.Importer':
                return ImporterImpl
            elif interface == 'de.uni_stuttgart.Voxie.Buffer':
                return BufferImpl
            elif interface == 'de.uni_stuttgart.Voxie.TomographyRawData2DRegular':
                return RawData2DRegularImpl
            elif interface == 'de.uni_stuttgart.Voxie.VolumeDataVoxel':
                return VolumeDataVoxelImpl
            elif interface == 'de.uni_stuttgart.Voxie.ImageDataPixel':
                return ImageDataPixelImpl
            elif interface == 'de.uni_stuttgart.Voxie.FileDataByteStream':
                return FileDataByteStreamImpl
            elif interface == 'de.uni_stuttgart.Voxie.ExternalOperation':
                return ExternalOperationImpl
            elif interface == 'de.uni_stuttgart.Voxie.ExternalOperationRunFilter':
                return ExternalOperationRunFilterImpl
            elif interface == 'de.uni_stuttgart.Voxie.ExternalOperationRunSegmentationStep':
                return ExternalOperationRunSegmentationStepImpl

            if self.getImplClassIntf2 is not None:
                implVal = self.getImplClassIntf2(interface)
                if implVal is not None:
                    return implVal
        return RefCountingContext.getImplClass(self, interfaces)

    def createInstance(self):
        return self.makeObject(self.bus, self.busName, '/de/uni_stuttgart/Voxie', ['de.uni_stuttgart.Voxie.Instance'])

    # TODO: Should createInstance() be replaced by this?
    # TODO: cache the returned object. TODO: How would such caching affect deterministic (GC-free) cleanup?
    @property
    def instance(self):
        return self.createInstance()
