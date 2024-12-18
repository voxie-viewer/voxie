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

#include "PropertyValueConvertDBus.hpp"

#include <VoxieClient/JsonDBus.hpp>

#include <QtCore/QJsonObject>

using namespace vx;

QMap<QString, QDBusVariant> PropertyValueConvertDBus<
    QJsonObject, QMap<QString, QDBusVariant>>::fromRaw(const QJsonObject& raw) {
  return vx::jsonToDBus(raw);
}
QJsonObject
PropertyValueConvertDBus<QJsonObject, QMap<QString, QDBusVariant>>::toRaw(
    const QMap<QString, QDBusVariant>& dbus) {
  return vx::dbusToJson(dbus);
}

std::tuple<QString, QMap<QString, QDBusVariant>>
PropertyValueConvertDBus<std::tuple<QString, QJsonObject>,
                         std::tuple<QString, QMap<QString, QDBusVariant>>>::
    fromRaw(const std::tuple<QString, QJsonObject>& raw) {
  return std::make_tuple(std::get<0>(raw), vx::jsonToDBus(std::get<1>(raw)));
}
std::tuple<QString, QJsonObject>
PropertyValueConvertDBus<std::tuple<QString, QJsonObject>,
                         std::tuple<QString, QMap<QString, QDBusVariant>>>::
    toRaw(const std::tuple<QString, QMap<QString, QDBusVariant>>& dbus) {
  return std::make_tuple(std::get<0>(dbus), vx::dbusToJson(std::get<1>(dbus)));
}

QDBusVariant PropertyValueConvertDBus<QJsonValue, QDBusVariant>::fromRaw(
    const QJsonValue& raw) {
  return vx::jsonToDBus(raw);
}
QJsonValue PropertyValueConvertDBus<QJsonValue, QDBusVariant>::toRaw(
    const QDBusVariant& dbus) {
  return vx::dbusToJson(dbus);
}
