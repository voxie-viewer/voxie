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

#ifndef VX_QUATERNION_ENABLE_QDEBUG
#define VX_QUATERNION_ENABLE_QDEBUG 1
#endif

#include <array>
#include <cstddef>
#include <ostream>

#if VX_QUATERNION_ENABLE_QDEBUG
#include <QtCore/QDebug>
#endif

#include <VoxieClient/Vector.hpp>

namespace vx {
template <typename T>
class Quaternion {
  std::array<T, 4> data_;

 public:
  Quaternion(T a, T b, T c, T d) : data_{a, b, c, d} {}
  explicit Quaternion(const std::array<T, 4>& data) : data_(data) {}

  const std::array<T, 4>& asArray() const { return this->data_; }
  std::array<T, 4>& asArray() { return this->data_; }

  const T& a() const { return asArray()[0]; }
  T& a() { return asArray()[0]; }

  const T& b() const { return asArray()[1]; }
  T& b() { return asArray()[1]; }

  const T& c() const { return asArray()[2]; }
  T& c() { return asArray()[2]; }

  const T& d() const { return asArray()[3]; }
  T& d() { return asArray()[3]; }

  static Quaternion zero() { return Quaternion(0, 0, 0, 0); }
  static Quaternion one() { return Quaternion(1, 0, 0, 0); }

  Quaternion conjugate() const { return Quaternion(a(), -b(), -c(), -d()); }

  friend Quaternion operator+(const Quaternion& q) { return q; }
  friend Quaternion operator-(const Quaternion& q) {
    return Quaternion(-q.a(), -q.b(), -q.c(), -q.d());
  }

  friend Quaternion operator+(const Quaternion& q1, const Quaternion& q2) {
    return Quaternion(q1.a() + q2.a(), q1.b() + q2.b(), q1.c() + q2.c(),
                      q1.d() + q2.d());
  }
  friend Quaternion& operator+=(Quaternion& q, const Quaternion& q2) {
    q.a() += q2.a();
    q.b() += q2.b();
    q.c() += q2.c();
    q.d() += q2.d();
    return q;
  }

  friend Quaternion operator-(const Quaternion& q1, const Quaternion& q2) {
    return Quaternion(q1.a() - q2.a(), q1.b() - q2.b(), q1.c() - q2.c(),
                      q1.d() - q2.d());
  }
  friend Quaternion& operator-=(Quaternion& q, const Quaternion& q2) {
    q.a() -= q2.a();
    q.b() -= q2.b();
    q.c() -= q2.c();
    q.d() -= q2.d();
    return q;
  }

  friend Quaternion operator*(const Quaternion& q, T scalar) {
    return Quaternion(q.a() * scalar, q.b() * scalar, q.c() * scalar,
                      q.d() * scalar);
  }
  friend Quaternion operator*(T scalar, const Quaternion& q) {
    return Quaternion(scalar * q.a(), scalar * q.b(), scalar * q.c(),
                      scalar * q.d());
  }
  friend Quaternion& operator*=(Quaternion& q, T scalar) {
    q.a() *= scalar;
    q.b() *= scalar;
    q.c() *= scalar;
    q.d() *= scalar;
    return q;
  }

  friend Quaternion operator*(const Quaternion& q1, const Quaternion& q2) {
    return Quaternion(
        q1.a() * q2.a() - q1.b() * q2.b() - q1.c() * q2.c() - q1.d() * q2.d(),
        q1.a() * q2.b() + q1.b() * q2.a() + q1.c() * q2.d() - q1.d() * q2.c(),
        q1.a() * q2.c() - q1.b() * q2.d() + q1.c() * q2.a() + q1.d() * q2.b(),
        q1.a() * q2.d() + q1.b() * q2.c() - q1.c() * q2.b() + q1.d() * q2.a());
  }
  friend Quaternion& operator*=(Quaternion& q, const Quaternion& q2) {
    return q = q * q2;
  }

  friend Quaternion operator/(const Quaternion& q, T scalar) {
    return Quaternion(q.a() / scalar, q.b() / scalar, q.c() / scalar,
                      q.d() / scalar);
  }
  friend Quaternion& operator/=(Quaternion& q, T scalar) {
    q.a() /= scalar;
    q.b() /= scalar;
    q.c() /= scalar;
    q.d() /= scalar;
    return q;
  }

  friend bool operator==(const Quaternion& q1, const Quaternion& q2) {
    return q1.a() == q2.a() && q1.b() == q2.b() && q1.c() == q2.c() &&
           q1.d() == q2.d();
  }
  friend bool operator!=(const Quaternion& q1, const Quaternion& q2) {
    return !(q1 == q2);
  }

  friend std::ostream& operator<<(std::ostream& o, const Quaternion& q) {
    o << "(" << q.a() << ", " << q.b() << ", " << q.c() << ", " << q.d() << ")";
    return o;
  }

#if VX_QUATERNION_ENABLE_QDEBUG
  friend QDebug operator<<(QDebug debug, const Quaternion& q) {
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug << "(" << q.a() << ", " << q.b() << ", " << q.c() << ", " << q.d()
          << ")";
    return debug;
  }
#endif
};

template <typename To, typename From>
inline std::enable_if_t<IsConvertibleWithoutNarrowing<From, To>::value,
                        Quaternion<To>>
quaternionCast(const Quaternion<From>& q) {
  return Quaternion<To>(q.a(), q.b(), q.c(), q.d());
}

template <typename To, typename From>
inline std::enable_if_t<std::is_convertible<From, To>::value, Quaternion<To>>
quaternionCastNarrow(const Quaternion<From>& q) {
  return Quaternion<To>(static_cast<To>(q.a()), static_cast<To>(q.b()),
                        static_cast<To>(q.c()), static_cast<To>(q.d()));
}

template <typename T>
inline T squaredNorm(const Quaternion<T>& q) {
  return q.a() * q.a() + q.b() * q.b() + q.c() * q.c() + q.d() * q.d();
}
}  // namespace vx
