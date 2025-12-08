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

#include <VoxieClient/VoxieClient.hpp>

#include <VoxieClient/ToStringImpl.hpp>

#include <QtCore/QString>

#include <QtCore/QMap>

#include <string>
#include <type_traits>
#include <typeinfo>

namespace vx {
namespace Intern {
template <typename T>
struct has_tostringimpl
    : std::integral_constant<
          bool, !std::is_base_of<InvalidToStringImpl, ToStringImpl<T>>::value> {
};

inline QString buildString(const char* s) { return s; }
inline QString buildString(const QString& s) { return s; }
inline QString buildString(const std::string& s) {
  return QString::fromStdString(s);
}
}  // namespace Intern

// TODO: Should these use ToStringImpl or should they be implemented
// differently?

#define DEF(ty)                                \
  template <>                                  \
  struct ToStringImpl<ty> {                    \
    static QString toString(const ty& value) { \
      return QString::number(value);           \
    }                                          \
  };
DEF(qint8);
DEF(quint8);
DEF(qint16);
DEF(quint16);
DEF(qint32);
DEF(quint32);
DEF(qint64);
DEF(quint64);
// TODO: Use different precision?
DEF(double);
#undef DEF

template <>
struct ToStringImpl<bool> {
  static const char* toString(const bool& value) {
    return value ? "true" : "false";
  }
};

// TODO: Should this escape the string?
template <>
struct ToStringImpl<QString> {
  static const QString toString(const QString& value) { return value; }
};

template <>
struct VOXIECLIENT_EXPORT ToStringImpl<std::type_info> {
  static const QString toString(const std::type_info& value);
};

template <typename T>
inline QString toString(const T& value) {
  static_assert(Intern::has_tostringimpl<T>::value,
                "T does not have a valid ToStringImpl");

  return Intern::buildString(ToStringImpl<T>::toString(value));
}

template <typename K, typename V>
struct ToStringImpl<QMap<K, V>> {
  static const QString toString(const QMap<K, V>& value) {
    QString result = "{";
    bool first = true;
    for (auto entry = value.begin(); entry != value.end(); ++entry) {
      if (!first) result += ", ";
      result += vx::toString(entry.key()) + ": " + vx::toString(entry.value());
      first = false;
    }
    result += "}";
    return result;
  }
};
}  // namespace vx
