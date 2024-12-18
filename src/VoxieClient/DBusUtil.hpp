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

#include <VoxieClient/Exception.hpp>
#include <VoxieClient/VoxieClient.hpp>

#include <QtDBus/QDBusArgument>
#include <QtDBus/QDBusUnixFileDescriptor>

namespace vx {
VOXIECLIENT_EXPORT QDBusSignature
dbusGetArgumentSignature(const QDBusArgument& arg);
VOXIECLIENT_EXPORT QDBusSignature
dbusGetVariantSignature(const QDBusVariant& variant);

template <typename T>
inline T dbusGetArgumentValueUnchecked(const QDBusArgument& argument);

namespace DBusUtilIntern {
template <typename T>
struct DBusTypeTraits;
}

template <typename T>
inline QString dbusGetSignatureString() {
  static QString sig = DBusUtilIntern::DBusTypeTraits<T>::makeSignature();
  return sig;
}
template <typename T>
inline QDBusSignature dbusGetSignature() {
  return QDBusSignature(dbusGetSignatureString<T>());
}

namespace DBusUtilIntern {

inline void assertAtEnd(const QDBusArgument& arg) {
  if (!arg.atEnd())
    throw Exception("de.uni_stuttgart.Voxie.Error",
                    "Unexpected data remaining in DBus complex type");
}
inline void assertNotAtEnd(const QDBusArgument& arg) {
  if (arg.atEnd())
    throw Exception("de.uni_stuttgart.Voxie.Error",
                    "Unexpected end of data in DBus complex type");
}
inline void assertType(const QDBusArgument& arg,
                       QDBusArgument::ElementType type) {
  if (arg.currentType() != type)
    throw Exception("de.uni_stuttgart.Voxie.Error",
                    "Unexpected type in DBus argument, expected " +
                        QString::number(type) + ", got " +
                        QString::number(arg.currentType()));
}

#define TYPE(sig, QtType, ty, argtype)                                     \
  template <>                                                              \
  struct DBusTypeTraits<ty> {                                              \
    static QString makeSignature() { return sig; }                         \
                                                                           \
    static ty getValue(const QDBusArgument& arg) {                         \
      assertType(arg, QDBusArgument::argtype);                             \
      /* TODO: check actual type? */                                       \
      QtType value;                                                        \
      arg >> value;                                                        \
      return (ty)value;                                                    \
    }                                                                      \
                                                                           \
    static ty getValueFromVariant(const QVariant& variant) {               \
      if (variant.userType() != qMetaTypeId<QtType>()) {                   \
        throw Exception("de.uni_stuttgart.Voxie.Error",                    \
                        QString() + "Unexpected DBus variant, expected " + \
                            QMetaType::typeName(qMetaTypeId<QtType>()) +   \
                            ", got " +                                     \
                            QMetaType::typeName(variant.userType()));      \
      }                                                                    \
      return variant.value<QtType>();                                      \
    }                                                                      \
  };

TYPE("y", quint8, quint8, BasicType)
TYPE("b", bool, bool, BasicType)
TYPE("n", qint16, qint16, BasicType)
TYPE("q", quint16, quint16, BasicType)
TYPE("i", qint32, qint32, BasicType)
TYPE("u", quint32, quint32, BasicType)
TYPE("x", qint64, qint64, BasicType)
TYPE("t", quint64, quint64, BasicType)
TYPE("d", double, double, BasicType)
TYPE("h", QDBusUnixFileDescriptor, QDBusUnixFileDescriptor,
     BasicType)  // TODO: is this BasicType?

TYPE("s", QString, QString, BasicType)
TYPE("o", QDBusObjectPath, QDBusObjectPath, BasicType)
TYPE("g", QDBusSignature, QDBusSignature, BasicType)

TYPE("v", QDBusVariant, QDBusVariant, VariantType)

TYPE("ay", QByteArray, QByteArray, BasicType)

// Note: ty has to be QList<QString> and not QStringList, otherwise getting a
// QList<QString> from a DBus variant will fail with "Unexpected DBus variant,
// expected a complex type (QDBusArgument),got QStringList" TYPE("as",
// QStringList, QStringList, ArrayType)
TYPE("as", QStringList, QList<QString>, ArrayType)
#undef TYPE

template <typename T>
struct DBusTypeTraits<QList<T>> {
  static_assert(!std::is_same<T, uint8_t>::value,
                "Use QByteArray instead of QList<byte>");

  static QString makeSignature() { return "a" + dbusGetSignatureString<T>(); }

  static QList<T> getValue(const QDBusArgument& arg) {
    assertType(arg, QDBusArgument::ArrayType);
    QList<T> result;
    arg.beginArray();
    while (!arg.atEnd()) result.append(dbusGetArgumentValueUnchecked<T>(arg));
    arg.endArray();
    return result;
  }

  static QList<T> getValueFromVariant(const QVariant& variant) {
    throw Exception("de.uni_stuttgart.Voxie.Error",
                    QString() +
                        "Unexpected DBus variant, expected a complex "
                        "type (QDBusArgument),got " +
                        QMetaType::typeName(variant.userType()));
  }
};

template <typename K, typename V>
struct DBusTypeTraits<QMap<K, V>> {
  static QString makeSignature() {
    return "a{" + dbusGetSignatureString<K>() + dbusGetSignatureString<V>() +
           "}";
  }

  static QMap<K, V> getValue(const QDBusArgument& arg) {
    assertType(arg, QDBusArgument::MapType);
    QMap<K, V> result;
    arg.beginMap();
    while (!arg.atEnd()) {
      arg.beginMapEntry();
      auto key = dbusGetArgumentValueUnchecked<K>(arg);
      auto value = dbusGetArgumentValueUnchecked<V>(arg);
      result.insert(key, value);
      arg.endMapEntry();
    }
    arg.endMap();
    return result;
  }

  static QMap<K, V> getValueFromVariant(const QVariant& variant) {
    throw Exception("de.uni_stuttgart.Voxie.Error",
                    QString() +
                        "Unexpected DBus variant, expected a complex "
                        "type (QDBusArgument),got " +
                        QMetaType::typeName(variant.userType()));
  }
};

template <typename... T>
struct StructImpl;
template <>
struct StructImpl<> {
  static void addToString(QString& str) { Q_UNUSED(str); }
  static void getValue(const QDBusArgument& arg) { assertAtEnd(arg); }
};
template <typename T, typename... Tail>
struct StructImpl<T, Tail...> {
  static void addToString(QString& str) {
    str += dbusGetSignatureString<T>();
    StructImpl<Tail...>::addToString(str);
  }
  static void getValue(const QDBusArgument& arg, T& result, Tail&... tail) {
    assertNotAtEnd(arg);
    result = dbusGetArgumentValueUnchecked<T>(arg);
    StructImpl<Tail...>::getValue(arg, tail...);
  }
};

template <typename... T>
struct DBusTypeTraits<std::tuple<T...>> {
  static_assert(std::tuple_size<std::tuple<T...>>::value != 0,
                "DBus struct cannot be empty");

  static QString makeSignature() {
    QString str = "(";
    StructImpl<T...>::addToString(str);
    str += ")";
    return str;
  }

  template <std::size_t... Is>
  static void getValue2(const QDBusArgument& arg, std::tuple<T...>& result,
                        std::index_sequence<Is...>) {
    StructImpl<T...>::getValue(arg, std::get<Is>(result)...);
  }

  static std::tuple<T...> getValue(const QDBusArgument& arg) {
    assertType(arg, QDBusArgument::StructureType);
    arg.beginStructure();
    std::tuple<T...> result;
    getValue2(arg, result, std::index_sequence_for<T...>{});
    arg.endStructure();
    return result;
  }

  static std::tuple<T...> getValueFromVariant(const QVariant& variant) {
    throw Exception("de.uni_stuttgart.Voxie.Error",
                    QString() +
                        "Unexpected DBus variant, expected a complex "
                        "type (QDBusArgument),got " +
                        QMetaType::typeName(variant.userType()));
  }
};
}  // namespace DBusUtilIntern

template <typename T>
inline T dbusGetArgumentValueUnchecked(const QDBusArgument& argument) {
  return DBusUtilIntern::DBusTypeTraits<T>::getValue(argument);
}
template <typename T>
inline T dbusGetArgumentValue(const QDBusArgument& argument) {
  {
    QDBusSignature expected = dbusGetSignature<T>();
    QDBusSignature actual = dbusGetArgumentSignature(argument);
    if (expected != actual)
      throw Exception("de.uni_stuttgart.Voxie.Error",
                      "Got unexpected DBus variant value, expected '" +
                          expected.signature() + "', got '" +
                          actual.signature() + "'");
  }
  return dbusGetArgumentValueUnchecked<T>(argument);
}

template <typename T>
inline T dbusGetVariantValue(const QDBusVariant& variant) {
  auto var = variant.variant();
  if (var.userType() == qMetaTypeId<QDBusArgument>()) {
    QDBusArgument arg = var.value<QDBusArgument>();
    return dbusGetArgumentValue<T>(arg);
  }
  // Is not a QDBusArgument, expect it is the QtType for this type
  return DBusUtilIntern::DBusTypeTraits<T>::getValueFromVariant(var);
}

// Note: The returned QDBusVariant cannot be passed to dbusGetVariantValue(), it
// only can be send over DBus. The reason for this is that it contains a
// 'marshalling' QDBusArgument, while dbusGetVariantValue can only deal with
// 'demarshalling' QDBusArguments. Turning a 'marshalling' QDBusArguments into a
// 'demarshalling' one seems to be quite difficult (probably would require
// sending it on a connection to a local method, then Qt will do this).
template <typename T>
inline QDBusVariant dbusMakeVariant(const T& value) {
  // Add special cases for int etc. here were a variant containing an int
  // is returned?
  // Does not seem to be needed and might cause problems if the decision is made
  // incorrectly.
  QDBusArgument arg;
  arg << value;
  return QDBusVariant(QVariant::fromValue(arg));
}

/*
This code would turn a segmentation fault into an error "[Warning]
QDBusConnection: error: could not send reply message to service "": Marshalling
failed: Invalid signature passed in arguments"
See https://bugreports.qt.io/browse/QTBUG-124919

template <>
inline QDBusVariant dbusMakeVariant<QDBusSignature>(
  const QDBusSignature& value) {
return QDBusVariant(QVariant::fromValue(value));
}
*/

VOXIECLIENT_EXPORT QList<quint8> fromByteArray(const QByteArray& data);
VOXIECLIENT_EXPORT QByteArray toByteArray(const QList<quint8>& data);

VOXIECLIENT_EXPORT QList<QList<quint8>> fromByteArray(
    const QList<QByteArray>& data);
VOXIECLIENT_EXPORT QList<QByteArray> toByteArray(
    const QList<QList<quint8>>& data);

}  // namespace vx
