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

#include "JsonDBus.hpp"

#include <VoxieClient/CastFloatToIntSafe.hpp>
#include <VoxieClient/DBusUtil.hpp>

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QMap>
#include <QtDBus/QDBusVariant>

#include <cmath>
#include <limits>

namespace vx {

QDBusVariant jsonToDBus(const QJsonValue& value) {
  switch (value.type()) {
    case QJsonValue::Type::Null:
    case QJsonValue::Type::Undefined:
    default:
      // Because of https://bugreports.qt.io/browse/QTBUG-124919 a signature of
      // "n" instead of "" is used.
      // return dbusMakeVariant(QDBusSignature(""));
      return dbusMakeVariant(QDBusSignature("n"));

    case QJsonValue::Type::Array:
      return dbusMakeVariant(jsonToDBus(value.toArray()));

    case QJsonValue::Type::Object:
      return dbusMakeVariant(jsonToDBus(value.toObject()));

    case QJsonValue::Type::Bool:
      return dbusMakeVariant(value.toBool());

    case QJsonValue::Type::String:
      return dbusMakeVariant(value.toString());

    case QJsonValue::Type::Double: {
      double number = value.toDouble();
      if (std::fmod(number, 1.0) != 0.0) {
        // Number is not an integer; store as double
        return dbusMakeVariant(number);
      } else if (auto numberInt = vx::castFloatToIntSafe<qint64>(number)) {
        // Number fits into a signed 64-bit integer
        return dbusMakeVariant(numberInt.value());
      } else if (auto numberUInt = vx::castFloatToIntSafe<quint64>(number)) {
        // Number fits into an unsigned 64-bit integer
        return dbusMakeVariant(numberUInt.value());
      } else {
        // Number is too large; store as double
        return dbusMakeVariant(number);
      }
    }
  }
}

QMap<QString, QDBusVariant> jsonToDBus(const QJsonObject& object) {
  QMap<QString, QDBusVariant> map;
  for (const auto& key : object.keys()) {
    map.insert(key, jsonToDBus(object.value(key)));
  }
  return map;
}

QList<QDBusVariant> jsonToDBus(const QJsonArray& array) {
  QList<QDBusVariant> list;
  for (const auto& entry : array) {
    list.append(jsonToDBus(entry));
  }
  return list;
}

QJsonValue dbusToJson(const QDBusVariant& value) {
  auto signature = dbusGetVariantSignature(value);

  if (signature == QDBusSignature("g")) {
    auto val = dbusGetVariantValue<QDBusSignature>(value);
    if (val.signature() != "" && val.signature() != "n")
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Got unsupported QDBusSignature ('g') value");
    return QJsonValue(QJsonValue::Null);
  } else if (signature == QDBusSignature("av")) {
    auto val = dbusGetVariantValue<QList<QDBusVariant>>(value);

    QJsonArray result;
    for (const auto& entry : val) result << dbusToJson(entry);
    return result;
  } else if (signature == QDBusSignature("a{sv}")) {
    return dbusToJson(dbusGetVariantValue<QMap<QString, QDBusVariant>>(value));
  } else if (signature == QDBusSignature("b")) {
    return dbusGetVariantValue<bool>(value);
  } else if (signature == QDBusSignature("s")) {
    return dbusGetVariantValue<QString>(value);
  } else if (signature == QDBusSignature("d")) {
    return dbusGetVariantValue<double>(value);
  } else if (signature == QDBusSignature("t")) {
    return (double)dbusGetVariantValue<quint64>(value);
  } else if (signature == QDBusSignature("x")) {
    return dbusGetVariantValue<qint64>(value);
  } else {
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.Error",
        "Unsupported DBus signature in dbusToJson(): " + signature.signature());
  }
}

QJsonObject dbusToJson(const QMap<QString, QDBusVariant>& value) {
  QJsonObject result;
  for (const auto& key : value.keys()) result[key] = dbusToJson(value[key]);
  return result;
}

}  // namespace vx
