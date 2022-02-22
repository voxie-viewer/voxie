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

// TODO fix include path
#include "../ExtDataT3R/EventListBufferGroup.hpp"
#include "../ExtDataT3R/EventListTypes.hpp"

#include <array>
#include <vector>

namespace vx {
namespace t3r {

class EventBucketer {
 private:
  using Index = uint32_t;

  struct Bucket {
    void insert(Timestamp timestamp, ShortTimestamp timeOverThreshold);

    std::vector<Timestamp> timestamps;
    std::vector<ShortTimestamp> timesOverThreshold;
    Index startIndex = 0;
    Index remainingEvents = 0;
  };

 public:
  class Range {
   public:
    class Iterator {
     public:
      Iterator(Timestamp* timestamp, ShortTimestamp* timeOverThreshold);
      Iterator(const Iterator& it) = default;

      Iterator& operator++();
      Iterator operator++(int);

      bool operator==(const Iterator& it) const;
      bool operator!=(const Iterator& it) const;

      bool isValid() const;

      Timestamp getTimestamp() const;
      ShortTimestamp getTimeOverThreshold() const;

     private:
      Timestamp* timestamp = nullptr;
      ShortTimestamp* timeOverThreshold = nullptr;
    };

    Range(Bucket& bucket, Index startIndex, Index endIndex);

    Iterator begin();
    Iterator end();

    bool empty() const;
    std::size_t size() const;

    void destroy();

    operator bool() const;

   private:
    Bucket& bucket;
    Index startIndex;
    Index endIndex;
  };

  EventBucketer(const EventListBufferGroup& bufferGroup,
                std::size_t entryCount);
  EventBucketer(const EventListEntry* entries, std::size_t entryCount);

  bool findInitialTimestamp(Coord x, Coord y, Timestamp& timestamp);

  Range findRangeAt(Coord x, Coord y, Timestamp minTime, Timestamp maxTime);

 private:
  Index lookUpIndexForMinimumTime(Coord x, Coord y, Timestamp minTime) const;
  Index lookUpIndexForMaximumTime(Coord x, Coord y, Timestamp maxTime) const;

  Bucket& getBucket(Coord x, Coord y);
  const Bucket& getBucket(Coord x, Coord y) const;

  std::array<Bucket, MatrixPixelCount> buckets;
};

}  // namespace t3r
}  // namespace vx
