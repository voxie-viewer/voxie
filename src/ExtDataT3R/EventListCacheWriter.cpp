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

#include "EventListCacheWriter.hpp"
#include "EventListCacheCommon.hpp"
#include "EventListReader.hpp"
#include "EventListTypes.hpp"

#include <QBitArray>
#include <QDebug>
#include <QIODevice>
#include <QVector>

namespace vx {
namespace t3r {

EventListCacheWriter::EventListCacheWriter(EventListReader& reader,
                                           QIODevice* outputFile)
    : reader(reader), stream(outputFile) {
  initializeSegments();
  mergeSegments();
  resizeSegments();
  populateSegments();
  setSegmentEventCounts();
}

void EventListCacheWriter::write() {
  stream.setByteOrder(QDataStream::LittleEndian);
  stream.setVersion(QDataStream::Qt_5_5);

  // Write header
  stream.writeRawData(cache::MagicHeader.c_str(), cache::MagicHeader.size());
  stream << cache::Version;

  // Write segment data
  stream << quint32(segments.size());
  for (size_t i = 0; i < segments.size(); ++i) {
    stream << segments[i];
  }
}

void EventListCacheWriter::setTargetIntervalSize(uint64_t targetIntervalSize) {
  this->targetIntervalSize = targetIntervalSize;
}

uint64_t EventListCacheWriter::getTargetIntervalSize() const {
  return targetIntervalSize;
}

cache::Segment* EventListCacheWriter::getSegmentContainingTimestamp(
    Timestamp timestamp, Timestamp margin) {
  for (size_t i = 0; i < segments.size(); ++i) {
    if (segments[i].minimumTimestamp - margin <= timestamp &&
        segments[i].maximumTimestamp + margin >= timestamp) {
      return &segments[i];
    }
  }

  return nullptr;
}

void EventListCacheWriter::addOrExpandSegmentForTimestamp(Timestamp timestamp,
                                                          Timestamp margin) {
  if (cache::Segment* segment =
          getSegmentContainingTimestamp(timestamp, margin)) {
    // Expand segment by timestamp range
    segment->minimumTimestamp =
        qMin<Timestamp>(segment->minimumTimestamp, timestamp);
    segment->maximumTimestamp =
        qMax<Timestamp>(segment->maximumTimestamp, timestamp);
  } else {
    cache::Segment newSegment;
    newSegment.minimumTimestamp = timestamp;
    newSegment.maximumTimestamp = timestamp;
    segments.push_back(newSegment);
  }
}

void EventListCacheWriter::initializeSegments() {
  qDebug() << "Initializing segments...";

  static const Timestamp Margin = 10000000;
  reader.seek(0);
  for (EventListReader::Size eventIndex = 0;
       eventIndex < reader.getEventCount(); ++eventIndex) {
    // Obtain event from reader
    auto entry = reader.readEntry();
    if (entry.isValid()) {
      addOrExpandSegmentForTimestamp(entry.timestamp, Margin);
    }
  }
}

void EventListCacheWriter::mergeSegments() {
  qDebug() << "Merging segments...";

  // Remove outliers/singleton segments
  segments.erase(std::remove_if(segments.begin(), segments.end(),
                                [](const cache::Segment& segment) {
                                  return segment.minimumTimestamp ==
                                         segment.maximumTimestamp;
                                }),
                 segments.end());

  if (segments.empty()) {
    return;
  }

  std::sort(segments.begin(), segments.end(),
            [](const cache::Segment& a, const cache::Segment& b) {
              return a.minimumTimestamp != b.minimumTimestamp
                         ? a.minimumTimestamp < b.minimumTimestamp
                         : a.maximumTimestamp < b.maximumTimestamp;
            });

  std::vector<cache::Segment> mergedSegments;

  cache::Segment segment = segments[0];
  for (size_t i = 1; i != segments.size(); ++i) {
    if (segment.maximumTimestamp >= segments[i].minimumTimestamp) {
      segment.maximumTimestamp = segments[i].minimumTimestamp;
    } else {
      mergedSegments.push_back(segment);
      segment = segments[i];
    }
  }

  mergedSegments.push_back(segment);
  segments = std::move(mergedSegments);

  qDebug() << segments.size() << "segments found";
}

void EventListCacheWriter::resizeSegments() {
  qDebug() << "Resizing segments...";

  int64_t totalIntervalCount =
      qMax<int64_t>(1, reader.getEventCount() / targetIntervalSize);

  // Get effective timestamp range across all segments without gaps
  Timestamp timestampSum = 0;
  for (auto& segment : segments) {
    timestampSum += segment.maximumTimestamp - segment.minimumTimestamp;
  }

  for (auto& segment : segments) {
    Timestamp duration = segment.maximumTimestamp - segment.minimumTimestamp;

    // Compute segment-specific interval count as a fraction of the total
    // interval count, based on the relative timestamp range of the segment
    int64_t intervalCount =
        qMax<int64_t>(1, double(duration) * totalIntervalCount / timestampSum);
    segment.intervalLength = duration / intervalCount;

    // TODO support stride > length (multiple intervals per event)
    segment.intervalStride = segment.intervalLength;

    segment.intervals.resize(intervalCount + 1);
    std::fill(segment.intervals.begin(), segment.intervals.end(),
              cache::InvalidInterval);
  }
}

void EventListCacheWriter::populateSegments() {
  qDebug() << "Populating segments...";

  reader.seek(0);

  size_t invalidCount = 0;
  size_t outlierCount = 0;

  for (EventListReader::Size eventIndex = 0;
       eventIndex < reader.getEventCount(); ++eventIndex) {
    // Obtain event from reader
    auto entry = reader.readEntry();
    if (entry.isValid()) {
      if (cache::Segment* segment =
              getSegmentContainingTimestamp(entry.timestamp, 0)) {
        // Determine which interval this entry falls within
        // TODO support non-zero start offset (>4 GB files)
        int64_t intervalIndex = (entry.timestamp - segment->minimumTimestamp) /
                                segment->intervalLength;

        // Expand the interval to fit the entry
        if (intervalIndex < int64_t(segment->intervals.size())) {
          auto& interval = segment->intervals[intervalIndex];
          interval = interval.expand(eventIndex);
        } else {
          qDebug() << "Entry out of interval bounds" << entry.timestamp
                   << segment->minimumTimestamp << segment->maximumTimestamp
                   << segment->intervalLength << segment->intervals.size();
        }
      } else {
        outlierCount++;
      }
    } else {
      invalidCount++;
    }
  }

  qDebug() << invalidCount << "dummy entries ("
           << ((invalidCount * 100.0) / reader.getEventCount()) << "%)";
  qDebug() << outlierCount << "outliers ("
           << ((outlierCount * 100.0) / reader.getEventCount()) << "%)";
}

void EventListCacheWriter::setSegmentEventCounts() {
  for (auto& segment : segments) {
    for (auto& interval : segment.intervals) {
      segment.eventCount += interval.entryCount;
    }
  }
}

}  // namespace t3r
}  // namespace vx
