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
from scipy import nan, inf


class TableColumn:
    """Defines a column definition within a TableData structure."""

    def __init__(self, dataType, dbusType, name, label, unit, converter, default):
        """Constructor for a single column.

        The use of this constructor is discouraged; use the datatype-specific class methods instead.
        """
        self.dataType = dataType
        # TODO Determine dbusType from PropertyTypes.json
        self.dbusType = dbusType
        self.name = name
        self.label = label
        self.unit = unit
        self.converter = converter
        self.default = default

    @classmethod
    def int(cls, name, label, unit=None):
        """Defines an integer column with the specified name and label.

        @param name: Column name, used as the key for dict lookups when adding rows, as well as the header in exported CSVs.
        @param label: Column label, used as the table header in Voxie's graphical user interface.
        """
        return cls('de.uni_stuttgart.Voxie.PropertyType.Int', 'x', name, label, unit, int, -1)

    @classmethod
    def float(cls, name, label, unit=None):
        """Defines a floating-point column with the specified name and label.

        @param name: Column name, used as the key for dict lookups when adding rows, as well as the header in exported CSVs.
        @param label: Column label, used as the table header in Voxie's graphical user interface.
        """
        return cls('de.uni_stuttgart.Voxie.PropertyType.Float', 'd', name, label, unit, float, nan)

    @classmethod
    def vec3(cls, name, label, unit=None):
        """Defines a 3D vector-valued floating-point column with the specified name and label.

        @param name: Column name, used as the key for dict lookups when adding rows, as well as the header in exported CSVs.
        @param label: Column label, used as the table header in Voxie's graphical user interface.
        """

        def toFloatVec3(value):
            return (float(value[0]), float(value[1]), float(value[2]))

        return cls('de.uni_stuttgart.Voxie.PropertyType.Position3D', '(ddd)', name, label, unit, toFloatVec3, (nan, nan, nan))

    @classmethod
    def bbox3(cls, name, label, unit=None):
        """Defines a 3D floating-point axis-aligned bounding box column with the specified name and label.

        @param name: Column name, used as the key for dict lookups when adding rows, as well as the header in exported CSVs.
        @param label: Column label, used as the table header in Voxie's graphical user interface.
        """

        def toFloatBBox3(value):
            return ((float(value[0][0]), float(value[0][1]), float(value[0][2])),
                    (float(value[1][0]), float(value[1][1]), float(value[1][2])))

        return cls('de.uni_stuttgart.Voxie.PropertyType.Box3DAxisAligned', '((ddd)(ddd))', name, label, unit, toFloatBBox3,
                   ((inf, inf, inf), (-inf, -inf, -inf)))


def _getMetadata(column):
    metadata = {}
    if column.unit is not None:
        metadata["unit"] = voxie.Variant('s', column.unit)
    return metadata


def createColumnDefinition(instance, columns):
    """Converts a list of TableColumn objects into a table column definition list, to pass to voxie's CreateTableData."""
    return [(column.name, instance.Components.GetComponent('de.uni_stuttgart.Voxie.ComponentType.PropertyType', column.dataType).CastTo('de.uni_stuttgart.Voxie.PropertyType'), column.label, _getMetadata(column), {}) for column in columns]


def createRow(columns, row):
    """Given a list of TableColumn objects, converts a dict into a dbus-ready variant list, to pass to voxie's AddRow."""
    return [voxie.Variant(column.dbusType, column.converter(row.get(column.name, column.default))) for column in columns]
