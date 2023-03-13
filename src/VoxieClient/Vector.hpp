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

#ifndef VX_VECTOR_ENABLE_QDEBUG
#define VX_VECTOR_ENABLE_QDEBUG 1
#endif

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
// This file contains for loops like this (where dim is a template parameter):
// for (size_t i = 0; i < dim; i++)
// On gcc 10.2.1 this will cause -Wtype-limits errors when the function is
// instantiated with dim=0.
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif

#include <VoxieClient/TypeTraits.hpp>

#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <ostream>
#include <type_traits>

#if VX_VECTOR_ENABLE_QDEBUG
#include <QtCore/QDebug>
#endif

namespace vx {

template <typename T>
struct RingTraits {
  // Whether a multiplicative inverse exists for all non-zero elements
  static constexpr bool isDivisionRing = !std::numeric_limits<T>::is_integer;
};

namespace intern {

// This base class is needed to create constructors with the correct number of
// arguments
template <typename T, typename... U>
class VectorBase {
  // Note: This must not be called 'dim' because at least MSVC2017 and MSVC2019
  // don't like it if the template parameter for Vector is called the same as
  // the class member here (this would cause msvcTest1() in VectorTest.cpp to
  // fail with:
  // error C2662: 'unknown-type &vx::Vector<quint64,3>::asArray(void)': cannot
  // convert 'this' pointer from 'const vx::Vector<quint64,3>' to
  // 'vx::Vector<quint64,3> &'
  static constexpr std::size_t dim_ = sizeof...(U);

 protected:
  std::array<T, dim_> data_;

 public:
  VectorBase() = default;
  VectorBase(U... par) : data_{par...} {}
};
// Specialization for 0-element vector
template <typename T>
class VectorBase<T> {
 protected:
  std::array<T, 0> data_;

 public:
  VectorBase() = default;
};

template <typename T, std::size_t i, typename... U>
struct VectorBaseType : VectorBaseType<T, i - 1, T, U...> {};
template <typename T, typename... U>
struct VectorBaseType<T, 0, U...> {
  using type = VectorBase<T, U...>;
};

}  // namespace intern

template <typename T, std::size_t dim>
class Vector : vx::intern::VectorBaseType<T, dim>::type {
  using BaseType = typename vx::intern::VectorBaseType<T, dim>::type;
  using BaseType::BaseType;

 public:
  const std::array<T, dim>& asArray() const { return this->data_; }
  std::array<T, dim>& asArray() { return this->data_; }

  const T& operator[](size_t pos) const { return asArray()[pos]; }
  T& operator[](size_t pos) { return asArray()[pos]; }

  const T& operator()(size_t pos) const { return asArray()[pos]; }
  T& operator()(size_t pos) { return asArray()[pos]; }

  template <std::size_t i>
  const T& access() const {
    static_assert(i < dim, "Index out of range");
    return asArray()[i];
  }
  template <std::size_t i>
  T& access() {
    static_assert(i < dim, "Index out of range");
    return asArray()[i];
  }

  friend Vector operator+(const Vector& v) { return v; }
  friend Vector operator-(const Vector& v) {
    Vector ret;
    for (size_t i = 0; i < dim; i++) ret[i] = -v[i];
    return ret;
  }

  friend Vector operator+(const Vector& v1, const Vector& v2) {
    Vector ret;
    for (size_t i = 0; i < dim; i++) ret[i] = v1[i] + v2[i];
    return ret;
  }
  friend Vector& operator+=(Vector& v1, const Vector& v2) {
    for (size_t i = 0; i < dim; i++) v1[i] += v2[i];
    return v1;
  }

  friend Vector operator-(const Vector& v1, const Vector& v2) {
    Vector ret;
    for (size_t i = 0; i < dim; i++) ret[i] = v1[i] - v2[i];
    return ret;
  }
  friend Vector& operator-=(Vector& v1, const Vector& v2) {
    for (size_t i = 0; i < dim; i++) v1[i] -= v2[i];
    return v1;
  }

  friend Vector operator*(T s, const Vector& v) {
    Vector ret;
    for (size_t i = 0; i < dim; i++) ret[i] = s * v[i];
    return ret;
  }
  friend Vector operator*(const Vector& v, T s) {
    Vector ret;
    for (size_t i = 0; i < dim; i++) ret[i] = v[i] * s;
    return ret;
  }
  friend Vector& operator*=(Vector& v, T s) {
    for (size_t i = 0; i < dim; i++) v[i] *= s;
    return v;
  }

  // This is a template to prevent it being instantiated for non-division-ring
  // types
  template <typename AllowNonDivisionRing = std::false_type>
  friend Vector operator/(const Vector& v, T s) {
    // TODO: Should this be allowed?
    static_assert(
        vx::RingTraits<T>::isDivisionRing || AllowNonDivisionRing::value,
        "operator/ cannot be used for vector module over non-division "
        "ring");

    Vector ret;
    for (size_t i = 0; i < dim; i++) ret[i] = v[i] / s;
    return ret;
  }
  // This is a template to prevent it being instantiated for non-division-ring
  // types
  template <typename AllowNonDivisionRing = std::false_type>
  friend Vector& operator/=(Vector& v, T s) {
    // TODO: Should this be allowed?
    static_assert(
        vx::RingTraits<T>::isDivisionRing || AllowNonDivisionRing::value,
        "operator/ cannot be used for vector module over non-division "
        "ring");

    for (size_t i = 0; i < dim; i++) v[i] /= s;
    return v;
  }

  friend bool operator==(const Vector& v1, const Vector& v2) {
    for (size_t i = 0; i < dim; i++)
      if (v1[i] != v2[i]) return false;
    return true;
  }
  friend bool operator!=(const Vector& v1, const Vector& v2) {
    return !(v1 == v2);
  }

  friend std::ostream& operator<<(std::ostream& o, const Vector& v) {
    o << "(";
    for (size_t i = 0; i < dim; i++) {
      if (i != 0) o << ", ";
      o << v[i];
    }
    o << ")";
    return o;
  }

#if VX_VECTOR_ENABLE_QDEBUG
  friend QDebug operator<<(QDebug debug, const Vector& v) {
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug << "(";
    for (size_t i = 0; i < dim; i++) {
      if (i != 0) debug << ", ";
      debug << v[i];
    }
    debug << ")";
    return debug;
  }
#endif

  static Vector zero() {
    Vector ret;
    for (size_t i = 0; i < dim; i++) ret(i) = 0;
    return ret;
  }
};

template <typename T, std::size_t dim>
inline Vector<T, dim> toVector(const std::array<T, dim>& array) {
  Vector<T, dim> ret;
  for (size_t i = 0; i < dim; i++) ret[i] = array[i];
  return ret;
}

template <typename To, typename From, std::size_t dim>
inline std::enable_if_t<IsConvertibleWithoutNarrowing<From, To>::value,
                        Vector<To, dim>>
vectorCast(const Vector<From, dim>& v) {
  Vector<To, dim> ret;
  for (size_t i = 0; i < dim; i++) ret[i] = v[i];
  return ret;
}

template <typename To, typename From, std::size_t dim>
inline std::enable_if_t<std::is_convertible<From, To>::value, Vector<To, dim>>
vectorCastNarrow(const Vector<From, dim>& v) {
  Vector<To, dim> ret;
  for (size_t i = 0; i < dim; i++) ret[i] = static_cast<To>(v[i]);
  return ret;
}

template <typename T, std::size_t dim>
inline T squaredNorm(const Vector<T, dim>& v) {
  T sum = 0;
  for (size_t i = 0; i < dim; i++) sum += v(i) * v(i);
  return sum;
}

template <typename T, std::size_t dim,
          typename AllowNonDivisionRing = std::false_type>
inline Vector<T, dim> normalize(const Vector<T, dim>& v,
                                AllowNonDivisionRing = AllowNonDivisionRing()) {
  using std::sqrt;
  return v / sqrt(squaredNorm(v));
}

template <typename T, std::size_t dim>
inline T dotProduct(const Vector<T, dim>& v1, const Vector<T, dim>& v2) {
  T result = 0;
  for (size_t i = 0; i < dim; i++) result += v1[i] * v2[i];
  return result;
}

template <typename T>
inline Vector<T, 3> crossProduct(const Vector<T, 3>& v1,
                                 const Vector<T, 3>& v2) {
  return Vector<T, 3>{v1[1] * v2[2] - v1[2] * v2[1],
                      v1[2] * v2[0] - v1[0] * v2[2],
                      v1[0] * v2[1] - v1[1] * v2[0]};
}

template <typename T, std::size_t dim>
inline Vector<T, dim> elementwiseProduct(const Vector<T, dim>& v1,
                                         const Vector<T, dim>& v2) {
  Vector<T, dim> result;
  for (size_t i = 0; i < dim; i++) result[i] = v1[i] * v2[i];
  return result;
}

template <typename T, std::size_t dim1, std::size_t dim2>
static Vector<T, dim1 + dim2> concat(const Vector<T, dim1>& b1,
                                     const Vector<T, dim2>& b2) {
  Vector<T, dim1 + dim2> ret;

  for (size_t i = 0; i < dim1; i++) ret(i) = b1(i);
  for (size_t i = 0; i < dim2; i++) ret(dim1 + i) = b2(i);

  return ret;
}
}  // namespace vx

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
