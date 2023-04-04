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

#include <Voxie/Voxie.hpp>

#include <VoxieClient/VoxieClient.hpp>

#include <QVector3D>

#include <limits>

namespace vx {

template <typename T, std::size_t dim>
class Vector;
// TODO: Switch from QVector3D to vx::Vector<double, 3>

class VOXIECORESHARED_EXPORT BoundingBox3D {
  QVector3D min_, max_;

  static constexpr double infinity = std::numeric_limits<double>::infinity();

 public:
  BoundingBox3D()
      : min_(infinity, infinity, infinity),
        max_(-infinity, -infinity, -infinity) {}

  BoundingBox3D(const QVector3D& min, const QVector3D& max)
      : min_(min), max_(max) {}

  const QVector3D& min() const { return min_; }
  const QVector3D& max() const { return max_; }

  BoundingBox3D operator+(const BoundingBox3D& other) const {
    return BoundingBox3D(QVector3D(std::min(min().x(), other.min().x()),
                                   std::min(min().y(), other.min().y()),
                                   std::min(min().z(), other.min().z())),
                         QVector3D(std::max(max().x(), other.max().x()),
                                   std::max(max().y(), other.max().y()),
                                   std::max(max().z(), other.max().z())));
  }

  BoundingBox3D& operator+=(const BoundingBox3D& other) {
    return *this = *this + other;
  }

  static BoundingBox3D empty() { return BoundingBox3D(); }

  static BoundingBox3D point(const QVector3D& point) {
    return BoundingBox3D(point, point);
  }
  static BoundingBox3D pointV(const vx::Vector<double, 3>& point);

  bool isEmpty() const {
    return min().x() > max().x() || min().y() > max().y() ||
           min().z() > max().z();
  }

  QList<vx::Vector<double, 3>> corners() const;
};

}  // namespace vx
