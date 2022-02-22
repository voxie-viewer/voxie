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

#include "EventListCacheCommon.hpp"

#include <QDataStream>

#include <vector>

class QIODevice;

namespace vx {
namespace t3r {

class EventListCacheReader {
 public:
  using Index = uint64_t;
  static constexpr Index InvalidIndex = std::numeric_limits<Index>::max();

  // Absolute interval struct (using 64-bit indices)
  struct Interval {
    Interval() = default;
    Interval(Index start, Index end, Index entryCount)
        : start(start), end(end), entryCount(entryCount) {}

    Index start = 0;
    Index end = 0;
    Index entryCount = 0;
  };

  EventListCacheReader() = default;

  bool open(QIODevice* device);

  /**
   * Returns the total number of events within the event list.
   *
   * This excludes invalid events such as dummy frames or padding data.
   */
  Index getEventCount() const;

  /**
   * Returns the minimum timestamp of the lowest segment in the event list.
   *
   * This excludes any outliers not represented within any segments in the
   * cache.
   */
  Timestamp getMinimumTimestamp() const;

  /**
   * Returns the maximum timestamp of the highest segment in the event list.
   *
   * This excludes any outliers not represented within any segments in the
   * cache.
   */
  Timestamp getMaximumTimestamp() const;

  std::vector<Interval> lookUpIntervals(Timestamp minTimestamp,
                                        Timestamp maxTimestamp) const;

  /**
   * Looks up a list of index intervals fully containing a timestamp region. If
   * the interval size exceeds the specified limit, the maximum timestamp is
   * reduced until the resulting interval contains slightly more entries than
   * the maximum allowed count. The modified upper bound is written back into
   * `maxTimestamp` in this case.
   *
   * Multiple intervals may be returned if they span multiple segments.
   */
  std::vector<Interval> lookUpBoundedIntervals(Timestamp minTimestamp,
                                               Timestamp& maxTimestamp,
                                               Index maxEntryCount) const;

 private:
  Index getSegmentContainingTimestamp(Timestamp timestamp) const;

  Index getMinIntervalIndex(const cache::Segment& segment,
                            Timestamp timestamp) const;

  Index getMaxIntervalIndex(const cache::Segment& segment,
                            Timestamp timestamp) const;

  std::vector<cache::Segment> segments;
};

}  // namespace t3r
}  // namespace vx
