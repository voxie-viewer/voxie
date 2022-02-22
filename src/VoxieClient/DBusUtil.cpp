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

#include "DBusUtil.hpp"

#include <VoxieClient/DBusTypeList.hpp>
#include <VoxieClient/Exception.hpp>

QDBusSignature vx::dbusGetArgumentSignature(const QDBusArgument& arg) {
  return QDBusSignature(arg.currentSignature());  // TODO: is an internal API
}

QDBusSignature vx::dbusGetVariantSignature(const QDBusVariant& variant) {
  auto var = variant.variant();
  auto type = var.userType();
  // See QDBusDemarshaller::toVariantInternal()
  if (type == qMetaTypeId<QDBusArgument>())
    return dbusGetArgumentSignature(var.value<QDBusArgument>());
#define TYPE(sig, QtType) \
  else if (type == qMetaTypeId<QtType>()) return QDBusSignature(sig);
  TYPE("y", uchar)
  TYPE("b", bool)
  TYPE("n", ushort)
  TYPE("q", short)
  TYPE("i", int)
  TYPE("u", uint)
  TYPE("x", qlonglong)
  TYPE("t", qulonglong)
  TYPE("d", double)
  TYPE("h", QDBusUnixFileDescriptor)

  TYPE("s", QString)
  TYPE("o", QDBusObjectPath)
  TYPE("g", QDBusSignature)

  TYPE("v", QDBusVariant)

  TYPE("ay", QByteArray)
  TYPE("as", QStringList)
#undef TYPE
  else {
    throw Exception("de.uni_stuttgart.Voxie.Error",
                    QString() +
                        "Unable to get DBus signature for variant with type: " +
                        QMetaType::typeName(type));
  }
}

Q_DECLARE_METATYPE(VX_IDENTITY_TYPE((std::tuple<int, int>)))

static void dbusCompileTest() {
  (void)&dbusCompileTest;  // Ignore the fact that the function is unused

  QDBusArgument arg;
  (void)vx::dbusGetArgumentValue<int>(arg);
  (void)vx::dbusGetArgumentValue<QList<quint64>>(arg);
  (void)vx::dbusGetArgumentValue<QMap<QString, QDBusUnixFileDescriptor>>(arg);
  (void)vx::dbusGetArgumentValue<QList<quint16>>(arg);
  (void)vx::dbusGetArgumentValue<QList<std::tuple<bool, QString>>>(arg);

  QVariant::fromValue(std::make_tuple(4, 5));

  std::tuple<int, int, int> a = vx::TupleVector<int, 3>(4, 5, 6);
  (void)a;
}
