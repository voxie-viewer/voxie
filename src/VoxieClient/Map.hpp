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

#include <VoxieClient/Matrix.hpp>

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
// This file contains for loops like this (where dim is a template parameter):
// for (size_t i = 0; i < dim; i++)
// On gcc 10.2.1 this will cause -Wtype-limits errors when the function is
// instantiated with dim=0.
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif

namespace vx {
// Linear Map

template <typename T, std::size_t srcDim, std::size_t dstDim = srcDim>
class LinearMap {
  using LinearMatrixType = vx::Matrix<T, dstDim, srcDim>;
  using AffineMatrixType = vx::Matrix<T, dstDim, srcDim + 1>;
  using ProjectiveMatrixType = vx::Matrix<T, dstDim + 1, srcDim + 1>;

  LinearMatrixType matrix_;

  LinearMap(const LinearMatrixType& matrix) : matrix_(matrix) {}

 public:
  const LinearMatrixType& linearMatrix() const { return matrix_; }
  AffineMatrixType affineMatrix() const {
    return concatColumns(linearMatrix(), vx::Matrix<T, dstDim, 1>::zero());
  }
  ProjectiveMatrixType projectiveMatrix() const {
    return concatRows(affineMatrix(),
                      concatColumns(vx::Matrix<T, 1, srcDim>::zero(),
                                    vx::Matrix<T, 1, 1>(1)));
  }

  static LinearMap fromLinearMatrix(const LinearMatrixType& matrix) {
    return LinearMap(matrix);
  }

  vx::Vector<T, dstDim> map(const vx::Vector<T, srcDim>& v) const {
    return linearMatrix() * v;
  }
  // mapVector(v1 - v2) = map(v1) - map(v2)
  // Note that for LinearMap this means that mapVector(v) = map(v)
  vx::Vector<T, dstDim> mapVector(const vx::Vector<T, srcDim>& v) const {
    return linearMatrix() * v;
  }

  friend bool operator==(const LinearMap& m1, const LinearMap& m2) {
    return m1.linearMatrix() == m2.linearMatrix();
  }
  friend bool operator!=(const LinearMap& m1, const LinearMap& m2) {
    return !(m1 == m2);
  }

  friend std::ostream& operator<<(std::ostream& o, const LinearMap& m) {
    return o << "LinearMap" << m.linearMatrix();
  }

#if VX_VECTOR_ENABLE_QDEBUG
  friend QDebug operator<<(QDebug debug, const LinearMap& m) {
    QDebugStateSaver saver(debug);
    return debug.nospace() << "LinearMap" << m.linearMatrix();
  }
#endif
};

template <typename T, std::size_t srcDim, std::size_t dstDim>
vx::LinearMap<T, srcDim, dstDim> createLinearMap(
    const vx::Matrix<T, dstDim, srcDim>& m) {
  return LinearMap<T, srcDim, dstDim>::fromLinearMatrix(m);
}

// Concatenation
template <typename T, size_t resSrcDim, size_t intDim, size_t resDstDim>
LinearMap<T, resSrcDim, resDstDim> operator*(
    const LinearMap<T, intDim, resDstDim>& m1,
    const LinearMap<T, resSrcDim, intDim>& m2) {
  return createLinearMap(m1.linearMatrix() * m2.linearMatrix());
}
// Only works for transforms with srcDim == dstDim
template <typename T, size_t dim>
LinearMap<T, dim, dim>& operator*=(LinearMap<T, dim, dim>& m,
                                   const LinearMap<T, dim, dim>& m2) {
  auto result = m * m2;
  return m = result;
}

template <typename T, std::size_t count>
static LinearMap<T, count, count> identityMap() {
  return createLinearMap(identityMatrix<T, count>());
}

// Note: Only maps which do not change dimension can be invertible
template <typename T, std::size_t dim>
LinearMap<T, dim> inverse(const LinearMap<T, dim>& m,
                          T* determinantPtr = nullptr) {
  return createLinearMap(inverse(m.linearMatrix(), determinantPtr));
}

template <typename To, typename From, std::size_t srcDim, std::size_t dstDim>
inline std::enable_if_t<std::is_convertible<From, To>::value,
                        LinearMap<To, srcDim, dstDim>>
mapCast(const LinearMap<From, srcDim, dstDim>& m) {
  return createLinearMap(matrixCast<To>(m.linearMatrix()));
}

// Affine map

template <typename T, std::size_t srcDim, std::size_t dstDim = srcDim>
class AffineMap {
  using LinearMatrixType = vx::Matrix<T, dstDim, srcDim>;
  using AffineMatrixType = vx::Matrix<T, dstDim, srcDim + 1>;
  using ProjectiveMatrixType = vx::Matrix<T, dstDim + 1, srcDim + 1>;

  AffineMatrixType matrix_;

  AffineMap(const AffineMatrixType& matrix) : matrix_(matrix) {}

 public:
  AffineMap(const LinearMap<T, srcDim, dstDim>& lin)
      : matrix_(lin.affineMatrix()) {}

  const AffineMatrixType& affineMatrix() const { return matrix_; }
  ProjectiveMatrixType projectiveMatrix() const {
    return concatRows(affineMatrix(),
                      concatColumns(vx::Matrix<T, 1, srcDim>::zero(),
                                    vx::Matrix<T, 1, 1>(1)));
  }

  // The result of mapping the origin
  vx::Vector<T, dstDim> translation() const {
    vx::Vector<T, dstDim> res;
    for (size_t row = 0; row < dstDim; row++)
      res(row) = affineMatrix()(row, srcDim);
    return res;
  }
  LinearMatrixType discardTranslationLinearMatrix() const {
    LinearMatrixType res;
    for (size_t row = 0; row < dstDim; row++)
      for (size_t col = 0; col < srcDim; col++)
        res(row, col) = affineMatrix()(row, col);
    return res;
  }
  vx::LinearMap<T, srcDim, dstDim> discardTranslation() const {
    return createLinearMap(discardTranslationLinearMatrix());
  }

  static AffineMap fromAffineMatrix(const AffineMatrixType& matrix) {
    return AffineMap(matrix);
  }

  vx::Vector<T, dstDim> map(const vx::Vector<T, srcDim>& v) const {
    return affineMatrix() * concat(v, vx::Vector<T, 1>(1));
  }
  // mapVector(v1 - v2) = map(v1) - map(v2)
  vx::Vector<T, dstDim> mapVector(const vx::Vector<T, srcDim>& v) const {
    return discardTranslationLinearMatrix() * v;
  }

  friend bool operator==(const AffineMap& m1, const AffineMap& m2) {
    return m1.affineMatrix() == m2.affineMatrix();
  }
  friend bool operator!=(const AffineMap& m1, const AffineMap& m2) {
    return !(m1 == m2);
  }

  friend std::ostream& operator<<(std::ostream& o, const AffineMap& m) {
    return o << "AffineMap" << m.affineMatrix();
  }

#if VX_VECTOR_ENABLE_QDEBUG
  friend QDebug operator<<(QDebug debug, const AffineMap& m) {
    QDebugStateSaver saver(debug);
    return debug.nospace() << "AffineMap" << m.affineMatrix();
  }
#endif
};

template <typename T, std::size_t srcDimPlusOne, std::size_t dstDim>
vx::AffineMap<T, srcDimPlusOne - 1, dstDim> createAffineMap(
    const vx::Matrix<T, dstDim, srcDimPlusOne>& m) {
  static_assert(srcDimPlusOne > 0,
                "Matrix for affine map must have at least one column");

  return AffineMap<T, srcDimPlusOne - 1, dstDim>::fromAffineMatrix(m);
}

// Concatenation
template <typename T, size_t resSrcDim, size_t intDim, size_t resDstDim>
AffineMap<T, resSrcDim, resDstDim> operator*(
    const AffineMap<T, intDim, resDstDim>& m1,
    const AffineMap<T, resSrcDim, intDim>& m2) {
  return createAffineMap(m1.affineMatrix() * m2.projectiveMatrix());
}
// Only works for transforms with srcDim == dstDim
template <typename T, size_t dim>
AffineMap<T, dim, dim>& operator*=(AffineMap<T, dim, dim>& m,
                                   const AffineMap<T, dim, dim>& m2) {
  auto result = m * m2;
  return m = result;
}
// Needed to allow template parameter deduction
template <typename T, size_t resSrcDim, size_t intDim, size_t resDstDim>
AffineMap<T, resSrcDim, resDstDim> operator*(
    const AffineMap<T, intDim, resDstDim>& m1,
    const LinearMap<T, resSrcDim, intDim>& m2) {
  // return m1 * (AffineMap<T, resSrcDim, intDim>)m2;
  return createAffineMap(m1.affineMatrix() * m2.projectiveMatrix());
}
template <typename T, size_t resSrcDim, size_t intDim, size_t resDstDim>
AffineMap<T, resSrcDim, resDstDim> operator*(
    const LinearMap<T, intDim, resDstDim>& m1,
    const AffineMap<T, resSrcDim, intDim>& m2) {
  // return (AffineMap<T, intDim, resDstDim>)m1 * m2;
  return createAffineMap(m1.affineMatrix() * m2.projectiveMatrix());
}

template <typename T, std::size_t dim>
AffineMap<T, dim, dim> createTranslation(const vx::Vector<T, dim>& v) {
  return createAffineMap(
      concatColumns(identityMatrix<T, dim>(), createColumnVector(v)));
}

// Note: Only maps which do not change dimension can be invertible
template <typename T, std::size_t dim>
AffineMap<T, dim> inverse(const AffineMap<T, dim>& m,
                          T* determinantPtr = nullptr) {
  return inverse(m.discardTranslation(), determinantPtr) *
         createTranslation(-m.translation());
}

template <typename To, typename From, std::size_t srcDim, std::size_t dstDim>
inline std::enable_if_t<std::is_convertible<From, To>::value,
                        AffineMap<To, srcDim, dstDim>>
mapCast(const AffineMap<From, srcDim, dstDim>& m) {
  return createAffineMap(matrixCast<To>(m.affineMatrix()));
}

// Projective map

template <typename T, std::size_t srcDim, std::size_t dstDim = srcDim>
class ProjectiveMap {
  using LinearMatrixType = vx::Matrix<T, dstDim, srcDim>;
  using AffineMatrixType = vx::Matrix<T, dstDim, srcDim + 1>;
  using ProjectiveMatrixType = vx::Matrix<T, dstDim + 1, srcDim + 1>;

  ProjectiveMatrixType matrix_;

  ProjectiveMap(const ProjectiveMatrixType& matrix) : matrix_(matrix) {}

 public:
  ProjectiveMap(const LinearMap<T, srcDim, dstDim>& lin)
      : matrix_(lin.projectiveMatrix()) {}
  ProjectiveMap(const AffineMap<T, srcDim, dstDim>& lin)
      : matrix_(lin.projectiveMatrix()) {}

  const ProjectiveMatrixType& projectiveMatrix() const { return matrix_; }

  static ProjectiveMap fromProjectiveMatrix(
      const ProjectiveMatrixType& matrix) {
    return ProjectiveMap(matrix);
  }

  // This is a template to prevent it being instantiated for non-division-ring
  // types
  template <typename AllowNonDivisionRing = std::false_type>
  vx::Vector<T, dstDim> map(
      const vx::Vector<T, srcDim>& v,
      AllowNonDivisionRing = AllowNonDivisionRing()) const {
    // Is needed for "T inv = 1 / res1(dstDim);"
    static_assert(
        vx::RingTraits<T>::isDivisionRing || AllowNonDivisionRing::value,
        "ProjectiveMap::map() cannot be used for vector module over "
        "non-division ring");

    vx::Vector<T, dstDim + 1> res1 =
        projectiveMatrix() * concat(v, vx::Vector<T, 1>(1));
    vx::Vector<T, dstDim> res;

    T inv = 1 / res1(dstDim);
    for (std::size_t i = 0; i < dstDim; i++) res(i) = res1(i) * inv;
    return res;
  }
  // There is no mapVector(): The result of transforming a vector with a
  // projective map depends on the position of the vector

  friend bool operator==(const ProjectiveMap& m1, const ProjectiveMap& m2) {
    return m1.projectiveMatrix() == m2.projectiveMatrix();
  }
  friend bool operator!=(const ProjectiveMap& m1, const ProjectiveMap& m2) {
    return !(m1 == m2);
  }

  friend std::ostream& operator<<(std::ostream& o, const ProjectiveMap& m) {
    return o << "ProjectiveMap" << m.projectiveMatrix();
  }

#if VX_VECTOR_ENABLE_QDEBUG
  friend QDebug operator<<(QDebug debug, const ProjectiveMap& m) {
    QDebugStateSaver saver(debug);
    return debug.nospace() << "ProjectiveMap" << m.projectiveMatrix();
  }
#endif
};

template <typename T, std::size_t srcDimPlusOne, std::size_t dstDimPlusOne>
vx::ProjectiveMap<T, srcDimPlusOne - 1, dstDimPlusOne - 1> createProjectiveMap(
    const vx::Matrix<T, dstDimPlusOne, srcDimPlusOne>& m) {
  static_assert(dstDimPlusOne > 0,
                "Matrix for projective map must have at least one row");
  static_assert(srcDimPlusOne > 0,
                "Matrix for projective map must have at least one column");

  return ProjectiveMap<T, srcDimPlusOne - 1,
                       dstDimPlusOne - 1>::fromProjectiveMatrix(m);
}

// Concatenation
template <typename T, size_t resSrcDim, size_t intDim, size_t resDstDim>
ProjectiveMap<T, resSrcDim, resDstDim> operator*(
    const ProjectiveMap<T, intDim, resDstDim>& m1,
    const ProjectiveMap<T, resSrcDim, intDim>& m2) {
  return createProjectiveMap(m1.projectiveMatrix() * m2.projectiveMatrix());
}
// Only works for transforms with srcDim == dstDim
template <typename T, size_t dim>
ProjectiveMap<T, dim, dim>& operator*=(ProjectiveMap<T, dim, dim>& m,
                                       const ProjectiveMap<T, dim, dim>& m2) {
  auto result = m * m2;
  return m = result;
}
// Needed to allow template parameter deduction
template <typename T, size_t resSrcDim, size_t intDim, size_t resDstDim>
ProjectiveMap<T, resSrcDim, resDstDim> operator*(
    const ProjectiveMap<T, intDim, resDstDim>& m1,
    const LinearMap<T, resSrcDim, intDim>& m2) {
  // return m1 * (ProjectiveMap<T, resSrcDim, intDim>)m2;
  return createProjectiveMap(m1.projectiveMatrix() * m2.projectiveMatrix());
}
template <typename T, size_t resSrcDim, size_t intDim, size_t resDstDim>
ProjectiveMap<T, resSrcDim, resDstDim> operator*(
    const LinearMap<T, intDim, resDstDim>& m1,
    const ProjectiveMap<T, resSrcDim, intDim>& m2) {
  // return (ProjectiveMap<T, intDim, resDstDim>)m1 * m2;
  return createProjectiveMap(m1.projectiveMatrix() * m2.projectiveMatrix());
}
template <typename T, size_t resSrcDim, size_t intDim, size_t resDstDim>
ProjectiveMap<T, resSrcDim, resDstDim> operator*(
    const ProjectiveMap<T, intDim, resDstDim>& m1,
    const AffineMap<T, resSrcDim, intDim>& m2) {
  // return m1 * (ProjectiveMap<T, resSrcDim, intDim>)m2;
  return createProjectiveMap(m1.projectiveMatrix() * m2.projectiveMatrix());
}
template <typename T, size_t resSrcDim, size_t intDim, size_t resDstDim>
ProjectiveMap<T, resSrcDim, resDstDim> operator*(
    const AffineMap<T, intDim, resDstDim>& m1,
    const ProjectiveMap<T, resSrcDim, intDim>& m2) {
  // return (ProjectiveMap<T, intDim, resDstDim>)m1 * m2;
  return createProjectiveMap(m1.projectiveMatrix() * m2.projectiveMatrix());
}

// Note: Only maps which do not change dimension can be invertible
template <typename T, std::size_t dim>
ProjectiveMap<T, dim> inverse(const ProjectiveMap<T, dim>& m,
                              T* determinantPtr = nullptr) {
  return createProjectiveMap(inverse(m.projectiveMatrix(), determinantPtr));
}

template <typename To, typename From, std::size_t srcDim, std::size_t dstDim>
inline std::enable_if_t<std::is_convertible<From, To>::value,
                        ProjectiveMap<To, srcDim, dstDim>>
mapCast(const ProjectiveMap<From, srcDim, dstDim>& m) {
  return createProjectiveMap(matrixCast<To>(m.projectiveMatrix()));
}

}  // namespace vx
