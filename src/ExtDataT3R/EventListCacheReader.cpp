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

#include "EventListCacheReader.hpp"

#include <VoxieClient/Exception.hpp>

#include <QtCore/QDebug>
#include <QtCore/QIODevice>

namespace vx {
namespace t3r {

bool EventListCacheReader::open(QIODevice* device) {
  QDataStream stream(device);
  stream.setByteOrder(QDataStream::LittleEndian);
  stream.setVersion(QDataStream::Qt_5_5);

  std::vector<char> header(cache::MagicHeader.size());
  stream.readRawData(header.data(), header.size());

  if (std::string(header.begin(), header.end()) != cache::MagicHeader) {
    qWarning() << "Malformed T3R cache (header mismatch)";
    return false;
  }

  uint32_t version = (uint32_t)-1;
  stream >> version;

  if (version != cache::Version) {
    qWarning() << "Invalid T3R cache (version mismatch)";
    return false;
  }

  uint32_t segmentCount = 0;
  stream >> segmentCount;

  segments.resize(segmentCount);

  for (auto& segment : segments) {
    stream >> segment;
  }

  return true;
}

EventListCacheReader::Index EventListCacheReader::getEventCount() const {
  Index eventCount = 0;
  for (auto& segment : segments) {
    eventCount += segment.eventCount;
  }
  return eventCount;
}

Timestamp EventListCacheReader::getMinimumTimestamp() const {
  return segments.empty() ? 0 : segments.front().minimumTimestamp;
}

Timestamp EventListCacheReader::getMaximumTimestamp() const {
  return segments.empty() ? 0 : segments.back().maximumTimestamp;
}

std::vector<EventListCacheReader::Interval>
EventListCacheReader::lookUpIntervals(Timestamp minTimestamp,
                                      Timestamp maxTimestamp) const {
  return lookUpBoundedIntervals(minTimestamp, maxTimestamp,
                                std::numeric_limits<Index>::max());
}

std::vector<EventListCacheReader::Interval>
EventListCacheReader::lookUpBoundedIntervals(Timestamp minTimestamp,
                                             Timestamp& maxTimestamp,
                                             Index maxEntryCount) const {
  std::vector<Interval> intervals;

  // Determine which segments have to be checked
  Index minSegmentID = getSegmentContainingTimestamp(minTimestamp);
  Index maxSegmentID = getSegmentContainingTimestamp(maxTimestamp);

  if (minSegmentID == InvalidIndex) {
    // Minimum segment out of bounds: read from start
    minSegmentID = 0;
  }

  if (maxSegmentID == InvalidIndex) {
    // Maximum segment out of bounds: read until end
    maxSegmentID = segments.size() - 1;
  }

  // Number of entries across all segments
  Index totalCount = 0;
  bool sizeLimitReached = false;

  // Iterate over all segments in the range, or until the size limit is reached
  for (Index segmentID = minSegmentID;
       segmentID <= maxSegmentID && !sizeLimitReached; ++segmentID) {
    // Compute segment-specific interval
    cache::Interval interval;
    auto& segment = segments[segmentID];

    // Determine which interval range needs to be checked
    Index minIntervalIndex = getMinIntervalIndex(segment, minTimestamp);
    Index maxIntervalIndex = getMaxIntervalIndex(segment, maxTimestamp);

    // Iterate over intervals in range, inclusively
    for (Index i = minIntervalIndex; i <= maxIntervalIndex; ++i) {
      // Merge interval with current working interval
      cache::Interval mergedInterval = interval.merge(segment.intervals[i]);
      if (totalCount + mergedInterval.entryCount > maxEntryCount &&
          totalCount > 0 && i > minIntervalIndex && i < maxIntervalIndex) {
        // Merged interval exceeds size limit: use previous interval and adjust
        // maximum timestamp accordingly
        maxTimestamp = segment.minimumTimestamp +
                       segment.intervalStride * (i - 1) +
                       segment.intervalLength;
        sizeLimitReached = true;
        break;
      } else {
        interval = mergedInterval;
      }
    }

    if (interval.isValid()) {
      // Add (valid) interval to result list after shifting it by segment offset
      totalCount += interval.entryCount;
      intervals.emplace_back(segment.startOffset + interval.start,
                             segment.startOffset + interval.end,
                             interval.entryCount);
    }
  }

  return intervals;
}

EventListCacheReader::Index EventListCacheReader::getSegmentContainingTimestamp(
    Timestamp timestamp) const {
  for (Index i = 0; i < segments.size(); ++i) {
    if (segments[i].minimumTimestamp <= timestamp &&
        segments[i].maximumTimestamp >= timestamp) {
      return i;
    }
  }

  return InvalidIndex;
}

EventListCacheReader::Index EventListCacheReader::getMinIntervalIndex(
    const cache::Segment& segment, Timestamp timestamp) const {
  Timestamp stride = qMax<Timestamp>(segment.intervalStride, 1);
  Timestamp relativeMinTimestamp =
      qMax<Timestamp>(0, timestamp - segment.minimumTimestamp);
  return qMin<Index>(relativeMinTimestamp / stride,
                     segment.intervals.size() - 1);
}

EventListCacheReader::Index EventListCacheReader::getMaxIntervalIndex(
    const cache::Segment& segment, Timestamp timestamp) const {
  Timestamp stride = qMax<Timestamp>(segment.intervalStride, 1);
  Timestamp relativeMaxTimestamp = qMax<Timestamp>(
      0, timestamp - segment.minimumTimestamp - segment.intervalLength);
  return qMin<Index>(((relativeMaxTimestamp + stride - 1) / stride),
                     segment.intervals.size() - 1);
}

}  // namespace t3r
}  // namespace vx
