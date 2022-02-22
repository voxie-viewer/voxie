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

#include <cstdint>
#include <vector>

namespace vx {
namespace t3r {

class ProjectionImage {
 public:
  using Coord = std::int32_t;

  ProjectionImage() = default;
  ProjectionImage(Coord width, Coord height);

  // Prevent accidental (expensive) copies
  ProjectionImage(const ProjectionImage& projection) = delete;
  ProjectionImage& operator=(const ProjectionImage& projection) = delete;

  ProjectionImage(ProjectionImage&& projection) = default;
  ProjectionImage& operator=(ProjectionImage&& projection) = default;

  void set(Coord x, Coord y, double value);
  double get(Coord x, Coord y) const;

  void add(Coord x, Coord y, double value);
  void addInBounds(Coord x, Coord y, double value);
  void splatAdd(float x, float y, double value);

  void combine(const ProjectionImage& image);

  Coord getWidth() const;
  Coord getHeight() const;

  void setExposureTime(double exposureTime);
  double getExposureTime() const;

 private:
  std::size_t getIndex(Coord x, Coord y) const;
  double& at(Coord x, Coord y);

  std::vector<double> data;
  Coord width = 0;
  Coord height = 0;
  double exposureTime = 1;
};

}  // namespace t3r
}  // namespace vx
