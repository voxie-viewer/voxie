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
import enum

# Conversion as described in doc/topic/interfaces/dbus-as-json.md

# TODO: A lot of stuff here is not really tested


class VariantStyle(enum.Enum):
    Unsupported = 0
    ArbitraryVariant = 1
    JsonVariant = 2


class DBusAsJSONStyle:
    def __init__(self):
        self.allow_object_paths = False
        self.variant_style = VariantStyle.Unsupported


def encode_dbus_as_json(sig, value, style=DBusAsJSONStyle()):
    if sig == 's':
        return value
    elif sig == 'g':
        return str(value)
    elif sig == 'b':
        return value
    elif sig == 'y' or sig == 'n' or sig == 'q' or sig == 'i' or sig == 'u' or sig == 'x' or sig == 't':
        return value
    elif sig == 'd':
        # Represent NaN / Infinity in a way which is kind of 'compatible' to JS but is valid JSON
        if math.isnan(value):
            return 'NaN'
        elif math.isinf(value):
            if value >= 0:
                return 'Infinity'
            else:
                return '-Infinity'
        else:
            return value
    elif sig.startswith('('):
        res = []
        subtypes = list(dbus.Signature(str(sig)[1:-1]))
        for i in range(len(subtypes)):
            res.append(encode_dbus_as_json(subtypes[i], value[i], style))
        return res
    elif sig.startswith('a{'):
        subtypes = list(dbus.Signature(str(sig)[2:-1]))
        if len(subtypes) != 2:
            raise Exception('len(subtypes) != 2')
        if subtypes[0] != 's':
            raise Exception('Array with non-string keys not supported')
        subtype = subtypes[1]
        res = {}
        for key in value.keys:
            res[key] = encode_dbus_as_json(subtype, value[key], style)
        return res
    elif sig.startswith('a'):
        subtypes = list(dbus.Signature(str(sig)[1:]))
        if len(subtypes) != 1:
            raise Exception('len(subtypes) != 1')
        subtype = subtypes[0]
        res = []
        for i in range(len(value)):
            res.append(encode_dbus_as_json(subtype, value[i], style))
        return res
    elif sig == 'o':
        if not style.allow_object_paths:
            raise Exception('Object paths are not supported')
        return str(value)
    elif sig == 'v':
        if style.variant_style == VariantStyle.Unsupported:
            raise Exception('Variants are not supported')
        elif style.variant_style == VariantStyle.ArbitraryVariant:
            varsig = value.signature
            return [str(varsig), encode_dbus_as_json(varsig, value.value, style)]
        elif style.variant_style == VariantStyle.JsonVariant:
            from . import json_dbus
            return json_dbus.dbus_to_json(value)
        else:
            raise Exception('Unknown VariantStyle: ' + style.variant_style)
    else:
        raise Exception('Unsupported DBus type: ' + repr(sig))


def encode_dbus_as_json_variant(sig, variant, style=DBusAsJSONStyle()):
    if variant.signature != sig:
        raise Exception('Expected signature %s, got %s',
                        sig, variant.signature)
    return encode_dbus_as_json(sig, variant.value, style)


def decode_dbus_as_json(sig, value, style=DBusAsJSONStyle()):
    if sig == 's':
        return str(value)
    elif sig == 'g':
        return dbus.Signature(str(value))
    elif sig == 'b':
        return bool(value)
    elif sig == 'y' or sig == 'n' or sig == 'q' or sig == 'i' or sig == 'u' or sig == 'x' or sig == 't':
        return int(value)
    elif sig == 'd':
        if value == 'NaN' or value == 'Infinity' or value == '-Infinity':
            return float(value)
        return float(value)
    elif sig.startswith('('):
        res = []
        subtypes = list(dbus.Signature(str(sig)[1:-1]))
        for i in range(len(subtypes)):
            res.append(decode_dbus_as_json(subtypes[i], value[i], style))
        return tuple(res)
    elif sig.startswith('a{'):
        subtypes = list(dbus.Signature(str(sig)[2:-1]))
        if len(subtypes) != 2:
            raise Exception('len(subtypes) != 2')
        if subtypes[0] != 's':
            raise Exception('Array with non-string keys not supported')
        subtype = subtypes[1]
        res = {}
        for key in value.keys():
            res[key] = decode_dbus_as_json(subtype, value[key], style)
        return res
    elif sig.startswith('a'):
        subtypes = list(dbus.Signature(str(sig)[1:]))
        if len(subtypes) != 1:
            raise Exception('len(subtypes) != 1')
        subtype = subtypes[0]
        res = []
        for i in range(len(value)):
            res.append(decode_dbus_as_json(subtype, value[i], style))
        return res
    elif sig == 'o':
        if not style.allow_object_paths:
            raise Exception('Object paths are not supported')
        return dbus.ObjectPath(str(value))
    elif sig == 'v':
        if style.variant_style == VariantStyle.Unsupported:
            raise Exception('Variants are not supported')
        elif style.variant_style == VariantStyle.ArbitraryVariant:
            if len(value) != 2:
                raise Exception('Expected 2 values for variant')
            varsig = value[0]
            return decode_dbus_as_json(varsig, value[1], style)
        elif style.variant_style == VariantStyle.JsonVariant:
            from . import json_dbus
            return json_dbus.json_to_dbus(value)
        else:
            raise Exception('Unknown VariantStyle: ' + style.variant_style)
    else:
        raise Exception('Unsupported DBus type: ' + repr(sig))
