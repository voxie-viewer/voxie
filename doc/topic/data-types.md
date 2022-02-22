Data types in Voxie
===================

Data types are normally stored as triple with base type, number of bits and
endianness.

Base types
----------

Valid base types are:
- `uint`: An unsigned integer.
- `int`: A signed integer in two's complement representation
- `bool`: Same as `uint`, but only 0 (false) and 1 (true) are valid values.
- `float` An IEEE 754-2008 'binary' representation of a floating point number.

Number of bits
--------------

The number of bits used for storing the value. Currently this must always be
a multiple of 8.

Endianness
----------

Can be one of the following values:
- `none`: Only valid if the number of bits is 8.
- `little`: Only valid if the number of bits is not 8, indicates little endian.
- `big`: Only valid if the number of bits is not 8, indicates big endian.
- `native`: In some contexts (e.g. when creating a new volume), `native` can
  be used to indicate the computers native endianness.

DBus
----

When transported over DBus, the data type is transported as `(sus)`, i.e.
a string (the base type), an unsigned 32-bit number (the number of bits) and
another string (the endianness).
