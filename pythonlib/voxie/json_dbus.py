import math
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
import dbus

# Conversion as described in doc/topic/interfaces/json-on-dbus.md


def json_to_dbus(value, *, accept_variants_in_json=False):
    import voxie

    if value is None:
        return voxie.Variant('g', dbus.Signature(''))
    elif isinstance(value, dict):
        return voxie.Variant('a{sv}', json_to_dbus_dict(value, accept_variants_in_json=accept_variants_in_json))
    elif isinstance(value, list) or isinstance(value, tuple):
        res = []
        for val in value:
            res.append(json_to_dbus(val, accept_variants_in_json=accept_variants_in_json))
        return voxie.Variant('av', res)
    elif isinstance(value, bool):
        return voxie.Variant('b', value)
    elif isinstance(value, str):
        return voxie.Variant('s', value)
    elif isinstance(value, int):
        if value < 2**63:
            return voxie.Variant('x', value)
        else:
            return voxie.Variant('t', value)
    elif isinstance(value, float):
        if (value % 1) == 0 and value >= 0 and value <= 2**64 - 1:
            return voxie.Variant('t', int(value))
        elif (value % 1) == 0 and value >= -2**63 and value <= 2**63 - 1:
            return voxie.Variant('x', int(value))
        else:
            return voxie.Variant('d', value)
    elif accept_variants_in_json and isinstance(value, voxie.Variant):
        return json_to_dbus(value.value, accept_variants_in_json=accept_variants_in_json)
    else:
        raise Exception('json_to_dbus: Unknown JSON type: ' + str(type(value)))


def json_to_dbus_dict(value, *, accept_variants_in_json=False):
    if not isinstance(value, dict):
        raise Exception('Expected a dict, got a ' + str(type(value)))
    res = {}
    for key in value:
        res[key] = json_to_dbus(value[key], accept_variants_in_json=accept_variants_in_json)
    return res


def dbus_to_json(value):
    import voxie

    if not isinstance(value, voxie.Variant):
        raise Exception('Expected a voxie.Variant, got a ' + str(type(value)))
    sig = value.signature

    if sig == 'x' or sig == 't' or sig == 'd':
        return value.value
    if sig == 's':
        return value.value
    if sig == 'b':
        return value.value
    if sig == 'av':
        result = []
        for entry in value.getValue('av'):
            result.append(dbus_to_json(entry))
        return result
    if sig == 'a{sv}':
        entries = value.getValue('a{sv}')
        return dbus_to_json_dict(entries)
    if sig == 'g':
        cont = str(value.getValue('g'))
        if cont == '' or cont == 'n':
            return None
        raise Exception('Unknown dbus signature value: ' + repr(cont))

    raise Exception('Unknown dbus JSON value: ' + repr(sig))


def dbus_to_json_dict(value):
    if not isinstance(value, dict):
        raise Exception('Expected a dict, got a ' + str(type(value)))
    result = {}
    for key in value:
        result[key] = dbus_to_json(value[key])
    return result
