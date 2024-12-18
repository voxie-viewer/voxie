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

#include <VoxieClient/HmgVector.hpp>

namespace vx {
template <typename T>
class Fraction {
  using HmgVectorType = vx::HmgVector<T, 1>;

  HmgVectorType hmgVector_;

  Fraction(const HmgVectorType& hmgVector) : hmgVector_(hmgVector) {}

 public:
  Fraction() = default;

  Fraction(T numerator, T denominator) {
    this->numerator() = numerator;
    this->denominator() = denominator;
  }

  const HmgVectorType& asHmgVector() const { return hmgVector_; }
  HmgVectorType& asHmgVector() { return hmgVector_; }

  static Fraction fromHmgVector(const HmgVectorType& hmgVector) {
    return Fraction(hmgVector);
  }
  static Fraction fromValue(T value) { return Fraction(value, 1); }

  const T& numerator() const { return asHmgVector().template accessHmg<0>(); }
  T& numerator() { return asHmgVector().template accessHmg<0>(); }

  const T& denominator() const { return asHmgVector().template accessHmg<1>(); }
  T& denominator() { return asHmgVector().template accessHmg<1>(); }

  friend Fraction operator+(const Fraction& v) { return v; }
  friend Fraction operator-(const Fraction& v) {
    // Note: Do not invert the denominator
    return Fraction(-v.numerator(), v.denominator());
  }

  // TODO: Do some normalization for + and - to avoid overflow or underflow when
  // doing many additions?
  friend Fraction operator+(const Fraction& v1, const Fraction& v2) {
    return Fraction(
        v1.numerator() * v2.denominator() + v2.numerator() * v1.denominator(),
        v1.denominator() * v2.denominator());
  }
  friend Fraction& operator+=(Fraction& v1, const Fraction& v2) {
    v1.numerator() =
        v1.numerator() * v2.denominator() + v2.numerator() * v1.denominator();
    v1.denominator() *= v2.denominator();
    return v1;
  }

  friend Fraction operator-(const Fraction& v1, const Fraction& v2) {
    return Fraction(
        v1.numerator() * v2.denominator() - v2.numerator() * v1.denominator(),
        v1.denominator() * v2.denominator());
  }
  friend Fraction& operator-=(Fraction& v1, const Fraction& v2) {
    v1.numerator() =
        v1.numerator() * v2.denominator() - v2.numerator() * v1.denominator();
    v1.denominator() *= v2.denominator();
    return v1;
  }

  /*
  friend Fraction operator*(T s, const Fraction& v) {
    return Fraction(s * v.numerator(), v.denominator());
  }
  friend Fraction operator*(const Fraction& v, T s) {
    return Fraction(v.numerator() * s, v.denominator());
  }
  friend Fraction& operator*=(Fraction& v, T s) {
    v.numerator() *= s;
    return v;
  }
  */
  friend Fraction operator*(const Fraction& v1, const Fraction& v2) {
    return Fraction(v1.numerator() * v2.numerator(),
                    v1.denominator() * v2.denominator());
  }
  friend Fraction& operator*=(Fraction& v1, const Fraction& v2) {
    v1.numerator() *= v2.numerator();
    v1.denominator() *= v2.denominator();
    return v1;
  }

  /*
  friend Fraction operator/(const Fraction& v, T s) {
    return Fraction(v.numerator(), v.denominator() * s);
  }
  friend Fraction& operator/=(Fraction& v, T s) {
    v.denominator() *= s;
    return v;
  }
  */
  friend Fraction operator/(const Fraction& v1, const Fraction& v2) {
    return Fraction(v1.numerator() * v2.denominator(),
                    v1.denominator() * v2.numerator());
  }
  friend Fraction& operator/=(Fraction& v1, const Fraction& v2) {
    v1.numerator() *= v2.denominator();
    v1.denominator() *= v2.numerator();
    return v1;
  }

  // TODO: Comparison? Should this check equivalence or equality of
  // asHmgVector.hmgVectorData()?

  friend std::ostream& operator<<(std::ostream& o, const Fraction& v) {
    o << v.numerator() << " | " << v.denominator();
    return o;
  }

#if VX_VECTOR_ENABLE_QDEBUG
  friend QDebug operator<<(QDebug debug, const Fraction& v) {
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug << v.numerator() << " | " << v.denominator();
    return debug;
  }
#endif

  static Fraction invalid() { return Fraction(0, 0); }

  static Fraction zero() { return Fraction(0, 1); }

  static Fraction infinity() { return Fraction(1, 0); }

  static Fraction negativeInfinity() { return Fraction(-1, 0); }
};

template <typename T>
inline Fraction<T> toFraction(T value) {
  return Fraction<T>::fromValue(value);
}

template <typename T>
vx::Fraction<T> createFraction(const vx::HmgVector<T, 1>& hmgVector) {
  return Fraction<T>::fromHmgVector(hmgVector);
}

template <typename T>
vx::Fraction<T> createFraction(const std::array<T, 2>& array) {
  return createFraction(toVector(array));
}

template <typename To, typename From>
inline std::enable_if_t<IsConvertibleWithoutNarrowing<From, To>::value,
                        Fraction<To>>
fractionCast(const Fraction<From>& v) {
  return Fraction<To>{v.numerator(), v.denominator()};
}

template <typename To, typename From>
inline std::enable_if_t<std::is_convertible<From, To>::value, Fraction<To>>
fractionCastNarrow(const Fraction<From>& v) {
  return Fraction<To>{static_cast<To>(v.numerator()),
                      static_cast<To>(v.denominator())};
}

template <typename T, typename AllowNonDivisionRing = std::false_type>
inline Fraction<T> normalizeSpherically(
    const Fraction<T>& v, AllowNonDivisionRing = AllowNonDivisionRing()) {
  return createFraction(
      normalizeSpherically<T, 1, AllowNonDivisionRing>(v.asHmgVector()));
}

// Note: This will return nan/infinity values for non-finite fractions
template <typename T, typename AllowNonDivisionRing = std::false_type>
T toRealNumber(const vx::Fraction<T>& v) {
  static_assert(
      vx::RingTraits<T>::isDivisionRing || AllowNonDivisionRing::value,
      "toRealNumber() cannot be used for vector module over non-division "
      "ring");

  return v.numerator() / v.denominator();
}
}  // namespace vx
