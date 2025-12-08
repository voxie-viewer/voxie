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

// TODO: Remove DBusUtil.hpp and rename DBusVoxie* to DBus*?

// TODO: Use vx::DBusSignature instead of QDBusSignature?

#pragma once

#include <VoxieClient/DBusValue.hpp>
#include <VoxieClient/DBusVariant.hpp>
#include <VoxieClient/Exception.hpp>
#include <VoxieClient/VoxieClient.hpp>

#include <QtDBus/QDBusUnixFileDescriptor>

namespace vx {
template <typename T>
inline T dbusVoxieParseValueUnchecked(const vx::DBusValue& value);

template <typename T>
inline vx::DBusValue dbusVoxieBuildValue(const T& value);

namespace DBusUtilIntern {
template <typename T>
struct DBusVoxieTypeTraits;
}

template <typename T>
inline QString dbusVoxieGetSignatureString() {
  static QString sig = DBusUtilIntern::DBusVoxieTypeTraits<T>::makeSignature();
  return sig;
}
template <typename T>
inline QDBusSignature dbusVoxieGetSignature() {
  return QDBusSignature(dbusVoxieGetSignatureString<T>());
}

namespace DBusUtilIntern {
#define TYPE(sig, ty)                                      \
  template <>                                              \
  struct DBusVoxieTypeTraits<ty> {                         \
    static QString makeSignature() {                       \
      char c[2]{sig, 0};                                   \
      return QString(c);                                   \
    }                                                      \
                                                           \
    static ty parseValue(const vx::DBusValue& value) {     \
      return value.getBasicValue<ty>();                    \
    }                                                      \
                                                           \
    static vx::DBusValue buildDBusValue(const ty& value) { \
      return vx::DBusValue::createBasic<ty>(value);        \
    }                                                      \
  };
VX_DBUS_FOR_EACH_BASIC_TYPE(TYPE)
#undef TYPE

template <>
struct DBusVoxieTypeTraits<vx::DBusVariant> {
  static QString makeSignature() { return "v"; }

  static vx::DBusVariant parseValue(const vx::DBusValue& value) {
    return value.getVariant();
  }

  static vx::DBusValue buildDBusValue(const vx::DBusVariant& value) {
    return vx::DBusValue::createVariant(value);
  }
};

template <typename T>
struct DBusVoxieTypeTraits<QList<T>> {
  static QString makeSignature() {
    return "a" + dbusVoxieGetSignatureString<T>();
  }

  static QList<T> parseValue(const vx::DBusValue& value) {
    QList<T> result;
    for (const auto& entry : value.getArray())
      result.append(dbusVoxieParseValueUnchecked<T>(entry));
    return result;
  }

  static vx::DBusValue buildDBusValue(const QList<T>& value) {
    std::vector<vx::DBusValue> entries;
    for (const auto& entry : value)
      entries.push_back(dbusVoxieBuildValue<T>(entry));
    return vx::DBusValue::createArray(dbusVoxieGetSignature<T>(), entries);
  }
};

template <typename K, typename V>
struct DBusVoxieTypeTraits<QMap<K, V>> {
  static QString makeSignature() {
    return "a{" + dbusVoxieGetSignatureString<K>() +
           dbusVoxieGetSignatureString<V>() + "}";
  }

  static QMap<K, V> parseValue(const vx::DBusValue& value) {
    QMap<K, V> result;
    for (const auto& entry : value.getDictionary()) {
      auto key = dbusVoxieParseValueUnchecked<K>(std::get<0>(entry));
      auto entryValue = dbusVoxieParseValueUnchecked<V>(std::get<1>(entry));
      result.insert(key, entryValue);
    }
    return result;
  }

  static vx::DBusValue buildDBusValue(const QMap<K, V>& value) {
    std::vector<std::tuple<vx::DBusValue, vx::DBusValue>> entries;
    for (auto entry = value.begin(); entry != value.end(); ++entry) {
      entries.push_back(std::make_tuple(dbusVoxieBuildValue<K>(entry.key()),
                                        dbusVoxieBuildValue<V>(entry.value())));
    }
    return vx::DBusValue::createDictionary(dbusVoxieGetSignature<K>(),
                                           dbusVoxieGetSignature<V>(), entries);
  }
};

template <typename... T>
struct VoxieStructImpl;
template <>
struct VoxieStructImpl<> {
  static void addToString(QString& str) { Q_UNUSED(str); }
  static void parseValue(const std::vector<DBusValue>& values,
                         size_t position) {
    if (position != values.size())
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Unexpected elements at end of struct");
  }
};
template <typename T, typename... Tail>
struct VoxieStructImpl<T, Tail...> {
  static void addToString(QString& str) {
    str += dbusVoxieGetSignatureString<T>();
    VoxieStructImpl<Tail...>::addToString(str);
  }
  // TODO: Pass position as template parameter?
  static void parseValue(const std::vector<DBusValue>& values, size_t position,
                         T& result, Tail&... tail) {
    result = dbusVoxieParseValueUnchecked<T>(values[position]);
    VoxieStructImpl<Tail...>::parseValue(values, position + 1, tail...);
  }
};

template <size_t size, size_t position, typename... T>
struct VoxieStructImplBuild {
  static void buildDBusValue(std::tuple<T...> value,
                             std::vector<DBusValue>& result) {
    result.push_back(dbusVoxieBuildValue(std::get<position>(value)));
    VoxieStructImplBuild<size, position + 1, T...>::buildDBusValue(value,
                                                                   result);
  }
};
template <size_t size, typename... T>
// Note: This does not work in Debian 11 and earlier, that's why the size
// parameter is needed.
// ("error: template argument ‘sizeof... (T)’ involves template parameter(s)")
// Probably related to <https://gcc.gnu.org/bugzilla/show_bug.cgi?id=96555>.
// struct VoxieStructImplBuild<sizeof...(T), T...> {
struct VoxieStructImplBuild<size, size, T...> {
  static void buildDBusValue(std::tuple<T...> value,
                             std::vector<DBusValue>& result) {
    Q_UNUSED(value);
    Q_UNUSED(result);
  }
};

template <typename... T>
struct DBusVoxieTypeTraits<std::tuple<T...>> {
  static_assert(std::tuple_size<std::tuple<T...>>::value != 0,
                "DBus struct cannot be empty");

  static QString makeSignature() {
    QString str = "(";
    VoxieStructImpl<T...>::addToString(str);
    str += ")";
    return str;
  }

  template <std::size_t... Is>
  static void parseValue2(const std::vector<DBusValue>& values,
                          std::tuple<T...>& result,
                          std::index_sequence<Is...>) {
    VoxieStructImpl<T...>::parseValue(values, 0, std::get<Is>(result)...);
  }

  static std::tuple<T...> parseValue(const vx::DBusValue& value) {
    std::tuple<T...> result;
    parseValue2(value.getStruct(), result, std::index_sequence_for<T...>{});
    return result;
  }

  static vx::DBusValue buildDBusValue(const std::tuple<T...>& value) {
    std::vector<DBusValue> result;
    VoxieStructImplBuild<sizeof...(T), 0, T...>::buildDBusValue(value, result);
    return vx::DBusValue::createStruct(result);
  }
};
}  // namespace DBusUtilIntern

template <typename T>
inline T dbusVoxieParseValueUnchecked(const vx::DBusValue& value) {
  return DBusUtilIntern::DBusVoxieTypeTraits<T>::parseValue(value);
}
template <typename T>
inline T dbusVoxieParseValue(const vx::DBusValue& value) {
  {
    QDBusSignature expected = dbusVoxieGetSignature<T>();
    QDBusSignature actual = value.signature();
    if (expected != actual)
      throw Exception("de.uni_stuttgart.Voxie.Error",
                      "Got unexpected DBus variant value, expected '" +
                          expected.signature() + "', got '" +
                          actual.signature() + "'");
  }
  return dbusVoxieParseValueUnchecked<T>(value);
}

template <typename T>
inline T dbusVoxieParseVariantValue(const vx::DBusVariant& variant) {
  return dbusVoxieParseValue<T>(variant.value());
}

template <typename T>
inline vx::DBusValue dbusVoxieBuildValue(const T& value) {
  return DBusUtilIntern::DBusVoxieTypeTraits<T>::buildDBusValue(value);
}

template <typename T>
inline vx::DBusVariant dbusVoxieBuildVariant(const T& value) {
  return vx::DBusVariant(dbusVoxieBuildValue<T>(value));
}
}  // namespace vx
