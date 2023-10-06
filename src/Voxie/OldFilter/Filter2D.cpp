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

#include "Filter2D.hpp"

#include <Voxie/OldFilterMask/ImageComparator.hpp>

using namespace vx::filter;
using namespace vx;

Filter2D::Filter2D(vx::plugin::MetaFilter2D* metaFilter)
    : QObject(nullptr), metaFilter_(metaFilter), mask(nullptr) {
  if (mask == nullptr) {
    this->mask = new Selection2DMask(this);
  }
  connect(this->mask, &Selection2DMask::changed, this,
          &Filter2D::triggerFilterChanged);
}

FloatImage Filter2D::applyToCopy(FloatImage input) {
  FloatImage output = input.clone();
  this->applyTo(input, output);
  ImageComparator::compareImage(
      input, output, QRectF(QPointF(0, 0), output.getDimension()), this->mask);
  return output;
}

SliceImage Filter2D::applyToCopy(SliceImage input) {
  SliceImage output = input.clone();
  this->applyTo(input, output);
  ImageComparator::compareImage(input, output, output.context().planeArea,
                                this->mask);
  return output;
}

bool Filter2D::isEnabled() { return this->enabled; }

void Filter2D::setEnabled(bool enable) {
  if (this->enabled != enable) {
    this->enabled = enable;
    Q_EMIT filterChanged(this);
  }
}
