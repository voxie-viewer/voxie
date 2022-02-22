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

#include <QtDBus/QDBusArgument>
#include <QtDBus/QDBusObjectPath>

#include <QtCore/qglobal.h>

namespace vx {
namespace DBusUtilIntern {
template <typename T, size_t c>
struct TupleVectorImpl {
  template <typename... Tail>
  static std::tuple<T, Tail...> addT(std::tuple<Tail...>);

  typedef typename TupleVectorImpl<T, c - 1>::type prev;
  typedef decltype(addT(*(prev*)nullptr)) type;
};
template <typename T>
struct TupleVectorImpl<T, 0> {
  typedef std::tuple<> type;
};
}  // namespace DBusUtilIntern
template <typename T, size_t c>
using TupleVector = typename DBusUtilIntern::TupleVectorImpl<T, c>::type;

namespace DBusUtilIntern {
// Useful e.g. for Q_DECLARE_METATYPE()
// https://stackoverflow.com/questions/32314575/q-declare-metatype-a-boostmulti-array
template <typename T>
struct identity_type;
template <typename T>
struct identity_type<void(T)> {
  typedef T type;
};
#define VX_IDENTITY_TYPE(T) \
  typename ::vx::DBusUtilIntern::identity_type<void T>::type
// Usage e.g.: Q_DECLARE_METATYPE(VX_IDENTITY_TYPE((QMap<K, V>)))
}  // namespace DBusUtilIntern

typedef std::tuple<QMap<QString, QDBusVariant>, qint64,
                   std::tuple<QString, quint32, QString>, std::tuple<quint64>,
                   std::tuple<qint64>, QMap<QString, QDBusVariant>>
    Array1InfoDBus;
typedef std::tuple<QMap<QString, QDBusVariant>, qint64,
                   std::tuple<QString, quint32, QString>,
                   std::tuple<quint64, quint64>, std::tuple<qint64, qint64>,
                   QMap<QString, QDBusVariant>>
    Array2InfoDBus;
typedef std::tuple<
    QMap<QString, QDBusVariant>, qint64, std::tuple<QString, quint32, QString>,
    std::tuple<quint64, quint64, quint64>, std::tuple<qint64, qint64, qint64>,
    QMap<QString, QDBusVariant>>
    Array3InfoDBus;

namespace DBusUtilIntern {
template <typename... T>
struct StructMarshallImpl;
template <>
struct StructMarshallImpl<> {
  static void marshall(QDBusArgument& argument) { Q_UNUSED(argument); }
  static void demarshall(const QDBusArgument& argument) { Q_UNUSED(argument); }
};
template <typename T, typename... Tail>
struct StructMarshallImpl<T, Tail...> {
  static void marshall(QDBusArgument& argument, const T& value,
                       const Tail&... tail) {
    argument << value;
    StructMarshallImpl<Tail...>::marshall(argument, tail...);
  }
  static void demarshall(const QDBusArgument& argument, T& value,
                         Tail&... tail) {
    argument >> value;
    StructMarshallImpl<Tail...>::demarshall(argument, tail...);
  }
};
}  // namespace DBusUtilIntern

template <typename... T, std::size_t... Is>
static void marshall(QDBusArgument& arg, const std::tuple<T...>& result,
                     std::index_sequence<Is...>) {
  vx::DBusUtilIntern::StructMarshallImpl<T...>::marshall(
      arg, std::get<Is>(result)...);
}
template <typename... T, std::size_t... Is>
static void demarshall(const QDBusArgument& arg, std::tuple<T...>& result,
                       std::index_sequence<Is...>) {
  vx::DBusUtilIntern::StructMarshallImpl<T...>::demarshall(
      arg, std::get<Is>(result)...);
}
}  // namespace vx

template <typename... T>
static inline QDBusArgument& operator<<(QDBusArgument& argument,
                                        const std::tuple<T...>& value) {
  argument.beginStructure();
  vx::marshall<T...>(argument, value, std::index_sequence_for<T...>{});
  argument.endStructure();
  return argument;
}
template <typename... T>
static inline const QDBusArgument& operator>>(const QDBusArgument& argument,
                                              std::tuple<T...>& value) {
  argument.beginStructure();
  vx::demarshall<T...>(argument, value, std::index_sequence_for<T...>{});
  argument.endStructure();
  return argument;
}
