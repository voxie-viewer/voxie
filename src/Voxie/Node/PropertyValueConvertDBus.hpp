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

#include <Voxie/Voxie.hpp>

#include <VoxieClient/DBusTypeList.hpp>

#include <QtCore/QJsonObject>

Q_DECLARE_METATYPE(VX_IDENTITY_TYPE((std::tuple<QString, QJsonObject>)))

namespace vx {
template <typename Raw, typename DBusType>
// Must not be marked with VOXIECORESHARED_EXPORT because
// PropertyValueConvertDBus<T, T> is inline (not really sure how exporting works
// on windows here)
struct PropertyValueConvertDBus;

template <typename T>
// Must not be marked with VOXIECORESHARED_EXPORT because it is inline
struct PropertyValueConvertDBus<T, T> {
  static T fromRaw(const T& raw) { return raw; }
  static T toRaw(const T& dbus) { return dbus; }
};

template <>
struct VOXIECORESHARED_EXPORT
    PropertyValueConvertDBus<QJsonObject, QMap<QString, QDBusVariant>> {
  static QMap<QString, QDBusVariant> fromRaw(const QJsonObject& raw);
  static QJsonObject toRaw(const QMap<QString, QDBusVariant>& dbus);
};

template <>
struct VOXIECORESHARED_EXPORT
    PropertyValueConvertDBus<std::tuple<QString, QJsonObject>,
                             std::tuple<QString, QMap<QString, QDBusVariant>>> {
  static std::tuple<QString, QMap<QString, QDBusVariant>> fromRaw(
      const std::tuple<QString, QJsonObject>& raw);
  static std::tuple<QString, QJsonObject> toRaw(
      const std::tuple<QString, QMap<QString, QDBusVariant>>& dbus);
};

template <>
struct VOXIECORESHARED_EXPORT
    PropertyValueConvertDBus<QJsonValue, QDBusVariant> {
  static QDBusVariant fromRaw(const QJsonValue& raw);
  static QJsonValue toRaw(const QDBusVariant& dbus);
};
}  // namespace vx
