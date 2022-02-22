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
#include <limits>

namespace vx {
namespace t3r {

using StreamID = uint64_t;

using Timestamp = int64_t;
using Coord = uint8_t;
using ShortTimestamp = uint16_t;
using Energy = float;

struct EventListEntry {
  // Time of activation (ToA), converted into 16/25th nanoseconds
  Timestamp timestamp;

  // X/Y coordinates of the activation event
  Coord x;
  Coord y;

  // Time over threshold (ToT), proxy for the value of the pixel
  ShortTimestamp timeOverThreshold;

  inline bool isValid() const {
    return timestamp != std::numeric_limits<Timestamp>::max();
  }
};

static constexpr EventListEntry InvalidEntry = {
    std::numeric_limits<Timestamp>::max(), std::numeric_limits<Coord>::max(),
    std::numeric_limits<Coord>::max(),
    std::numeric_limits<ShortTimestamp>::max()};

static constexpr uint32_t MatrixColumnCount = 256;
static constexpr uint32_t MatrixRowCount = 256;
static constexpr uint32_t MatrixPixelCount = MatrixColumnCount * MatrixRowCount;

}  // namespace t3r
}  // namespace vx
