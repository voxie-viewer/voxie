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

#pragma once

#include <VoxieClient/DBusUtil.hpp>

#include <QtDBus/QDBusArgument>
#include <QtDBus/QDBusVariant>

namespace vx {

namespace DBusNumericUtilIntern {

template <typename ValueType, typename ResultType>
bool dbusTryGetNumberFromArgumentValue(const QDBusArgument& arg,
                                       ResultType& result) {
  if (dbusGetSignature<ValueType>() == dbusGetArgumentSignature(arg)) {
    result =
        static_cast<ResultType>(dbusGetArgumentValueUnchecked<ValueType>(arg));
    return true;
  } else {
    return false;
  }
}

template <typename ValueType, typename ResultType>
bool dbusTryGetNumber(const QVariant& var, ResultType& result) {
  if (var.userType() == qMetaTypeId<QDBusArgument>()) {
    QDBusArgument arg = var.value<QDBusArgument>();
    return dbusTryGetNumberFromArgumentValue<ValueType>(arg, result);
  } else if (var.userType() == qMetaTypeId<ValueType>()) {
    result = static_cast<ResultType>(var.value<ValueType>());
    return true;
  } else {
    return false;
  }
}

}  // namespace DBusNumericUtilIntern

// Helper function for extracting any-type numbers from DBus variants and
// converting them to the desired target type
template <typename T>
T dbusGetNumber(const QDBusVariant& variant) {
  using namespace DBusNumericUtilIntern;

  QVariant qVariant = variant.variant();
  T result = T();

  // Try every numeric type
  bool success = dbusTryGetNumber<double>(qVariant, result) ||
                 dbusTryGetNumber<qint64>(qVariant, result) ||
                 dbusTryGetNumber<quint64>(qVariant, result) ||
                 dbusTryGetNumber<qint32>(qVariant, result) ||
                 dbusTryGetNumber<quint32>(qVariant, result) ||
                 dbusTryGetNumber<qint16>(qVariant, result) ||
                 dbusTryGetNumber<quint16>(qVariant, result) ||
                 dbusTryGetNumber<quint8>(qVariant, result) ||
                 dbusTryGetNumber<bool>(qVariant, result);

  if (success) {
    return result;
  } else {
    throw Exception(
        "de.uni_stuttgart.Voxie.Error",
        QString() + "Unexpected DBus variant, expected a numeric type, got " +
            QMetaType::typeName(qVariant.userType()));
  }
}

}  // namespace vx
