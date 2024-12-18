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


def json_type_to_string(ty):
    if ty == dict:
        return 'object'
    if ty == list:
        return 'array'
    if ty == str:
        return 'string'
    if ty == int:
        return 'number(int)'
    if ty == float:
        return 'number(real)'
    if ty == bool:
        return 'boolean'
    if ty == NoneType:
        return 'null'
    raise Exception('Got unknown json type')


def expect(value, *types):
    actual_ty = type(value)
    if actual_ty not in types:
        if len(types) == 1:
            as_str = json_type_to_string(types[0])
        else:
            as_str = ' or '.join([json_type_to_string(ty) for ty in types])
        raise Exception('Got unexpected JSON data, expected {}, got {}'.format(as_str, json_type_to_string(actual_ty)))
    return value


def expect_string(value):
    return expect(value, str)


def expect_object(value):
    return expect(value, dict)


def expect_array(value):
    return expect(value, list)


def expect_array_with_size(value, size):
    array = expect_array(value)
    if len(value) != size:
        raise Exception('Got unexpected JSON data, expected length {}, got {}'.format(size, len(value)))
    return array


def expect_number(value):
    return expect(value, int, float)


def expect_float(value):
    return float(expect(value, int, float))


def expect_int(value):
    return expect(value, int)


def expect_float3(value):
    expect_array_with_size(value, 3)
    return [expect_float(value[i]) for i in range(3)]


def expect_int3(value):
    expect_array_with_size(value, 3)
    return [expect_int(value[i]) for i in range(3)]
