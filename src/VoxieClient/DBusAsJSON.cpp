/*
 * Copyright (c) 2014-2022 The Voxie Authors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "DBusAsJSON.hpp"

#include <VoxieClient/DBusUtil.hpp>

#include <QtCore/QJsonValue>

// TODO: More error checking (probably also in python implementation)

QDBusVariant vx::decodeDBusAsJSON(const QDBusSignature& sig,
                                  const QJsonValue& value) {
  if (sig == QDBusSignature("s")) {
    return vx::dbusMakeVariant<QString>(value.toString());
  } else if (sig == QDBusSignature("g")) {
    return vx::dbusMakeVariant<QDBusSignature>(
        QDBusSignature(value.toString()));
  } else if (sig == QDBusSignature("b")) {
    return vx::dbusMakeVariant<bool>(value.toBool());

    // TODO: for integer values there might be overflow
  } else if (sig == QDBusSignature("y")) {
    return vx::dbusMakeVariant<quint8>(value.toDouble());
  } else if (sig == QDBusSignature("n")) {
    return vx::dbusMakeVariant<qint16>(value.toDouble());
  } else if (sig == QDBusSignature("q")) {
    return vx::dbusMakeVariant<quint16>(value.toDouble());
  } else if (sig == QDBusSignature("i")) {
    return vx::dbusMakeVariant<qint32>(value.toDouble());
  } else if (sig == QDBusSignature("u")) {
    return vx::dbusMakeVariant<quint32>(value.toDouble());
  } else if (sig == QDBusSignature("x")) {
    return vx::dbusMakeVariant<qint64>(value.toDouble());
  } else if (sig == QDBusSignature("t")) {
    return vx::dbusMakeVariant<quint64>(value.toDouble());

  } else if (sig == QDBusSignature("d")) {
    if (value.isString()) {
      if (value.toString() == "NaN")
        return vx::dbusMakeVariant<double>(
            std::numeric_limits<double>::quiet_NaN());
      else if (value.toString() == "Infinity")
        return vx::dbusMakeVariant<double>(
            std::numeric_limits<double>::infinity());
      else if (value.toString() == "-Infinity")
        return vx::dbusMakeVariant<double>(
            -std::numeric_limits<double>::infinity());
      else
        throw vx::Exception(
            "de.uni_stuttgart.Voxie.Error",
            "Unsupported string for double in decodeDBusAsJSON(): '" +
                value.toString() + "'");
    } else {
      return vx::dbusMakeVariant<double>(value.toDouble());
    }

    // TODO: Support for structs, arrays, dicts
    // TODO: (Optional) support for object paths
    // TODO: Support for variants
  } else {
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.Error",
        "Unsupported DBus signature in decodeDBusAsJSON(): " + sig.signature());
  }

  /*
    elif sig.startswith('('):
        res = []
        subtypes = list(dbus.Signature(str(sig)[1:-1]))
        for i in range(len(subtypes)):
            res.append(decode_dbus_as_json(subtypes[i], value[i], style))
        return tuple(res)
    elif sig.startswith('a{'):
        subtypes = list(dbus.Signature(str(sig)[1:]))
        if len(subtypes) != 2:
            raise Exception('len(subtypes) != 2')
        if subtypes[0] != 's':
            raise Exception('Array with non-string keys not supported')
        subtype = subtypes[1]
        res = {}
        for key in value.keys:
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
            import json_dbus
            return json_dbus.json_to_dbus(value)
        else:
            raise Exception('Unknown VariantStyle: ' + style.variant_style)
  */
}
