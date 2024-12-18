Buffers
=======

Instances of de.uni_stuttgart.Voxie.Buffer are used as shared memory buffer
to exchange data between Voxie and extensions.

Buffer Types
------------

Buffer types are described as JSON data.

Each type is a JSON array where the first value is string describing which kind
of type it is. Types can be primitive types, arrays or structs.

### Primitive types

Primitive types have the string `"primitive"` as the first element.

There are 3 more elements in the array, see [Data types in Voxie](voxie:///help/topic/data-types) for more information.

Examples:
```
# An unsigned 8-bit number
["primitive", "uint", 8, "none"]

# An unsigned 16-bit number, little endian
["primitive", "uint", 16, "little"]

# An 32-bit number, bit endian
["primitive", "float", 32, "big"]
```

### Array types

Array types describe a regular array. In most cases, the content of a buffer
will be an array.

The elements describing an array type are:
- The string `"array`"
- A list of non-negative integers describing the shape of the array
- A list of integers describing the strides of the array in bytes
- The type of the elements

Examples:
```
# A one-dimensional array of 20 16-bit integers without padding.
["array", [20], [2], ["primitive, "uint", 16, "little"]]

# The same array, but with 2 bytes of padding after each value.
["array", [20], [4], ["primitive, "uint", 16, "little"]]

# A three-dimensional array with 10x12x14 float values without padding, in fortran order.
["array", [10, 12, 14], [4, 40, 480], ["primitive, "float", 32, "little"]]

# An array stored in reverse order (requires an offset value of at least 72)
["array", [10], [-8], ["primitive, "float", 64, "little"]]
```

### Struct types

Struct types contain a list of members.

A struct type is described by the string `"struct"` and a list of the description of the members.

Each struct member is described by a list containing:
- The internal name of the struct member as a string or `null` if the member does not have a name
- The offset of the member in the struct in bytes as an integer
- The type of the member

Examples:
```
# A complex number
["struct", [
    ["Real", 0, ["primitive", "float", 32, "little"]],
    ["Imag", 4, ["primitive", "float", 32, "little"]]
]]

# A complex number, without member names
["struct", [
    [null, 0, ["primitive", "float", 32, "little"]],
    [null, 4, ["primitive", "float", 32, "little"]]
]]
```
