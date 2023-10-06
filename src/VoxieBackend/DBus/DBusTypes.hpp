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

#include <VoxieClient/DBusTypeList.hpp>
#include <VoxieClient/Vector.hpp>

#include <QtGui/QQuaternion>
#include <QtGui/QVector2D>
#include <QtGui/QVector3D>

namespace vx {
template <typename T>
TupleVector<T, 3> toTupleVector(const vx::Vector<T, 3>& value) {
  return std::make_tuple(value.template access<0>(), value.template access<1>(),
                         value.template access<2>());
}

inline TupleVector<double, 2> toTupleVector(const QVector2D& vec) {
  return TupleVector<double, 2>(vec.x(), vec.y());
}
inline TupleVector<double, 3> toTupleVector(const QVector3D& vec) {
  return TupleVector<double, 3>(vec.x(), vec.y(), vec.z());
}
inline TupleVector<double, 4> toTupleVector(const QQuaternion& vec) {
  return TupleVector<double, 4>(vec.scalar(), vec.x(), vec.y(), vec.z());
}
inline QVector2D toQtVector(const TupleVector<double, 2>& vec) {
  return QVector2D(std::get<0>(vec), std::get<1>(vec));
}
inline QVector3D toQtVector(const TupleVector<double, 3>& vec) {
  return QVector3D(std::get<0>(vec), std::get<1>(vec), std::get<2>(vec));
}
inline QQuaternion toQtVector(const TupleVector<double, 4>& vec) {
  return QQuaternion(std::get<0>(vec), std::get<1>(vec), std::get<2>(vec),
                     std::get<3>(vec));
}

// TODO: Remove VectorSizeT2 and VectorSizeT3

struct VectorSizeT3 {
  size_t x, y, z;

  VectorSizeT3() {}

  VectorSizeT3(size_t x, size_t y, size_t z) : x(x), y(y), z(z) {}

  // TODO: Do range checks?
  VectorSizeT3(const vx::TupleVector<quint64, 3>& vector)
      : x(std::get<0>(vector)),
        y(std::get<1>(vector)),
        z(std::get<2>(vector)) {}

  VectorSizeT3(const vx::Vector<size_t, 3>& vector)
      : x(vector.access<0>()), y(vector.access<1>()), z(vector.access<2>()) {}

  QVector3D toQVector3D() const { return QVector3D(x, y, z); }

  vx::TupleVector<quint64, 3> toTupleVector() const {
    return vx::TupleVector<quint64, 3>(x, y, z);
  }
};

struct VectorSizeT2 {
 public:
  size_t x, y;

  VectorSizeT2() {}

  VectorSizeT2(size_t x, size_t y) : x(x), y(y) {}

  // TODO: Do range checks?
  VectorSizeT2(const vx::TupleVector<quint64, 2>& vector)
      : x(std::get<0>(vector)), y(std::get<1>(vector)) {}

  QVector2D toQVector2D() const { return QVector2D(x, y); }

  vx::TupleVector<quint64, 2> toTupleVector() const {
    return vx::TupleVector<quint64, 2>(x, y);
  }
};

}  // namespace vx
