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

#include "EventBucketer.hpp"

#include <algorithm>

namespace vx {
namespace t3r {

static const ShortTimestamp NullToT = 0;

EventBucketer::EventBucketer(const EventListBufferGroup& bufferGroup,
                             std::size_t entryCount) {
  // Obtain attribute buffers
  auto bufferX = bufferGroup.getBuffer<const Coord>("x");
  auto bufferY = bufferGroup.getBuffer<const Coord>("y");
  auto bufferTime = bufferGroup.getBuffer<const Timestamp>("timestamp");
  auto bufferTot =
      bufferGroup.getBuffer<const ShortTimestamp>("timeOverThreshold");

  // Place events in buckets
  for (std::size_t i = 0; i < entryCount; ++i) {
    getBucket(bufferX(i), bufferY(i)).insert(bufferTime(i), bufferTot(i));
  }
}

EventBucketer::EventBucketer(const EventListEntry* entries,
                             std::size_t entryCount) {
  for (std::size_t i = 0; i < entryCount; ++i) {
    auto& entry = entries[i];
    getBucket(entry.x, entry.y)
        .insert(entry.timestamp, entry.timeOverThreshold);
  }
}

bool EventBucketer::findInitialTimestamp(Coord x, Coord y,
                                         Timestamp& timestamp) {
  auto& bucket = getBucket(x, y);

  if (bucket.remainingEvents != 0) {
    while (bucket.startIndex < bucket.timestamps.size()) {
      auto timeOverThreshold = bucket.timesOverThreshold[bucket.startIndex];
      if (timeOverThreshold == NullToT) {
        bucket.startIndex++;
      } else {
        timestamp = bucket.timestamps[bucket.startIndex];
        return true;
      }
    }
  }

  return false;
}

EventBucketer::Range EventBucketer::findRangeAt(Coord x, Coord y,
                                                Timestamp minTime,
                                                Timestamp maxTime) {
  return Range(getBucket(x, y), lookUpIndexForMinimumTime(x, y, minTime),
               lookUpIndexForMaximumTime(x, y, maxTime));
}

EventBucketer::Index EventBucketer::lookUpIndexForMinimumTime(
    Coord x, Coord y, Timestamp minTime) const {
  auto& bucket = getBucket(x, y);
  return std::lower_bound(bucket.timestamps.begin() + bucket.startIndex,
                          bucket.timestamps.end(), minTime) -
         bucket.timestamps.begin();
}

EventBucketer::Index EventBucketer::lookUpIndexForMaximumTime(
    Coord x, Coord y, Timestamp maxTime) const {
  auto& bucket = getBucket(x, y);
  return std::upper_bound(bucket.timestamps.begin() + bucket.startIndex,
                          bucket.timestamps.end(), maxTime) -
         bucket.timestamps.begin();
}

void EventBucketer::Bucket::insert(Timestamp timestamp,
                                   ShortTimestamp timeOverThreshold) {
  if (timestamps.empty() || timestamps.back() <= timestamp) {
    // Common case: timestamps are already "sorted enough"
    timestamps.push_back(timestamp);
    timesOverThreshold.push_back(timeOverThreshold);
  } else {
    // Slower fallback case: timestamps need to be inserted into appropriate
    // position
    auto it = std::upper_bound(timestamps.begin(), timestamps.end(), timestamp);
    auto index = it - timestamps.begin();
    timestamps.insert(it, timestamp);
    timesOverThreshold.insert(timesOverThreshold.begin() + index,
                              timeOverThreshold);
  }

  remainingEvents++;
}

EventBucketer::Range::Iterator::Iterator(Timestamp* timestamp,
                                         ShortTimestamp* timeOverThreshold)
    : timestamp(timestamp), timeOverThreshold(timeOverThreshold) {}

Timestamp EventBucketer::Range::Iterator::getTimestamp() const {
  return *timestamp;
}

ShortTimestamp EventBucketer::Range::Iterator::getTimeOverThreshold() const {
  return *timeOverThreshold;
}

EventBucketer::Range::Iterator EventBucketer::Range::begin() {
  return Iterator(bucket.timestamps.data() + startIndex,
                  bucket.timesOverThreshold.data() + startIndex);
}

EventBucketer::Range::Iterator EventBucketer::Range::end() {
  return Iterator(bucket.timestamps.data() + endIndex,
                  bucket.timesOverThreshold.data() + endIndex);
}

bool EventBucketer::Range::empty() const { return endIndex <= startIndex; }

std::size_t EventBucketer::Range::size() const {
  return empty() ? 0 : endIndex - startIndex;
}

void EventBucketer::Range::destroy() {
  std::fill(bucket.timesOverThreshold.begin() + startIndex,
            bucket.timesOverThreshold.begin() + endIndex, NullToT);
  bucket.remainingEvents -= size();
  if (bucket.startIndex == startIndex) {
    bucket.startIndex = endIndex;
  }
}

EventBucketer::Range::operator bool() const { return !empty(); }

EventBucketer::Range::Range(Bucket& bucket, Index start, Index end)
    : bucket(bucket), startIndex(start), endIndex(end) {}

bool EventBucketer::Range::Iterator::operator==(const Iterator& it) const {
  return timestamp == it.timestamp;
}

bool EventBucketer::Range::Iterator::operator!=(const Iterator& it) const {
  return timestamp != it.timestamp;
}

bool EventBucketer::Range::Iterator::isValid() const {
  return getTimeOverThreshold() != NullToT;
}

EventBucketer::Range::Iterator EventBucketer::Range::Iterator::operator++(int) {
  Iterator it = *this;
  ++(*this);
  return it;
}

EventBucketer::Range::Iterator& EventBucketer::Range::Iterator::operator++() {
  timestamp++;
  timeOverThreshold++;
  return *this;
}

EventBucketer::Bucket& EventBucketer::getBucket(Coord x, Coord y) {
  return buckets[x + MatrixRowCount * y];
}

const EventBucketer::Bucket& EventBucketer::getBucket(Coord x, Coord y) const {
  return buckets[x + MatrixRowCount * y];
}

}  // namespace t3r
}  // namespace vx
