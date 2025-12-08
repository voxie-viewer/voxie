/*
 * Copyright (c) 2014-2025 The Voxie Authors
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

#include "DBusVariant.hpp"

#include <VoxieClient/DBusSignature.hpp>

vx::DBusVariant vx::DBusVariant::fromQDBusVariant(const QDBusVariant& variant) {
  auto var = variant.variant();
  auto type = var.userType();

  // See QDBusDemarshaller::toVariantInternal()

  if (type == qMetaTypeId<QDBusArgument>()) {
    QDBusArgument arg = var.value<QDBusArgument>();

    // TODO: This seems to be an internal API, but there does not seem to be any
    // other way to get the signature of the content of a variant (especially if
    // it is an array or a dictionary)
    QString signatureString = arg.currentSignature();
    auto signature = DBusSignatureBase::parseSingle(signatureString);

    return DBusVariant(DBusValue::demarshalFromQDBusArgument(signature, arg));
  }
#define TYPE(sig, QtType)                                     \
  else if (type == qMetaTypeId<QtType>()) return DBusVariant( \
      DBusValue::createBasic<QtType>(var.value<QtType>()));
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
#undef TYPE
  else if (type == qMetaTypeId<QDBusVariant>()) return DBusVariant(
      DBusValue::createVariant(fromQDBusVariant(var.value<QDBusVariant>())));
  else if (type == qMetaTypeId<QByteArray>()) return DBusVariant(
      DBusValue::fromQByteArray(var.value<QByteArray>()));
  else if (type == qMetaTypeId<QStringList>()) return DBusVariant(
      DBusValue::fromQStringList(var.value<QStringList>()));
  else {
    throw Exception("de.uni_stuttgart.Voxie.Error",
                    QString() +
                        "Unable to get DBus signature for variant with type: " +
                        QMetaType::typeName(type));
  }
}

QDBusVariant vx::DBusVariant::asQDBusVariant() const {
  // Add special cases for int etc. here were a variant containing an int
  // is returned?
  // Does not seem to be needed and might cause problems if the decision is made
  // incorrectly.
  QDBusArgument arg;
  this->value().marshalToQDBusArgument(arg);
  return QDBusVariant(QVariant::fromValue<QDBusArgument>(arg));
}

QDBusSignature vx::DBusVariant::valueSignature() const {
  return this->value().signature();
}

const QString vx::ToStringImpl<vx::DBusVariant>::toString(
    const DBusVariant& variant) {
  return "Variant(" + vx::toString(variant.valueSignature()) + ", " +
         vx::toString(variant.value()) + ")";
}

bool vx::operator==(const vx::DBusVariant& v1, const vx::DBusVariant& v2) {
  return v1.value() == v2.value();
}
bool vx::operator!=(const vx::DBusVariant& v1, const vx::DBusVariant& v2) {
  return v1.value() != v2.value();
}
