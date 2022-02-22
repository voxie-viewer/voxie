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

#include "ProjectionImage.hpp"

#include <QtGlobal>

using namespace vx;
using namespace vx::t3r;

ProjectionImage::ProjectionImage(Coord width, Coord height)
    : width(width), height(height) {
  data.resize(qMax<std::size_t>(width * height, 1));
}

void ProjectionImage::set(Coord x, Coord y, double value) { at(x, y) = value; }

double ProjectionImage::get(Coord x, Coord y) const {
  return data[getIndex(x, y)];
}

void ProjectionImage::add(Coord x, Coord y, double value) { at(x, y) += value; }

void ProjectionImage::addInBounds(Coord x, Coord y, double value) {
  if (x >= 0 && x < width && y >= 0 && y < height) {
    data[x + y * width] += value;
  }
}

void ProjectionImage::splatAdd(float x, float y, double value) {
  Coord floorX = x;
  Coord floorY = y;

  float subX = x - floorX;
  float subY = y - floorY;

  addInBounds(floorX, floorY, (1 - subX) * (1 - subY) * value);
  addInBounds(floorX + 1, floorY, subX * (1 - subY) * value);
  addInBounds(floorX, floorY + 1, (1 - subX) * subY * value);
  addInBounds(floorX + 1, floorY + 1, subX * subY * value);
}

void ProjectionImage::combine(const ProjectionImage& image) {
  if (data.size() == image.data.size()) {
    for (std::size_t i = 0; i < data.size(); ++i) {
      data[i] += image.data[i];
    }
    exposureTime += image.exposureTime;
  }
}

ProjectionImage::Coord ProjectionImage::getWidth() const { return width; }

ProjectionImage::Coord ProjectionImage::getHeight() const { return height; }

void ProjectionImage::setExposureTime(double exposureTime) {
  this->exposureTime = exposureTime;
}

double ProjectionImage::getExposureTime() const { return exposureTime; }

std::size_t ProjectionImage::getIndex(Coord x, Coord y) const {
  return qMin<Coord>(qMax<Coord>(x, 0), width - 1) +
         qMin<Coord>(qMax<Coord>(y, 0), height - 1) * width;
}

double& ProjectionImage::at(Coord x, Coord y) { return data[getIndex(x, y)]; }
