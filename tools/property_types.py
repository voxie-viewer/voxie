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

# Mapping of the DBus signature to the corresponding Qt type (must be bijective because QtDBus cannot manage multiple Qt types which are mapped to the same DBus signature, or multiple DBus signatures mapped to the same Qt type either)
# TODO: This probably can be automatically generated now (see dbusToCppRawType())
dbusToQtType = {
    '(bs)': 'std::tuple<bool, QString>',
    '(dd)': 'vx::TupleVector<double, 2>',
    '(ddd)': 'vx::TupleVector<double, 3>',
    '((ddd)(ddd))': 'vx::TupleVector<vx::TupleVector<double, 3>, 2>',
    '(dddd)': 'vx::TupleVector<double, 4>',
    '((ddd)(dddd))': 'std::tuple<vx::TupleVector<double, 3>, vx::TupleVector<double, 4>>',
    '(tt)': 'vx::TupleVector<quint64, 2>',
    '(ttt)': 'vx::TupleVector<quint64, 3>',
    'ai': 'QList<int>',
    '(st)': 'std::tuple<QString, quint64>',
    'a{ot}': 'QMap<QDBusObjectPath, quint64>',
    'a{sv}': 'QMap<QString, QDBusVariant>',
    'a{sg}': 'QMap<QString, QDBusSignature>',
    'aa{sv}': 'QList<QMap<QString, QDBusVariant>>',
    # '(so)': 'vx::Tuple_String_ObjectPath',
    '(so)': 'std::tuple<QString, QDBusObjectPath>',
    '(sus)': 'std::tuple<QString, quint32, QString>',
    # '(a{sv}x(sus)(t)(x)a{sv})': 'vx::Array1Info',
    # '(a{sv}x(sus)(tt)(xx)a{sv})': 'vx::Array2Info',
    # '(a{sv}x(sus)(ttt)(xxx)a{sv})': 'vx::Array3Info',
    '(a{sv}x(sus)(t)(x)a{sv})': 'std::tuple<QMap<QString, QDBusVariant>, qint64, std::tuple<QString, quint32, QString>, std::tuple<quint64>, std::tuple<qint64>, QMap<QString, QDBusVariant>>',
    '(a{sv}x(sus)(tt)(xx)a{sv})': 'std::tuple<QMap<QString, QDBusVariant>, qint64, std::tuple<QString, quint32, QString>, std::tuple<quint64, quint64>, std::tuple<qint64, qint64>, QMap<QString, QDBusVariant>>',
    '(a{sv}x(sus)(ttt)(xxx)a{sv})': 'std::tuple<QMap<QString, QDBusVariant>, qint64, std::tuple<QString, quint32, QString>, std::tuple<quint64, quint64, quint64>, std::tuple<qint64, qint64, qint64>, QMap<QString, QDBusVariant>>',
    'a{sa{sv}}': 'QMap<QString, QMap<QString, QDBusVariant>>',
    'a{oa{sv}}': 'QMap<QDBusObjectPath, QMap<QString, QDBusVariant>>',
    'av': 'QList<QDBusVariant>',  # Seems to be ignored
    'a(tav)': 'QList<std::tuple<quint64, QList<QDBusVariant>>>',
    'a(tosa{sv}a{sv})': 'QList<std::tuple<quint64, QDBusObjectPath, QString, QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>',
    'a(sosa{sv}a{sv})': 'QList<std::tuple<QString, QDBusObjectPath, QString, QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>',
    'a(sst(sus)sa{sv}a{sv})': 'QList<std::tuple<QString, QString, quint64, std::tuple<QString, quint32, QString>, QString, QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>',
    'a(s(sus)sa{sv}a{sv})': 'QList<std::tuple<QString, std::tuple<QString, quint32, QString>, QString, QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>',
}

# Same as dbusToQtType but with QtDBus builtin types
dbusToQtTypeAll = {
    **dbusToQtType,
    'd': 'double',
    'x': 'qlonglong',
    's': 'QString',
    'b': 'bool',
    'o': 'QDBusObjectPath',
    'ao': 'QList<QDBusObjectPath>',
}


def dbusToCppRawType(sig):
    sig = dbus.Signature(sig)
    if len(list(sig)) != 1:
        raise Exception('Expected a single complete type')
    if sig[0] == 'y':
        return 'quint8'
    if sig[0] == 'b':
        return 'bool'
    if sig[0] == 'n':
        return 'qint16'
    if sig[0] == 'q':
        return 'quint16'
    if sig[0] == 'i':
        return 'qint32'
    if sig[0] == 'u':
        return 'quint32'
    if sig[0] == 'x':
        return 'qint64'
    if sig[0] == 't':
        return 'quint64'
    if sig[0] == 'd':
        return 'double'
    if sig[0] == 'h':
        return 'QDBusUnixFileDescriptor'
    if sig[0] == 's':
        return 'QString'
    if sig[0] == 'o':
        return 'QDBusObjectPath'
    if sig[0] == 'g':
        return 'QDBusSignature'
    if sig[0] == 'v':
        return 'QDBusVariant'
    if sig[0] == 'ay':
        return 'QByteArray'
    if sig[0] == 'as':
        return 'QStringList'
    if sig[0] == '(':
        elems = list(dbus.Signature(sig[1:-1]))
        res = 'std::tuple<'
        isAllSame = True
        for i in range(len(elems)):
            if i != 0:
                res += ', '
                if elems[i] != elems[0]:
                    isAllSame = False
            res += dbusToCppRawType(elems[i])
        res += '>'
        if isAllSame and len(elems) > 1 and False:
            return 'vx::TupleVector<%s, %d>' % (dbusToCppRawType(elems[0]), len(elems))
        return res
    if sig[0:2] == 'a{' and sig[-1] == '}':
        elems = list(dbus.Signature(sig[2:-1]))
        if len(elems) != 2:
            raise Exception('Expected exactly two complete types in map entry')
        return 'QMap<' + dbusToCppRawType(elems[0]) + ', ' + dbusToCppRawType(elems[1]) + '>'
    if sig[0] == 'a':
        return 'QList<' + dbusToCppRawType(sig[1:]) + '>'
    raise Exception('Unknown DBus signature: ' + repr(sig))


def dbusGetComponentTypes(sig):
    sig = dbus.Signature(sig)
    if len(list(sig)) != 1:
        raise Exception('Expected a single complete type')
    simple = ['y', 'b', 'n', 'q', 'i', 'u',
              'x', 't', 'd', 'h', 's', 'o', 'g', 'v']
    # TODO: Add ay and as?
    if sig in simple:
        return []
    if sig[0] == '(':
        return list(dbus.Signature(sig[1:-1]))
    if sig[0:2] == 'a{' and sig[-1] == '}':
        elems = list(dbus.Signature(sig[2:-1]))
        if len(elems) != 2:
            raise Exception('Expected exactly two complete types in map entry')
        return elems
    if sig[0] == 'a':
        return [sig[1:]]
    raise Exception('Unknown DBus signature: ' + repr(sig))


types = {
    'de.uni_stuttgart.Voxie.PropertyType.Float': {  # Name
        # 'ShortName': 'Float', # Default to suffix of Name
        'DisplayName': 'Floating pointer number',
        'DBusSignature': 'd',
        # 'QtType': 'double', # Qt type which represents the property normally. Defaults to dbusToQtTypeAll[DBusSignature]
        # 'RawType: 'double', # Qt type which represents the property internally. Defaults to dbusToQtTypeAll[DBusSignature]. Should normally not be needed.
        # Must be of type dbusToQtTypeAll[DBusSignature]. # TODO: should this be of type QtType instead?
        'DefaultValueExpression': '0.0',
        # 'JSONParseFunction': 'ParseJsonFun<double>::parse', # A function which returns a  dbusToQtTypeAll[DBusSignature]. # TODO: should this return a QtType instead? Probably not.
        'CompareFunction': 'defaultValueCompare',  # A function which will get two values and should return -1, 0 or 1 depending on the ordering of the values
    },

    'de.uni_stuttgart.Voxie.PropertyType.Int': {
        'DisplayName': 'Integer number (64 bit, signed)',
        'DBusSignature': 'x',
        'DefaultValueExpression': '0',
        'CompareFunction': 'defaultValueCompare',
    },

    'de.uni_stuttgart.Voxie.PropertyType.Boolean': {
        'DisplayName': 'Boolean',
        'DBusSignature': 'b',
        'DefaultValueExpression': 'false',
        'CompareFunction': 'defaultValueCompare',
    },

    'de.uni_stuttgart.Voxie.PropertyType.String': {
        'DisplayName': 'String',
        'DBusSignature': 's',
        'DefaultValueExpression': '""',
        'CompareFunction': 'defaultValueCompare',
    },

    'de.uni_stuttgart.Voxie.PropertyType.FileName': {
        'DisplayName': 'File Name',
        'DBusSignature': 's',
        'DefaultValueExpression': '""',
        'CompareFunction': 'defaultValueCompare',
    },

    'de.uni_stuttgart.Voxie.PropertyType.Enumeration': {
        'DisplayName': 'Enumeration',
        'DBusSignature': 's',
        'DefaultValueExpression': '""',
        'VerifyFunction': 'verifyEnum',
        'CanonicalizeFunction': 'canonicalizeEnum',
        # 'CompareFunction': 'compareEnum',  # Enums cannot be compared without a property reference
    },

    'de.uni_stuttgart.Voxie.PropertyType.Color': {
        'DisplayName': 'Color',
        'DBusSignature': '(dddd)',
        'QtType': 'vx::Color',
        'DefaultValueExpression': 'vx::Color::black().asTuple()',
        # 'CompareFunction': 'defaultValueCompare',  # TODO: allow comparison?
    },

    # 2D point on a plane in plane coordinate system
    'de.uni_stuttgart.Voxie.PropertyType.Point2D': {
        'DisplayName': 'Point (2D)',
        'DBusSignature': '(dd)',
        'QtType': 'QPointF',
        'DefaultValueExpression': 'vx::TupleVector<double, 2>(0, 0)',
        # 'CompareFunction': 'defaultValueCompare',  # TODO: allow comparison?
    },

    # A position in 3D space
    'de.uni_stuttgart.Voxie.PropertyType.Position3D': {
        'DisplayName': 'Position',
        'DBusSignature': '(ddd)',
        'QtType': 'QVector3D',
        'DefaultValueExpression': 'vx::TupleVector<double, 3>(0, 0, 0)',
        # 'CompareFunction': 'defaultValueCompare',  # TODO: allow comparison?
    },

    # The (integer) size of a box
    'de.uni_stuttgart.Voxie.PropertyType.SizeInteger3D': {
        'DisplayName': 'Size (integer)',
        'DBusSignature': '(ttt)',
        'QtType': 'Vector<quint64, 3>',
        'DefaultValueExpression': 'vx::TupleVector<quint64, 3>(0, 0, 0)',
        # 'CompareFunction': 'defaultValueCompare',  # TODO: allow comparison?
    },

    # The axis-aligned bounding box of an object in three-dimensional space, specified by the minimum and maximum position
    'de.uni_stuttgart.Voxie.PropertyType.Box3DAxisAligned': {
        'DisplayName': 'Bounding box (3D)',
        'DBusSignature': '((ddd)(ddd))',
        'QtType': 'vx::BoundingBox3D',
        'DefaultValueExpression': 'std::make_tuple(vx::TupleVector<double, 3>(0, 0, 0), vx::TupleVector<double, 3>(0, 0, 0))',
        # 'CompareFunction': 'defaultValueCompare',  # TODO: allow comparison?
    },

    # An orientation of an object in 3D space, represented as a quaternion (which, when considered as a rotation, will transform coordinates from the object coordinate system into the global coordinate system)
    'de.uni_stuttgart.Voxie.PropertyType.Orientation3D': {
        'DisplayName': 'Orientation (3D)',
        'DBusSignature': '(dddd)',
        'QtType': 'QQuaternion',
        'DefaultValueExpression': 'vx::TupleVector<double, 4>(1, 0, 0, 0)',
        # 'CompareFunction': 'defaultValueCompare',  # TODO: allow comparison?
    },

    # TODO: Add a type which consists of a Position3D, a SizeInteger3D, a Orientation3D and a (ddd) to describe the position and size of a voxel grid in 3D space?

    # A data type for storing numerical data
    'de.uni_stuttgart.Voxie.PropertyType.DataType': {
        'DisplayName': 'Data type',
        'DBusSignature': '(sus)',
        'QtType': 'vx::DataType',
        # 'DefaultValueExpression': 'std::tuple<QString, quint32, QString>("float", 32, vx::getNativeByteorder())',
        'DefaultValueExpression': 'std::tuple<QString, quint32, QString>("float", 32, "native")',
        'VerifyFunction': 'verifyDataType',
        'CompareFunction': 'defaultValueCompare',  # Will return order defined by vx::DataType
    },

    'de.uni_stuttgart.Voxie.PropertyType.ValueColorMapping': {
        'DisplayName': 'Color mapping',
        'DBusSignature': 'a(d(dddd)i)',
        'QtType': 'QList<vx::ColorizerEntry>',
        'DefaultValueExpression': 'QList<std::tuple<double, vx::TupleVector<double, 4>, qint32>>{std::make_tuple(NAN, vx::TupleVector<double, 4>(0, 0, 0, 0), 0), std::make_tuple(0, vx::TupleVector<double, 4>(0, 0, 0, 1), 0), std::make_tuple(1, vx::TupleVector<double, 4>(1, 1, 1, 1), 0)}',
        'VerifyFunction': 'verifyValueColorMapping',
        'JSONParseFunction': None,
        # No comparison
    },

    'de.uni_stuttgart.Voxie.PropertyType.GeometricPrimitive': {
        'DisplayName': 'Geometric primitive',
        'DBusSignature': 't',
        'QtType': 'quint64',
        'DefaultValueExpression': '0',
        # No comparison?
    },

    'de.uni_stuttgart.Voxie.PropertyType.TomographyRawDataImageKind': {
        'DisplayName': 'Tomography raw data image kind',
        'DBusSignature': 'a{sv}',
        'RawType': 'QJsonObject',
        'QtType': 'QJsonObject',
        'DefaultValueExpression': 'QJsonObject{}',
        # No comparison
    },

    'de.uni_stuttgart.Voxie.PropertyType.TomographyRawDataImageList': {
        'DisplayName': 'Tomography raw data image list',
        'DBusSignature': '(sa{sv})',
        'RawType': 'std::tuple<QString, QJsonObject>',
        'QtType': 'std::tuple<QString, QJsonObject>',
        'DefaultValueExpression': 'std::make_tuple("", QJsonObject{})',
        # No comparison
    },

    # A list of int values
    'de.uni_stuttgart.Voxie.PropertyType.IntList': {  # Name
        'DisplayName': 'Int List',
        'DBusSignature': 'ax',
        'QtType': 'QList<qint64>',
        'DefaultValueExpression': 'QList<qint64>()',
        'JSONParseFunction': None,
        # No comparison
    },

    # A list of labelIDs (For Segmentation)
    'de.uni_stuttgart.Voxie.PropertyType.LabelList': {  # Name
        'DisplayName': 'Label List',
        'DBusSignature': 'at',
        'QtType': 'QList<quint64>',
        'DefaultValueExpression': 'QList<quint64>()',
        'JSONParseFunction': None,
        # No comparison
    },

    # A reference to a single input node (can be a null reference)
    'de.uni_stuttgart.Voxie.PropertyType.NodeReference': {
        'DisplayName': 'Node reference',
        'DBusSignature': 'o',
        'QtType': 'vx::Node*',
        'DefaultValueExpression': 'QDBusObjectPath("/")',
        'JSONParseFunction': None,
        'CompatibilityNames': [
            'de.uni_stuttgart.Voxie.PropertyType.ObjectReference',
        ],
        # No comparison
    },

    # A reference to a list of input nodes
    'de.uni_stuttgart.Voxie.PropertyType.NodeReferenceList': {
        'DisplayName': 'Node reference list',
        'DBusSignature': 'ao',
        'QtType': 'QList<vx::Node*>',
        'DefaultValueExpression': 'QList<QDBusObjectPath>()',
        'JSONParseFunction': None,
        'CompatibilityNames': [
            'de.uni_stuttgart.Voxie.PropertyType.ObjectReferenceList',
        ],
        # No comparison
    },

    # A list of double values
    'de.uni_stuttgart.Voxie.PropertyType.ListPosition3DDoubleTuple': {
        'DisplayName': 'List with (3DPosition, double) tuples',
        'DBusSignature': 'a((ddd)d)',
        'QtType': 'QList<std::tuple<QVector3D, double>>',
        'DefaultValueExpression': 'QList<std::tuple<vx::TupleVector<double, 3>, double>>()',
        'JSONParseFunction': None,
        # No comparison
    },

    # A list of 3D positions
    'de.uni_stuttgart.Voxie.PropertyType.ListPosition3D': {
        'DisplayName': 'Position3D List',
        'DBusSignature': 'a(ddd)',
        'QtType': 'QList<QVector3D>',
        'DefaultValueExpression': 'QList<vx::TupleVector<double, 3>>()',
        'JSONParseFunction': None,
        # No comparison
    },

    # A list of Volume Indexes(Tuple)
    'de.uni_stuttgart.Voxie.PropertyType.VolumeIndexList': {
        "DisplayName": "Volume index list",
        "DBusSignature": "a(uuu)",
        'QtType': 'QList<std::tuple<quint32, quint32, quint32>>',
        'DefaultValueExpression': 'QList<std::tuple<quint32, quint32, quint32>>()',
        'JSONParseFunction': None,
        # No comparison
    },

    # A reference to a single output node (can be a null reference)
    'de.uni_stuttgart.Voxie.PropertyType.OutputNodeReference': {
        'DisplayName': 'Output node reference',
        'DBusSignature': 'o',
        'QtType': 'vx::Node*',
        'DefaultValueExpression': 'QDBusObjectPath("/")',
        'JSONParseFunction': None,
        'CompatibilityNames': [
            'de.uni_stuttgart.Voxie.PropertyType.OutputObjectReference',
        ],
        # No comparison
    },

    # A List of mappings between a double threshold and a label id
    'de.uni_stuttgart.Voxie.PropertyType.ThresholdLabelMapping': {
        'DisplayName': 'ThresholdLabelMapping',
        'DBusSignature': 'a(d(dddd)x)',
        'QtType': 'QList<std::tuple<double, std::tuple<double, double, double, double>, qint64>>',
        'DefaultValueExpression': 'QList<std::tuple<double, vx::TupleVector<double, 4>, qint64>>{std::make_tuple(NAN, vx::TupleVector<double, 4>(0, 0, 0, 0), 0), std::make_tuple(0, vx::TupleVector<double, 4>(0, 0, 0, 1), 0), std::make_tuple(1, vx::TupleVector<double, 4>(1, 1, 1, 1), 0)}',
        'JSONParseFunction': None,
        # No comparison
    },

    # # A List of mappings between double, color tuple and integer
    # 'de.uni_stuttgart.Voxie.PropertyType.DoubleColorIntMapList': {
    #     'DisplayName': 'DoubleColorIntMapList',
    #     'DBusSignature': 'a(d(dddd)i)',
    #     'QtType': 'QList<std::tuple<double, std::tuple<double, double, double, double>, qint32>>',
    #     'DefaultValueExpression': 'QList<std::tuple<double, vx::TupleVector<double, 4>, qint32>>{std::make_tuple(NAN, vx::TupleVector<double, 4>(0, 0, 0, 0), 0), std::make_tuple(0, vx::TupleVector<double, 4>(0, 0, 0, 1), 0), std::make_tuple(1, vx::TupleVector<double, 4>(1, 1, 1, 1), 0)}',
    #     'JSONParseFunction': None,
    # },
}

typesWithCompat = dict(types)
for key in types:
    if 'CompatibilityNames' in types[key]:
        for compat in types[key]['CompatibilityNames']:
            typesWithCompat[compat] = types[key]
