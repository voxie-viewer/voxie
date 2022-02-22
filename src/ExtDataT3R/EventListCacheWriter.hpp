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

#include <utility>
#include <vector>

class QIODevice;

namespace vx {
namespace t3r {

class EventListReader;

class EventListCacheWriter {
 public:
  EventListCacheWriter(EventListReader& reader, QIODevice* outputFile);

  void write();

  void setTargetIntervalSize(uint64_t targetIntervalSize);
  uint64_t getTargetIntervalSize() const;

 private:
  cache::Segment* getSegmentContainingTimestamp(Timestamp timestamp,
                                                Timestamp margin);
  void addOrExpandSegmentForTimestamp(Timestamp timestamp, Timestamp margin);

  void initializeSegments();
  void mergeSegments();
  void resizeSegments();
  void populateSegments();
  void setSegmentEventCounts();

  EventListReader& reader;
  QDataStream stream;

  std::vector<cache::Segment> segments;

  uint64_t targetIntervalSize = 20000;
};

}  // namespace t3r
}  // namespace vx
