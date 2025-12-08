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
#include <VoxieClient/DBusUtilVoxie.hpp>
#include <VoxieClient/DBusVariant.hpp>

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QMap>
#include <QtDBus/QDBusVariant>

#include <cmath>
#include <limits>

namespace vx {

// TODO: Remove?
QDBusVariant jsonToDBus(const QJsonValue& value) {
  return jsonToDBusVoxie(value).asQDBusVariant();
}
QMap<QString, QDBusVariant> jsonToDBus(const QJsonObject& object) {
  QMap<QString, QDBusVariant> result;
  auto map = jsonToDBusVoxie(object);
  for (auto entry = map.begin(); entry != map.end(); ++entry) {
    result.insert(entry.key(), entry.value().asQDBusVariant());
  }
  return result;
}
QList<QDBusVariant> jsonToDBus(const QJsonArray& array) {
  QList<QDBusVariant> result;
  for (const auto& entry : jsonToDBusVoxie(array))
    result.push_back(entry.asQDBusVariant());
  return result;
}

vx::DBusVariant jsonToDBusVoxie(const QJsonValue& value) {
  switch (value.type()) {
    case QJsonValue::Type::Null:
    case QJsonValue::Type::Undefined:
    default:
      // Because of https://bugreports.qt.io/browse/QTBUG-124919 a signature of
      // "n" instead of "" is used.
      // return DBusVariant(
      //     DBusValue::createBasic<QDBusSignature>(QDBusSignature("")));
      return DBusVariant(
          DBusValue::createBasic<QDBusSignature>(QDBusSignature("n")));

    case QJsonValue::Type::Array: {
      std::vector<vx::DBusValue> values;
      for (const auto& entry : jsonToDBusVoxie(value.toArray()))
        values.push_back(DBusValue::createVariant(entry));
      return DBusVariant(DBusValue::createArray(QDBusSignature("v"), values));
    }

    case QJsonValue::Type::Object: {
      std::vector<std::tuple<vx::DBusValue, vx::DBusValue>> values;
      auto map = jsonToDBusVoxie(value.toObject());
      for (auto entry = map.begin(); entry != map.end(); ++entry) {
        values.push_back(
            std::make_tuple(DBusValue::createBasic<QString>(entry.key()),
                            DBusValue::createVariant(entry.value())));
      }
      return DBusVariant(DBusValue::createDictionary(
          QDBusSignature("s"), QDBusSignature("v"), values));
    }

    case QJsonValue::Type::Bool:
      return vx::DBusVariant(DBusValue::createBasic<bool>(value.toBool()));

    case QJsonValue::Type::String:
      return vx::DBusVariant(DBusValue::createBasic<QString>(value.toString()));

    case QJsonValue::Type::Double: {
      double number = value.toDouble();
      if (std::fmod(number, 1.0) != 0.0) {
        // Number is not an integer; store as double
        return vx::DBusVariant(DBusValue::createBasic<double>(number));
      } else if (auto numberInt = vx::castFloatToIntSafe<qint64>(number)) {
        // Number fits into a signed 64-bit integer
        return vx::DBusVariant(
            DBusValue::createBasic<qint64>(numberInt.value()));
      } else if (auto numberUInt = vx::castFloatToIntSafe<quint64>(number)) {
        // Number fits into an unsigned 64-bit integer
        return vx::DBusVariant(
            DBusValue::createBasic<quint64>(numberUInt.value()));
      } else {
        // Number is too large; store as double
        return vx::DBusVariant(DBusValue::createBasic<double>(number));
      }
    }
  }
}

QMap<QString, vx::DBusVariant> jsonToDBusVoxie(const QJsonObject& object) {
  QMap<QString, vx::DBusVariant> map;
  for (const auto& key : object.keys()) {
    map.insert(key, jsonToDBusVoxie(object.value(key)));
  }
  return map;
}

QList<vx::DBusVariant> jsonToDBusVoxie(const QJsonArray& array) {
  QList<vx::DBusVariant> list;
  for (const auto& entry : array) {
    list.append(jsonToDBusVoxie(entry));
  }
  return list;
}

// TODO: Remove
QJsonValue dbusToJson(const QDBusVariant& value) {
  return dbusVoxieToJson(vx::DBusVariant::fromQDBusVariant(value));
}
QJsonObject dbusToJson(const QMap<QString, QDBusVariant>& value) {
  QJsonObject result;
  for (const auto& key : value.keys()) result[key] = dbusToJson(value[key]);
  return result;
}

QJsonValue dbusVoxieToJson(const vx::DBusVariant& value) {
  auto signature = value.valueSignature();

  if (signature == QDBusSignature("g")) {
    auto val = dbusVoxieParseVariantValue<QDBusSignature>(value);
    if (val.signature() != "" && val.signature() != "n")
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Got unsupported QDBusSignature ('g') value");
    return QJsonValue(QJsonValue::Null);
  } else if (signature == QDBusSignature("av")) {
    auto val = dbusVoxieParseVariantValue<QList<vx::DBusVariant>>(value);

    QJsonArray result;
    for (const auto& entry : val) result << dbusVoxieToJson(entry);
    return result;
  } else if (signature == QDBusSignature("a{sv}")) {
    return dbusVoxieToJson(
        dbusVoxieParseVariantValue<QMap<QString, vx::DBusVariant>>(value));
  } else if (signature == QDBusSignature("b")) {
    return dbusVoxieParseVariantValue<bool>(value);
  } else if (signature == QDBusSignature("s")) {
    return dbusVoxieParseVariantValue<QString>(value);
  } else if (signature == QDBusSignature("d")) {
    return dbusVoxieParseVariantValue<double>(value);
  } else if (signature == QDBusSignature("t")) {
    return (double)dbusVoxieParseVariantValue<quint64>(value);
  } else if (signature == QDBusSignature("x")) {
    return dbusVoxieParseVariantValue<qint64>(value);
  } else {
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.Error",
        "Unsupported DBus signature in dbusToJson(): " + signature.signature());
  }
}

QJsonObject dbusVoxieToJson(const QMap<QString, vx::DBusVariant>& value) {
  QJsonObject result;
  for (auto entry = value.begin(); entry != value.end(); ++entry)
    result.insert(entry.key(), dbusVoxieToJson(entry.value()));
  return result;
}
}  // namespace vx
