#
# Copyright (c) 2014-2025 The Voxie Authors
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

# See doc/topic/data-types.md

base_type_to_numpy = {
    "float": "f",
    "uint": "u",
    "int": "i",
    "bool": "b",
}
base_type_from_numpy = {}
for key in base_type_to_numpy:
    value = base_type_to_numpy[key]
    if value in base_type_from_numpy:
        raise Exception('value in base_type_from_numpy')
    base_type_from_numpy[value] = key

endianness_to_numpy = {
    "none": "|",
    "little": "<",
    "big": ">",
}
endianness_from_numpy = {}
for key in endianness_to_numpy:
    value = endianness_to_numpy[key]
    if value in endianness_from_numpy:
        raise Exception('value in endianness_from_numpy')
    endianness_from_numpy[value] = key


def to_numpy_dtype_string(data_type, *, allow_native=False):
    if len(data_type) != 3:
        raise Exception('Expected tuple with 3 values')
    if type(data_type[0]) != str:
        raise Exception('Expected string as first value of data type')
    if type(data_type[1]) != int:
        raise Exception('Expected int as second value of data type')
    if type(data_type[2]) != str:
        raise Exception('Expected string as third value of data type')

    if data_type[1] % 8 != 0:
        raise Exception('Number of bits is not a multiple of 8')
    if data_type[1] < 0:
        raise Exception('Negative number of bits')

    if allow_native and data_type[2] == 'native':
        endianness_str = ''
    else:
        endianness_str = endianness_to_numpy[data_type[2]]

    return endianness_str + base_type_to_numpy[data_type[0]] + str(data_type[1] // 8)
