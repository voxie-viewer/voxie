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

#include <VoxieBackend/Property/PropertyType.hpp>

#include <QString>
#include <QStringList>
#include <QVariant>

#include <tuple>
#include <type_traits>

// Note: the functions in this header are implementation details of Types.cpp
// and not intended to be called directly. Use vx::PropertyType::valueToString
// to convert dynamically typed data objects to strings.

namespace vx {

// Tuple iteration function (recursive step)
template <std::size_t RevIndex, typename... T>
struct StringConversionHelperTuple {
  template <typename Func>
  static void tupleForEach(const std::tuple<T...>& tuple, const Func& func) {
    // Invoke callback function on current value
    func(std::get<sizeof...(T) - RevIndex>(tuple));

    // Invoke iteration function for next index
    StringConversionHelperTuple<RevIndex - 1, T...>::tupleForEach(tuple, func);
  }
};

// Tuple iteration function (recursive tail)
template <typename... T>
struct StringConversionHelperTuple<0, T...> {
  template <typename Func>
  static void tupleForEach(const std::tuple<T...>&, const Func&) {}
};

// String conversion fallback (empty string)
template <typename T>
struct StringConversionHelper {
  using FallbackTag = void;
  static QString toString(const T&) { return QString(); }
};

// String conversion specialization (string)
template <>
struct StringConversionHelper<QString> {
  static QString toString(const QString& value) { return value; }
};

// String conversion specialization (unsigned int)
template <>
struct StringConversionHelper<quint64> {
  static QString toString(quint64 value) { return QString::number(value); }
};

// String conversion specialization (signed int)
template <>
struct StringConversionHelper<qint64> {
  static QString toString(qint64 value) { return QString::number(value); }
};

// String conversion specialization (single-precision floating point value)
template <>
struct StringConversionHelper<float> {
  static QString toString(float value) {
    return QString::number(value, 'g', 10);
  }
};

// String conversion specialization (double-precision floating point value)
template <>
struct StringConversionHelper<double> {
  static QString toString(double value) {
    return QString::number(value, 'g', 10);
  }
};

// String conversion specialization (tuple)
template <typename... T>
struct StringConversionHelper<std::tuple<T...>> {
  static QString toString(const std::tuple<T...>& tuple) {
    QStringList elements;
    StringConversionHelperTuple<sizeof...(T), T...>::tupleForEach(
        tuple, [&](const auto& value) {
          using ElemType = typename std::decay<decltype(value)>::type;
          elements.append(StringConversionHelper<ElemType>::toString(value));
        });
    return "[" + elements.join("; ") + "]";
  }
};

// Trait class to determine string conversion template specialization status
template <typename T, typename = void>
struct StringConversionFallbackHelper {
  static constexpr bool isFallback = false;
};

template <typename T>
struct StringConversionFallbackHelper<
    T, typename StringConversionHelper<T>::FallbackTag> {
  static constexpr bool isFallback = true;
};

}  // namespace vx
