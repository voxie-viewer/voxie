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

#include <VoxieClient/Vector.hpp>
#include <VoxieClient/VoxieClient.hpp>

#include <limits>

namespace vx {

class VOXIECORESHARED_EXPORT BoundingBox3D {
  vx::Vector<double, 3> min_, max_;

  static constexpr double infinity = std::numeric_limits<double>::infinity();

 public:
  BoundingBox3D()
      : min_(infinity, infinity, infinity),
        max_(-infinity, -infinity, -infinity) {}

  BoundingBox3D(const vx::Vector<double, 3>& min,
                const vx::Vector<double, 3>& max)
      : min_(min), max_(max) {}

  const vx::Vector<double, 3>& min() const { return min_; }
  const vx::Vector<double, 3>& max() const { return max_; }

  BoundingBox3D operator+(const BoundingBox3D& other) const {
    return BoundingBox3D(
        vx::Vector<double, 3>(
            std::min(min().access<0>(), other.min().access<0>()),
            std::min(min().access<1>(), other.min().access<1>()),
            std::min(min().access<2>(), other.min().access<2>())),
        vx::Vector<double, 3>(
            std::max(max().access<0>(), other.max().access<0>()),
            std::max(max().access<1>(), other.max().access<1>()),
            std::max(max().access<2>(), other.max().access<2>())));
  }

  BoundingBox3D& operator+=(const BoundingBox3D& other) {
    return *this = *this + other;
  }

  static BoundingBox3D empty() { return BoundingBox3D(); }

  static BoundingBox3D point(const vx::Vector<double, 3>& point) {
    return BoundingBox3D(point, point);
  }

  bool isEmpty() const {
    return min().access<0>() > max().access<0>() ||
           min().access<1>() > max().access<1>() ||
           min().access<2>() > max().access<2>();
  }

  QList<vx::Vector<double, 3>> corners() const;
};

}  // namespace vx
