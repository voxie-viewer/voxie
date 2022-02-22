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

#include "EventListTypes.hpp"

#include <QDataStream>

#include <algorithm>
#include <limits>
#include <string>
#include <vector>

namespace vx {
namespace t3r {
namespace cache {

static const std::string MagicHeader = "t3rcache";
static const quint32 Version = 3;

struct Interval {
  using Value = quint32;

  Value start = std::numeric_limits<Value>::max();
  Value end = 0;
  Value entryCount = 0;

  Interval() = default;
  Interval(Value start, Value end, Value entryCount = 0)
      : start(start), end(end), entryCount(entryCount) {}

  inline bool isValid() const {
    return start != std::numeric_limits<Value>::max() && end != 0;
  }

  inline Value size() const { return start < end ? end - start : 0; }

  inline Interval expand(Value value) const {
    return Interval(std::min(start, value), std::max(end, value),
                    entryCount + 1);
  }

  inline Interval merge(const Interval& interval) const {
    if (!isValid()) {
      return interval;
    } else if (!interval.isValid()) {
      return *this;
    } else {
      return Interval(std::min(start, interval.start),
                      std::max(end, interval.end),
                      entryCount + interval.entryCount);
    }
  }

  friend bool operator==(const Interval& a, const Interval& b) {
    return a.start == b.start && a.end == b.end && a.entryCount == b.entryCount;
  }

  friend bool operator!=(const Interval& a, const Interval& b) {
    return a.start != b.start || a.end != b.end || a.entryCount != b.entryCount;
  }

  friend QDataStream& operator<<(QDataStream& stream,
                                 const Interval& interval) {
    return stream << interval.start << interval.end << interval.entryCount;
  }

  friend QDataStream& operator>>(QDataStream& stream, Interval& interval) {
    return stream >> interval.start >> interval.end >> interval.entryCount;
  }
};

static const Interval InvalidInterval{std::numeric_limits<quint32>::max(), 0};

struct Segment {
  // Sad Qt things (qint64 != int64_t)
  using Timestamp = qint64;

  Timestamp minimumTimestamp = 0;
  Timestamp maximumTimestamp = 0;
  quint64 startOffset = 0;
  Timestamp intervalLength = 0;
  Timestamp intervalStride = 0;
  quint64 eventCount = 0;
  std::vector<Interval> intervals;

  friend QDataStream& operator<<(QDataStream& stream, const Segment& segment) {
    quint64 intervalCount = segment.intervals.size();
    stream << segment.minimumTimestamp << segment.maximumTimestamp
           << segment.intervalLength << segment.intervalStride
           << segment.eventCount << intervalCount;
    for (auto& interval : segment.intervals) {
      stream << interval;
    }
    return stream;
  }

  friend QDataStream& operator>>(QDataStream& stream, Segment& segment) {
    quint64 intervalCount = 0;
    stream >> segment.minimumTimestamp >> segment.maximumTimestamp >>
        segment.intervalLength >> segment.intervalStride >>
        segment.eventCount >> intervalCount;
    segment.intervals.resize(intervalCount);
    for (auto& interval : segment.intervals) {
      stream >> interval;
    }
    return stream;
  }
};

}  // namespace cache
}  // namespace t3r
}  // namespace vx
