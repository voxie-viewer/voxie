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

#include <VoxieClient/VoxieClient.hpp>

#include <QtCore/QJsonDocument>

namespace vx {
template <typename T, std::size_t dim>
class Vector;

// filename is for diagnostic purposes
VOXIECLIENT_EXPORT QJsonDocument parseJsonData(const QByteArray& data,
                                               const QString& filename);

VOXIECLIENT_EXPORT QJsonDocument parseJsonFile(const QString& filename);

VOXIECLIENT_EXPORT QString jsonTypeToString(QJsonValue::Type type);

VOXIECLIENT_EXPORT bool isNull(const QJsonValue& value);

VOXIECLIENT_EXPORT bool expectBool(const QJsonValue& value);
VOXIECLIENT_EXPORT QString expectString(const QJsonValue& value);
VOXIECLIENT_EXPORT QJsonObject expectObject(const QJsonValue& value);
VOXIECLIENT_EXPORT QJsonArray expectArray(const QJsonValue& value);
VOXIECLIENT_EXPORT double expectDouble(const QJsonValue& value);
VOXIECLIENT_EXPORT qint64 expectSignedInt(const QJsonValue& value);
VOXIECLIENT_EXPORT quint64 expectUnsignedInt(const QJsonValue& value);
VOXIECLIENT_EXPORT vx::Vector<double, 3> expectVector3(const QJsonValue& value);
VOXIECLIENT_EXPORT QList<QString> expectStringList(const QJsonValue& value);

VOXIECLIENT_EXPORT QJsonObject expectObject(const QJsonDocument& doc);
VOXIECLIENT_EXPORT QJsonArray expectArray(const QJsonDocument& doc);
}  // namespace vx
