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

#include <VoxieClient/Vector.hpp>

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
// This file contains for loops like this (where dim is a template parameter):
// for (size_t i = 0; i < dim; i++)
// On gcc 10.2.1 this will cause -Wtype-limits errors when the function is
// instantiated with dim=0.
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif

#include <VoxieClient/TypeTraits.hpp>

namespace vx {

template <typename T, std::size_t dim>
class HmgVector {
  using VectorType = vx::Vector<T, dim>;
  using HmgVectorDataType = vx::Vector<T, dim + 1>;

  HmgVectorDataType hmgVectorData_;

  HmgVector(const HmgVectorDataType& hmgVectorData)
      : hmgVectorData_(hmgVectorData) {}

 public:
  HmgVector() = default;

  HmgVector(const VectorType& vector, T value) {
    for (size_t i = 0; i < dim; i++) this->hmgVectorData()[i] = vector[i];
    this->w() = value;
  }

  const HmgVectorDataType& hmgVectorData() const { return hmgVectorData_; }
  HmgVectorDataType& hmgVectorData() { return hmgVectorData_; }

  static HmgVector fromHmgVectorData(const HmgVectorDataType& hmgVectorData) {
    return HmgVector(hmgVectorData);
  }
  static HmgVector fromVector(const VectorType& vector) {
    return HmgVector(vector, 1);
  }

  /*
  const std::array<T, dim + 1>& asHmgArray() const {
    return this->hmgVectorData().asArray();
  }
  std::array<T, dim + 1>& asHmgArray() {
    return this->hmgVectorData().asArray();
  }
  */

  template <std::size_t i>
  const T& accessHmg() const {
    static_assert(i < dim + 1, "Index out of range");
    return hmgVectorData().template access<i>();
  }
  template <std::size_t i>
  T& accessHmg() {
    static_assert(i < dim + 1, "Index out of range");
    return hmgVectorData().template access<i>();
  }

  T w() const { return accessHmg<dim>(); }
  T& w() { return accessHmg<dim>(); }

  // This returns a copy of the first dim values as a vector
  // Note: Performance might be bad (because it returns a copy)
  // Note: The returned vector might not be equivalent to the HmgVector (if w !=
  // 1)
  vx::Vector<T, dim> getVectorPart() const {
    vx::Vector<T, dim> res;
    for (size_t i = 0; i < dim; i++) res[i] = this->hmgVectorData()[i];
    return res;
  }

  friend HmgVector operator+(const HmgVector& v) { return v; }
  friend HmgVector operator-(const HmgVector& v) {
    HmgVector ret;
    for (size_t i = 0; i < dim; i++)
      ret.hmgVectorData()[i] = -v.hmgVectorData()[i];
    // Note: Do not invert the sign here
    ret.w() = v.w();
    return ret;
  }

  // TODO: Do some normalization for + and - to avoid overflow or underflow when
  // doing many additions?
  friend HmgVector operator+(const HmgVector& v1, const HmgVector& v2) {
    HmgVector ret;
    auto w1 = v1.w();
    auto w2 = v2.w();
    for (size_t i = 0; i < dim; i++)
      ret.hmgVectorData()[i] =
          v1.hmgVectorData()[i] * w2 + v2.hmgVectorData()[i] * w1;
    ret.w() = w1 * w2;
    return ret;
  }
  friend HmgVector& operator+=(HmgVector& v1, const HmgVector& v2) {
    auto w1 = v1.w();
    auto w2 = v2.w();
    for (size_t i = 0; i < dim; i++)
      v1.hmgVectorData()[i] =
          v1.hmgVectorData()[i] * w2 + v2.hmgVectorData()[i] * w1;
    v1.w() = w1 * w2;
    return v1;
  }

  friend HmgVector operator-(const HmgVector& v1, const HmgVector& v2) {
    HmgVector ret;
    auto w1 = v1.w();
    auto w2 = v2.w();
    for (size_t i = 0; i < dim; i++)
      ret.hmgVectorData()[i] =
          v1.hmgVectorData()[i] * w2 - v2.hmgVectorData()[i] * w1;
    ret.w() = w1 * w2;
    return ret;
  }
  friend HmgVector& operator-=(HmgVector& v1, const HmgVector& v2) {
    auto w1 = v1.w();
    auto w2 = v2.w();
    for (size_t i = 0; i < dim; i++)
      v1.hmgVectorData()[i] =
          v1.hmgVectorData()[i] * w2 - v2.hmgVectorData()[i] * w1;
    v1.w() = w1 * w2;
    return v1;
  }

  friend HmgVector operator*(T s, const HmgVector& v) {
    HmgVector ret;
    for (size_t i = 0; i < dim; i++)
      ret.hmgVectorData()[i] = s * v.hmgVectorData()[i];
    // Note: Do not multiply w
    ret.w() = v.w();
    return ret;
  }
  friend HmgVector operator*(const HmgVector& v, T s) {
    HmgVector ret;
    for (size_t i = 0; i < dim; i++)
      ret.hmgVectorData()[i] = v.hmgVectorData()[i] * s;
    // Note: Do not multiply w
    ret.w() = v.w();
    return ret;
  }
  friend HmgVector& operator*=(HmgVector& v, T s) {
    for (size_t i = 0; i < dim; i++) v.hmgVectorData()[i] *= s;
    // Note: Do not multiply w
    return v;
  }

  // This is a template to prevent it being instantiated for non-division-ring
  // types
  template <typename AllowNonDivisionRing = std::false_type>
  friend HmgVector operator/(const HmgVector& v, T s) {
    // TODO: Should this be allowed?
    static_assert(
        vx::RingTraits<T>::isDivisionRing || AllowNonDivisionRing::value,
        "operator/ cannot be used for vector module over non-division "
        "ring");

    HmgVector ret;
    for (size_t i = 0; i < dim; i++)
      ret.hmgVectorData()[i] = v.hmgVectorData()[i] / s;
    // Note: Do not divide w
    ret.w() = v.w();
    return ret;
  }
  // This is a template to prevent it being instantiated for non-division-ring
  // types
  template <typename AllowNonDivisionRing = std::false_type>
  friend HmgVector& operator/=(HmgVector& v, T s) {
    // TODO: Should this be allowed?
    static_assert(
        vx::RingTraits<T>::isDivisionRing || AllowNonDivisionRing::value,
        "operator/ cannot be used for vector module over non-division "
        "ring");

    for (size_t i = 0; i < dim; i++) v.hmgVectorData()[i] /= s;
    // Note: Do not divide w
    return v;
  }

  // TODO: Comparison? Should this check equivalence or equality of
  // hmgVectorData()?
  /*
  friend bool operator==(const HmgVector& v1, const HmgVector& v2) {
    for (size_t i = 0; i < dim; i++)
      if (v1.hmgVectorData()[i] != v2.hmgVectorData()[i]) return false;
    return true;
  }
  friend bool operator!=(const HmgVector& v1, const HmgVector& v2) {
    return !(v1 == v2);
  }
  */

  friend std::ostream& operator<<(std::ostream& o, const HmgVector& v) {
    o << "(";
    for (size_t i = 0; i < dim; i++) {
      o << v.hmgVectorData()[i];
      o << ((i + 1 != dim) ? ", " : " | ");
    }
    o << v.w();
    o << ")";
    return o;
  }

#if VX_VECTOR_ENABLE_QDEBUG
  friend QDebug operator<<(QDebug debug, const HmgVector& v) {
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug << "(";
    for (size_t i = 0; i < dim; i++) {
      debug << v.hmgVectorData()[i];
      debug << ((i + 1 != dim) ? ", " : " | ");
    }
    debug << v.w();
    debug << ")";
    return debug;
  }
#endif

  static HmgVector invalid() {
    HmgVector ret;
    for (size_t i = 0; i < dim; i++) ret.hmgVectorData()[i] = 0;
    ret.w() = 0;
    return ret;
  }

  static HmgVector zero() {
    HmgVector ret;
    for (size_t i = 0; i < dim; i++) ret.hmgVectorData()[i] = 0;
    ret.w() = 1;
    return ret;
  }
};

template <typename T, std::size_t dim>
inline HmgVector<T, dim> toHmgVector(const vx::Vector<T, dim>& vector) {
  return HmgVector<T, dim>::fromVector(vector);
}

template <typename T, std::size_t dimPlusOne>
vx::HmgVector<T, dimPlusOne - 1> createHmgVector(
    const vx::Vector<T, dimPlusOne>& hmgVectorData) {
  static_assert(dimPlusOne > 0,
                "Homogeneous vector requires at least one element");

  return HmgVector<T, dimPlusOne - 1>::fromHmgVectorData(hmgVectorData);
}

template <typename T, std::size_t dimPlusOne>
vx::HmgVector<T, dimPlusOne - 1> createHmgVector(
    const std::array<T, dimPlusOne>& array) {
  static_assert(dimPlusOne > 0,
                "Homogeneous vector requires at least one element");

  return createHmgVector(toVector(array));
}

template <typename To, typename From, std::size_t dim>
inline std::enable_if_t<IsConvertibleWithoutNarrowing<From, To>::value,
                        HmgVector<To, dim>>
vectorCast(const HmgVector<From, dim>& v) {
  HmgVector<To, dim> ret;
  for (size_t i = 0; i < dim + 1; i++)
    ret.hmgVectorData()[i] = v.hmgVectorData()[i];
  return ret;
}

template <typename To, typename From, std::size_t dim>
inline std::enable_if_t<std::is_convertible<From, To>::value,
                        HmgVector<To, dim>>
vectorCastNarrow(const HmgVector<From, dim>& v) {
  HmgVector<To, dim> ret;
  for (size_t i = 0; i < dim + 1; i++)
    ret.hmgVectorData()[i] = static_cast<To>(v.hmgVectorData()[i]);
  return ret;
}

// See e.g. doi:10.1007/978-3-319-11550-4_5 5.1.2.2
template <typename T, std::size_t dim,
          typename AllowNonDivisionRing = std::false_type>
inline HmgVector<T, dim> normalizeSpherically(
    const HmgVector<T, dim>& v, AllowNonDivisionRing = AllowNonDivisionRing()) {
  using std::sqrt;
  return createHmgVector(v.hmgVectorData() /
                         sqrt(squaredNorm(v.hmgVectorData())));
}

// Note: This will return nan/infinity values for non-finite HmgVectors
template <typename T, std::size_t dim,
          typename AllowNonDivisionRing = std::false_type>
vx::Vector<T, dim> toVectorFinite(const vx::HmgVector<T, dim>& hmgVector) {
  // TODO: Should this be allowed?
  static_assert(
      vx::RingTraits<T>::isDivisionRing || AllowNonDivisionRing::value,
      "toVectorFinite() cannot be used for vector module over non-division "
      "ring");

  return hmgVector.getVectorPart() / hmgVector.w();
}
}  // namespace vx

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
