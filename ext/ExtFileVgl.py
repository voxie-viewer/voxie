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

import voxie
import dbus
import sys
import codecs
import os
import gzip
import io
import abc

import xml.etree.ElementTree

import numpy as np

data_types = {
    "Int8": ('int', 8, 'none'),
    "Int16": ('int', 16, 'little'),
    "Int32": ('int', 32, 'little'),
    "Int64": ('int', 64, 'little'),
    "UInt8": ('uint', 8, 'none'),
    "UInt16": ('uint', 16, 'little'),
    "UInt32": ('uint', 32, 'little'),
    "UInt64": ('uint', 64, 'little'),
    "Float": ('float', 32, 'little'),
    "Double": ('float', 64, 'little'),
}


def expect_element(element, name):
    if type(element) != xml.etree.ElementTree.Element:
        raise Exception('type(element) != xml.etree.ElementTree.Element')
    if element.tag != name:
        raise Exception('Expected {!r} element, got {!r} element'.format(name, element.tag))


def expect_child_count(element, count):
    if type(element) != xml.etree.ElementTree.Element:
        raise Exception('type(element) != xml.etree.ElementTree.Element')
    if len(element) != count:
        raise Exception('Expected {!r} children, got {!r}'.format(count, len(element)))


def get_attribute_opt(element, name):
    if type(element) != xml.etree.ElementTree.Element:
        raise Exception('type(element) != xml.etree.ElementTree.Element')
    if name in element.attrib:
        return element.attrib[name]
    else:
        return None


def get_attribute(element, name):
    value = get_attribute_opt(element, name)
    if value is None:
        raise Exception('Did not find {!r} attribute on element {!r}'.format(name, element.tag))
    return value


def get_attribute_bool(element, name):
    value = get_attribute(element, name)
    if value == 'true':
        return True
    elif value == 'false':
        return False
    else:
        raise Exception('Got invalid bool value: {!r}'.format(value))


def get_inner_text(element):
    if type(element) != xml.etree.ElementTree.Element:
        raise Exception('type(element) != xml.etree.ElementTree.Element')
    return ''.join(element.itertext())


def get_inner_text_trimmed(element):
    return get_inner_text(element).strip()


class VglUnit:
    def __init__(self, element):
        expect_element(element, 'unit')
        expect_child_count(element, 0)

        self.quantity = get_attribute(element, 'quantity')
        self.name = get_attribute(element, 'name')
        self.abbreviation = get_attribute(element, 'abbreviation')
        self.factor = float(get_attribute(element, 'factor'))
        isInternalUnit_value = int(get_attribute(element, 'isInternalUnit'))
        if isInternalUnit_value == 0:
            self.is_internal_unit = False
        elif isInternalUnit_value == 1:
            self.is_internal_unit = True
        else:
            raise Exception('Unknown isInternalUnit_value: {!r}'.format(isInternalUnit_value))


class VglObjectRegistry:
    def __init__(self):
        self.objects = {}
        self.to_resolve = {}

    def add(self, obj):
        id = obj.id

        if id in self.objects:
            raise Exception('Got object {!r} twice'.format(id))
        self.objects[id] = obj

        if id in self.to_resolve:
            for oref in self.to_resolve[id]:
                if oref.objects is not None:
                    raise Exception('oref.objects is not None')
                oref.objects = obj
            del self.to_resolve[id]

    def resolve(self, id, oref):
        if oref.object is not None:
            raise Exception('oref.object is not None')

        if id in self.objects:
            oref.object = self.objects[id]
            return

        if id not in self.to_resolve:
            self.to_resolve[id] = []
        self.to_resolve[id] += [oref]

    def assert_done(self):
        if len(self.to_resolve) == 0:
            return

        raise Exception('Unresolved object IDs: ' + ', '.join(self.to_resolve))


class VglProperty:
    def __init__(self, registry, element):
        expect_element(element, "property")

        self.type = get_attribute(element, "type")
        self.name = get_attribute(element, "name")
        self.min_index = get_attribute_opt(element, "minIndex")
        self.max_index = get_attribute_opt(element, "maxIndex")
        self.enabled = get_attribute_bool(element, "enabled")
        self.locked = get_attribute_bool(element, "locked")
        self.context = get_attribute_opt(element, "context")

        if self.type == "objectlink":
            expect_child_count(element, 1)
            link = VglObjectLink.get(registry, element[0])
            # if link != null && link.Enum != Name && link.Enum != "Unknown":
            # throw Exception ("link != null && link.Enum != Name && link.Enum != \"Unknown\": '" + link.Enum + "' != '" + Name + "'")
            self.value = link
        elif self.type == "objectlinkarray":
            children = list([VglObjectLink.get(registry, child) for child in element])
            self.value = VglObjectLinkArray(children)
        elif self.type == "string":
            expect_child_count(element, 1)
            self.value = VglString(element[0])
        elif self.type == "typeinfo":
            expect_child_count(element, 1)
            self.value = VglTypeInfo(element[0])
        elif self.type == "vector3":
            expect_child_count(element, 1)
            self.value = VglVector3(element[0])
        elif self.type == "vector4":
            expect_child_count(element, 1)
            self.value = VglVector4(element[0])
        elif self.type == "matrix4":
            expect_child_count(element, 1)
            self.value = VglMatrix4(element[0])
        else:
            # TODO
            # print (self.type)
            self.value = VglNotImplemented()

    @property
    def full_name(self):
        if self.context is not None:
            return self.context + '.' + self.name
        else:
            return self.name

    def as_val(self, T):
        if not isinstance(self.value, T):
            raise Exception('Expected a {!r} for property {!r}, got a {!r}'.format(T, self.name, type(self.value)))
        return self.value

    @property
    def ObjectLink(self):
        return self.as_val(VglObjectLink)

    @property
    def ObjectLinkArray(self):
        return self.as_val(VglObjectLinkArray)

    @property
    def ObjectLinkValue(self):
        if len(self.ObjectLink.values) != 1:
            raise Exception('len(self.ObjectLink.values) != 1')
        return self.ObjectLink[0]

    @property
    def Object(self):
        return self.ObjectLinkValue.object

    @property
    def String(self):
        return self.as_val(VglString).value

    @property
    def TypeInfo(self):
        return self.as_val(VglTypeInfo).value

    @property
    def Vector3(self):
        return self.as_val(VglVector3).values

    @property
    def Vector4(self):
        return self.as_val(VglVector4).values

    @property
    def Matrix4(self):
        return self.as_val(VglMatrix4).values


class VglValue(abc.ABC):
    pass


class VglNotImplemented(VglValue):
    pass


class VglString(VglValue):
    def __init__(self, element):
        expect_element(element, 'string')
        expect_child_count(element, 0)
        self.value = get_inner_text(element)


class VglTypeInfo(VglValue):
    def __init__(self, element):
        expect_element(element, 'typeinfo')
        expect_child_count(element, 0)
        self.value = get_inner_text(element)


class VglVector3(VglValue):
    def __init__(self, element):
        expect_element(element, 'vector3')
        expect_child_count(element, 0)
        values = get_inner_text(element).split()
        if len(values) != 3:
            raise Exception("len(values) != 3")
        self.values = list([float(val) for val in values])


class VglVector4(VglValue):
    def __init__(self, element):
        expect_element(element, 'vector4')
        expect_child_count(element, 0)
        values = get_inner_text(element).split()
        if len(values) != 4:
            raise Exception("len(values) != 4")
        self.values = list([float(val) for val in values])


class VglMatrix4(VglValue):
    def __init__(self, element):
        expect_element(element, 'matrix4')
        expect_child_count(element, 0)
        values = get_inner_text(element).split()
        if len(values) > 16:  # TODO: <matrix4> 1 0 0 0  0 1 0 0  0 0 1 0  -50.047693 45.599009 50.047693_X -50.047693 45.599009 50.047693_Y -50.047693 45.599009 50.047693_Z 1 </matrix4>
            self.values = None
            return
        if len(values) != 16:
            raise Exception("len(values) != 16")
        self.values = list([float(val) for val in values])


class VglObjectRef:
    def __init__(self):
        self.object = None

    @staticmethod
    def get(registry, element):
        if element.tag == 'objectref':
            id = get_attribute_opt(element, 'id')
            oref = VglObjectRef()
            registry.resolve(id, oref)
            return oref
        else:
            oref = VglObjectRef()
            oref.object = VglObject(registry, element)
            return oref


class VglObjectLinkValue:
    @property
    def object(self):
        if self.object_ref is None:
            return None
        obj = self.object_ref.object
        if obj is None:
            raise Exception('obj is None')
        return obj

    @property
    def object2(self):
        if self.object2_ref is None:
            return None
        obj = self.object2_ref.object
        if obj is None:
            raise Exception('obj is None')
        return obj

    @staticmethod
    def get(registry, element, offset):
        expect_element(element, 'objectlink')

        link = VglObjectLinkValue()
        link.object_ref = VglObjectRef.get(registry, element[offset + 0])
        enum1 = element[offset + 1]
        expect_element(enum1, "enum")
        expect_child_count(enum1, 0)
        link.Object2Ref = VglObjectRef.get(registry, element[offset + 2])
        enum2 = element[offset + 3]
        expect_element(enum2, "enum")
        expect_child_count(enum2, 0)

        link.enum = get_inner_text_trimmed(enum1)
        link.enum2 = get_inner_text_trimmed(enum2)

        if link.object_ref is None:
            raise Exception('link.object_ref is None')
        if link.enum2 != 'Unknown':
            raise Exception("link.enum2 != 'Unknown'")

        return link


class VglObjectLink(VglValue):
    @staticmethod
    def get(registry, element):
        expect_element(element, 'objectlink')

        if len(element) == 0:
            return None
        else:
            count = len(element) // 4
            expect_child_count(element, count * 4)

            link = VglObjectLink()
            link.values = list([VglObjectLinkValue.get(registry, element, i * 4) for i in range(count)])
            return link

    def __getitem__(self, name):
        return self.values.__getitem__(name)


class VglObjectLinkArray(VglValue):
    def __init__(self, links):
        self.links = links


class VglObject:
    def __init__(self, registry, element):
        expect_element(element, 'object')
        self.id = get_attribute_opt(element, 'id')
        self.cls = get_attribute_opt(element, 'class')

        if self.id is not None:
            registry.add(self)

        props = {}
        for node in element:
            if node.tag != 'compatibility' and node.tag != 'compatibility_2':
                property = VglProperty(registry, node)
                props[property.full_name] = property
        self.properties = props

    def __getitem__(self, name):
        return self.properties.__getitem__(name)


class VglFile:
    def __init__(self, root_element):
        expect_element(root_element, 'Project')
        expect_child_count(root_element, 3)

        version_elem = root_element[0]
        expect_element(version_elem, 'version')
        # expect_child_count(version_elem, 0)
        self.version = get_attribute(version_elem, 'identifier')

        units_elem = root_element[1]
        expect_element(units_elem, 'units')
        self.units = list([VglUnit(child) for child in units_elem])

        registry = VglObjectRegistry()

        vgl_elem = root_element[2]
        expect_element(vgl_elem, 'vgl')
        self.vgl = list([VglObject(registry, child) for child in vgl_elem])

        registry.assert_done()

        if len(self.vgl) == 0:
            raise Exception('len(self.vgl) == 0')

        self.unit_by_quantity = {}
        for unit in self.units:
            self.unit_by_quantity[unit.quantity] = unit


class VolumeRenderObject:
    def __init__(self, obj):
        if obj.cls != 'VGLVolumeRenderObject':
            raise Exception("obj.cls != 'VGLVolumeRenderObject'")
        self.obj = obj

        # print(obj.dump())
        sample_grid = obj['SampleGrid'].Object
        # print(sample_grid.dump())
        if False:
            type = sample_grid["SampleDataType"].TypeInfo
            size = sample_grid["GridSize"].Vector4
            if size[3] != 1:
                raise Exception('size[3] != 1')
        self.sampling_distance = sample_grid["SamplingDistance"].Vector3

        import_settings = sample_grid["ImportSettings"].Object
        file_io_list = import_settings["FileIOList"].ObjectLinkArray
        if len(file_io_list.links) != 1:
            raise Exception('len(file_io_list.links) != 1')
        import_fhg = file_io_list.links[0][0].object
        self.raw_file_name_full = import_fhg["FileName"].String
        raw_file_name_parts = self.raw_file_name_full.split('/')
        self.raw_file_name = raw_file_name_parts[-1]
        import_settings_file_info = import_fhg["ImportSettingsFileInfo"].Object
        # print(import_settings_file_info.dump())
        self.type = import_settings_file_info["SampleDataType"].TypeInfo
        size_v = import_settings_file_info["GridSize"].Vector4
        if size_v[3] != 1:
            raise Exception("size_v[3] != 1")
        self.size = [int(size_v[0]), int(size_v[1]), int(size_v[2])]
        for i in range(3):
            if size_v[i] != self.size[i]:
                raise Exception("size_v[i] != self.size[i]")
        # print(import_fhg["ReadGrid"].Object.dump())

    @property
    def name(self):
        return obj['Name'].String

    def get_full_raw_file_name(self, vgl_file_name):
        return os.path.join(os.path.dirname(vgl_file_name), self.raw_file_name)

    @property
    def overall_size(self):
        size = 1
        for dim in self.size:
            size *= dim

        if self.type in data_types:
            element_size_bits = data_types[self.type][1]
            if element_size_bits % 8 != 0:
                raise Exception('element_size_bits % 8 != 0')
            element_size = element_size_bits // 8
        else:
            raise Exception('Unknown type: {!r}'.format(self.type))

        return size * element_size

    @staticmethod
    def get_render_objects(vgl):
        if len(vgl.vgl) != 1:
            raise Exception('len(vgl.vgl) != 1')
        if vgl.vgl[0].cls == 'VGLVolumeRenderObject':
            return [VolumeRenderObject(vgl.vgl[0])]
        else:
            return Scene.get(vgl).volume_render_object_list


class Scene:
    def __init__(self, obj):
        if obj.cls != 'VGLScene':
            raise Exception("obj.cls != 'VGLScene'")
        self.obj = obj

        self.volume_render_object_list = self.get_list('VolumeRenderObjectList', VolumeRenderObject)
        # TODO
        # self.volume_render_object_list_transform = self.get_list('VolumeRenderObjectList', Transform, get_second_object=True)
        # print(obj.dump())
        # print(obj.properties)

    def get_list(self, name, ctor, get_second_object=False):
        lst = []
        for link in self.obj[name].ObjectLinkArray.links:
            link_value = link.values[0]
            if get_second_object:
                o = link_value.object2
            else:
                o = link_value.object
            lst.append(ctor(o))
        return lst

    @staticmethod
    def get(vgl):
        if len(vgl.vgl) != 1:
            raise Exception('len(vgl.vgl) != 1')
        # Nikon CT Pro 3D 4.3.1 produces VGL files where the VGLScene object is the root object
        if vgl.vgl[0].cls == 'VGLScene':
            return Scene(vgl.vgl[0])
        else:
            return Scene(vgl.vgl[0]['Scene'].Object)


args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

if args.voxie_action != 'Import':
    raise Exception('Invalid operation: ' + args.voxie_action)

with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationImport']).ClaimOperationAndCatch() as op:
    filename = op.Filename

    with open(filename, 'rb') as file:
        file_d = gzip.open(file)
        doc = xml.etree.ElementTree.parse(file_d)

    # with open('/tmp/data.xml', 'wb') as file:
    #     doc.write(file)
    vgl = VglFile(doc.getroot())
    vros = VolumeRenderObject.get_render_objects(vgl)
    if len(vros) != 1:
        raise Exception('File does not contain exactly one VolumeRenderObject')
    vro = vros[0]
    raw = vro.get_full_raw_file_name(filename)
    overall_size = vro.overall_size
    print(raw, overall_size, vro.type, vro.size, vro.sampling_distance)

    dataType = data_types[vro.type]
    arrayShape = vro.size
    gridSpacing = list(np.asarray(vro.sampling_distance) * vgl.unit_by_quantity['Length'].factor)
    volumeOrigin = list(-np.asarray(arrayShape) * gridSpacing / 2)

    with open(raw, 'rb') as raw_file:
        raw_len = raw_file.seek(0, 2)
        offset = raw_len - overall_size
        if offset != 0 and offset != 2048:
            raise Exception('offset != 0 and offset != 2048')
        raw_file.seek(offset)
        with instance.CreateVolumeDataVoxel(arrayShape, dataType, volumeOrigin, gridSpacing) as resultData:
            with resultData.CreateUpdate() as update, resultData.GetBufferWritable(update) as buffer:
                outData = buffer[()]
                dt = outData.dtype  # TODO
                op.ThrowIfCancelled()
                for z in range(arrayShape[2]):
                    # TODO: Avoid memory allocations here?
                    data = np.fromfile(raw_file, dtype=dt, count=arrayShape[0] * arrayShape[1])
                    data = data.reshape(arrayShape[0], arrayShape[1]).transpose()
                    outData[:, :, z] = data

                    op.ThrowIfCancelled()
                    op.SetProgress((z + 1) / arrayShape[2])

                version = update.Finish()
            with version:
                op.Finish(resultData, version)

context.client.destroy()
