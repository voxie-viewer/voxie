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

#include <stdlib.h>

#ifdef __unix__
#undef min
#undef max
#endif
namespace vx {

class Range {
 public:
  explicit Range() : min(1), max(1) {}

  explicit Range(size_t exactValue) : min(exactValue), max(exactValue) {}

  explicit Range(size_t min, size_t max) : min(min), max(max) {}

  inline bool isValid() const { return this->max >= this->min; }

  inline bool isSingularity() const {
    return this->isValid() && (this->max == this->min);
  }

  inline bool isNull() const { return (this->max == 0) && (this->min == 0); }

  inline bool contains(size_t value) const {
    return this->isValid() && (value >= this->min) && (value <= this->max);
  }

  size_t min;
  size_t max;
};

}  // namespace vx
