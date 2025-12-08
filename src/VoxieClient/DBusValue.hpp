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

#pragma once

#include <VoxieClient/ToStringImpl.hpp>
#include <VoxieClient/VoxieClient.hpp>

#include <cstdint>
#include <memory>
#include <vector>

// TODO: Replace QDBusSignature with vx::DBusSignatureBase? Also when using it
// as a DBus value? Introduce a DBusSignature class which wraps a
// std::vector<std::shared_ptr<DBusSignatureBase>>?

class QByteArray;
class QDBusArgument;
class QDBusObjectPath;
class QDBusSignature;
class QDBusUnixFileDescriptor;
class QDBusVariant;
class QStringList;

namespace vx {
class DBusSignatureBase;

class DBusValue;
class DBusVariant;

enum class DBusTypeKind : std::uint8_t {
  // Includes the fixed basic types (e.g. quint32, bool), the string-like basic
  // types (QString, QDBusSignature, QDBusObjectPath) and
  // QDBusUnixFileDescriptor
  // Note that 'ay' is treated as 'Array', not 'Basic'.
  Basic,

  // A VARIANT ('v')
  Variant,

  // An ARRAY of a type which is not DICT_ENTRY.
  Array,

  // An ARRAY of type DICT_ENTRY.
  Dictionary,

  // A DBus STRUCT.
  Struct,
};

// Container for a DBus value when the type is not known at compile time.
//
// Values are immutable.
class VOXIECLIENT_EXPORT DBusValue {
  // TODO: Use a std::variant or something like that to avoid memory allocations
  // for fixed basic values?

  class Instance {
   public:
    virtual ~Instance();

    virtual DBusTypeKind kind() const = 0;
    virtual QDBusSignature signature() const = 0;

    virtual void marshalToQDBusArgument(QDBusArgument& arg) const = 0;
    virtual bool equals(const Instance* other) const = 0;
  };

  template <typename T>
  struct BasicTypeTraits;

  class InstanceBasic;
  template <typename T>
  class InstanceBasicInst;
  class InstanceVariant;
  class InstanceArray;
  class InstanceDictionary;
  class InstanceStruct;

  // TODO: Should this use std::map? This would require implementing DBusValue
  // comparisons properly (including ordered comparisons)
  // using DictionaryType = std::map<DBusValue, DBusValue>;
  using DictionaryType = std::vector<std::tuple<DBusValue, DBusValue>>;

  std::shared_ptr<const Instance> instance;

  explicit DBusValue(std::shared_ptr<const Instance>&& instance)
      : instance(std::move(instance)) {}

  template <typename T>
  const T& getBasicValueInst() const;

 public:
  DBusTypeKind kind() const { return instance->kind(); }
  QDBusSignature signature() const;

  // TODO: Decouple values for T from Qt types and use e.g. std::uint64_t
  // instead?
  template <typename T>
  static DBusValue createBasic(const T& value) {
    static_assert(BasicTypeTraits<T>::valid, "Invalid type for Basic type");
    return BasicTypeTraits<T>::create(value);
  }

  // Returns the value of a basic type.
  // Throws for non-basic types or if the signature of T does not match.
  // T must be one of the possible basic types (quint8, qint16, quint16, qint32,
  // quint32, qint64, quint64, bool, double, QString, QDBusObjectPath,
  // QDBusSignature, QDBusUnixFileDescriptor).
  template <typename T>
  const T& getBasicValue() const {
    // Note: This method is defined inline so that this static_assert is always
    // evaluated.
    static_assert(BasicTypeTraits<T>::valid, "Invalid type for Basic type");

    return this->getBasicValueInst<T>();
  }
  char getBasicSignatureChar() const;

  static DBusValue createVariant(const DBusVariant& variant);

  // Returns the value of a variant.
  // Throws for non-variants.
  const DBusVariant& getVariant() const;

  static DBusValue createArray(QDBusSignature elementType,
                               const std::vector<DBusValue>& values);

  // TODO: Return values in a different form?
  const std::vector<DBusValue>& getArray() const;
  const QDBusSignature& getArrayElementType() const;

  static DBusValue createDictionary(QDBusSignature keyType,
                                    QDBusSignature valueType,
                                    const DictionaryType& values);

  // TODO: Return values in a different form?
  const DictionaryType& getDictionary() const;
  const QDBusSignature& getDictionaryKeyType() const;
  const QDBusSignature& getDictionaryValueType() const;

  static DBusValue createStruct(const std::vector<DBusValue>& values);

  // TODO: Return values in a different form?
  const std::vector<DBusValue>& getStruct() const;

  static DBusValue demarshalFromQDBusArgument(
      const std::shared_ptr<DBusSignatureBase>& signature,
      const QDBusArgument& value);
  // Add the value to arg
  void marshalToQDBusArgument(QDBusArgument& arg) const;

  static DBusValue fromQByteArray(const QByteArray& value);
  static DBusValue fromQStringList(const QStringList& value);

  friend VOXIECLIENT_EXPORT bool operator==(const DBusValue& v1,
                                            const DBusValue& v2);
  friend VOXIECLIENT_EXPORT bool operator!=(const DBusValue& v1,
                                            const DBusValue& v2);
};
VOXIECLIENT_EXPORT bool operator==(const DBusValue& v1, const DBusValue& v2);
VOXIECLIENT_EXPORT bool operator!=(const DBusValue& v1, const DBusValue& v2);

#define VX_DBUS_FOR_EACH_BASIC_TYPE(MACRO) \
  MACRO('y', quint8)                       \
  MACRO('b', bool)                         \
  MACRO('n', qint16)                       \
  MACRO('q', quint16)                      \
  MACRO('i', qint32)                       \
  MACRO('u', quint32)                      \
  MACRO('x', qint64)                       \
  MACRO('t', quint64)                      \
  MACRO('d', double)                       \
  MACRO('h', QDBusUnixFileDescriptor)      \
  MACRO('s', QString)                      \
  MACRO('o', QDBusObjectPath)              \
  MACRO('g', QDBusSignature)

template <typename T>
struct VOXIECLIENT_EXPORT DBusValue::BasicTypeTraits {
  static const bool valid = false;
};

#define TYPE(sig, T)                                        \
  template <>                                               \
  struct VOXIECLIENT_EXPORT DBusValue::BasicTypeTraits<T> { \
    static const bool valid = true;                         \
                                                            \
    static DBusValue create(T value);                       \
    static char getSignatureChar() { return sig; };         \
  };
VX_DBUS_FOR_EACH_BASIC_TYPE(TYPE)
#undef TYPE

// Note: The string returned here does not contain the signature of value.
template <>
struct VOXIECLIENT_EXPORT ToStringImpl<DBusValue> {
  static const QString toString(const DBusValue& value);
};

// TODO: Should this be done here?
template <>
struct VOXIECLIENT_EXPORT ToStringImpl<QDBusObjectPath> {
  static const QString toString(const QDBusObjectPath& value);
};
template <>
struct VOXIECLIENT_EXPORT ToStringImpl<QDBusSignature> {
  static const QString toString(const QDBusSignature& value);
};
template <>
struct VOXIECLIENT_EXPORT ToStringImpl<QDBusUnixFileDescriptor> {
  static const QString toString(const QDBusUnixFileDescriptor& value);
};

}  // namespace vx
