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

#include <VoxieClient/Map.hpp>
#include <VoxieClient/Matrix.hpp>
#include <VoxieClient/Rotation.hpp>
#include <VoxieClient/Vector.hpp>

#include <QtGui/QGenericMatrix>
#include <QtGui/QMatrix4x4>
#include <QtGui/QQuaternion>
#include <QtGui/QVector2D>
#include <QtGui/QVector3D>
#include <QtGui/QVector4D>

namespace vx {

// Vectors

inline QVector2D toQVector(const vx::Vector<float, 2>& v) {
  return QVector2D(v.access<0>(), v.access<1>());
}
inline QVector3D toQVector(const vx::Vector<float, 3>& v) {
  return QVector3D(v.access<0>(), v.access<1>(), v.access<2>());
}
inline QVector4D toQVector(const vx::Vector<float, 4>& v) {
  return QVector4D(v.access<0>(), v.access<1>(), v.access<2>(), v.access<3>());
}

inline vx::Vector<float, 2> toVector(const QVector2D& v) {
  return {v.x(), v.y()};
}
inline vx::Vector<float, 3> toVector(const QVector3D& v) {
  return {v.x(), v.y(), v.z()};
}
inline vx::Vector<float, 4> toVector(const QVector4D& v) {
  return {v.x(), v.y(), v.z(), v.w()};
}

// Quaternions / Rotations

inline vx::Quaternion<float> toQuaternion(const QQuaternion& q) {
  return {q.scalar(), q.x(), q.y(), q.z()};
}

// Note: Will throw if q is not a unit quaternion
inline vx::Rotation<float, 3> toRotation(const QQuaternion& q) {
  return vx::Rotation<float, 3>(toQuaternion(q));
}

inline QQuaternion toQQuaternion(const vx::Quaternion<float>& q) {
  return QQuaternion(q.a(), q.b(), q.c(), q.d());
}

inline QQuaternion toQQuaternion(const vx::Rotation<float, 3>& rot) {
  return toQQuaternion(rot.asQuaternion());
}

// Matrices

// Type of rowCount / columnCount must be int to allow template parameter
// deduction
// template <typename T, std::size_t rowCount, std::size_t columnCount>
template <typename T, int rowCount, int columnCount>
inline vx::Matrix<T, rowCount, columnCount> toMatrix(
    const QGenericMatrix<columnCount, rowCount, T>& m) {
  static_assert(rowCount >= 0, "Row count is negative");
  static_assert(columnCount >= 0, "Column count is negative");

  vx::Matrix<T, rowCount, columnCount> res;
  for (size_t row = 0; row < rowCount; row++)
    for (size_t col = 0; col < columnCount; col++) res(row, col) = m(row, col);
  return res;
}

inline vx::Matrix<float, 4, 4> toMatrix(const QMatrix4x4& m) {
  return toMatrix(m.toGenericMatrix<4, 4>());
}

template <typename T, std::size_t rowCount, std::size_t columnCount>
inline QGenericMatrix<columnCount, rowCount, T> toQGenericMatrix(
    const vx::Matrix<T, rowCount, columnCount>& m) {
  QGenericMatrix<columnCount, rowCount, T> res;
  for (size_t row = 0; row < rowCount; row++)
    for (size_t col = 0; col < columnCount; col++) res(row, col) = m(row, col);
  return res;
}

inline QMatrix4x4 toQMatrix4x4(const vx::Matrix<float, 4, 4>& m) {
  return QMatrix4x4(toQGenericMatrix(m));
}

// QPoint / QPointF

inline vx::Vector<int, 2> pointToVector(const QPoint& v) {
  return {v.x(), v.y()};
}
inline vx::Vector<qreal, 2> pointToVector(const QPointF& v) {
  return {v.x(), v.y()};
}

}  // namespace vx
