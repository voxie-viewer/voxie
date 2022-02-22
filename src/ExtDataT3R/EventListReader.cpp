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

#include "EventListReader.hpp"

#include "T3RLookupTables.hpp"

#include <VoxieClient/Exception.hpp>

#include <QDebug>
#include <QIODevice>

namespace vx {
namespace t3r {

const EventListReader::Size EventListReader::eventSize = 8;

EventListReader::EventListReader(QIODevice* device) : device(device) {
  readHeader();
}

void EventListReader::seek(Size eventIndex) {
  device->seek(startOffset + eventIndex * eventSize);
}

bool EventListReader::endReached() const {
  return (device->pos() - startOffset) / eventSize >= eventCount;
}

EventListEntry EventListReader::readEntry() {
  std::array<char, eventSize> buffer;

  if (device->read(buffer.data(), buffer.size()) < (int64_t)buffer.size()) {
    qWarning() << "Unexpected end of file";
    return InvalidEntry;
  } else {
    auto entry =
        processEntry(reinterpret_cast<const unsigned char*>(buffer.data()));
    if (entry.isValid()) {
      return entry;
    } else {
      // Try correcting the offset for the next read
      device->seek(device->pos() - 4);
      return entry;
    }
  }
}

EventListReader::ReadResult EventListReader::readIntoBufferGroup(
    EventListBufferGroup& bufferGroup, ReadOptions options) {
  ReadResult result;

  // Return "reverse" timestamp bounds if no events were read
  result.minTimestamp = options.maxTimestamp;
  result.maxTimestamp = options.minTimestamp;

  if (options.startIndex >= options.endIndex ||
      options.targetStartIndex >= options.targetEndIndex) {
    return result;
  }

  // Obtain attribute buffers
  auto bufferX = bufferGroup.getBuffer<Coord>("x");
  auto bufferY = bufferGroup.getBuffer<Coord>("y");
  auto bufferTime = bufferGroup.getBuffer<Timestamp>("timestamp");
  auto bufferTot = bufferGroup.getBuffer<ShortTimestamp>("timeOverThreshold");

  if (options.startIndex >= getEventCount()) {
    throw vx::Exception("de.uni_stuttgart.Voxie.ExtDataT3R",
                        "EventListReader: startIndex out of bounds");
  } else if (options.endIndex > getEventCount()) {
    throw vx::Exception("de.uni_stuttgart.Voxie.ExtDataT3R",
                        "EventListReader: endIndex out of bounds");
  } else if (options.targetStartIndex >= bufferGroup.getBufferSize()) {
    throw vx::Exception("de.uni_stuttgart.Voxie.ExtDataT3R",
                        "EventListReader: targetStartIndex out of bounds");
  } else if (options.targetEndIndex > bufferGroup.getBufferSize()) {
    throw vx::Exception("de.uni_stuttgart.Voxie.ExtDataT3R",
                        "EventListReader: targetEndIndex out of bounds");
  }

  seek(options.startIndex);

  Size offset = options.targetStartIndex;

  auto append = [&](const EventListEntry& entry) {
    // Add entry data to buffer group
    bufferX(offset) = entry.x;
    bufferY(offset) = entry.y;
    bufferTime(offset) = entry.timestamp;
    bufferTot(offset) = entry.timeOverThreshold;
    ++offset;
  };

  auto isEntryInRange = [&](const EventListEntry& entry) {
    return entry.isValid() && entry.timestamp >= options.minTimestamp &&
           entry.timestamp <= options.maxTimestamp;
  };

  if (options.sorted) {
    std::size_t limit = options.targetEndIndex - options.targetStartIndex;

    // Read entries into vector first
    std::vector<EventListEntry> entries;
    entries.reserve(limit);

    for (auto index = options.startIndex; index < options.endIndex; ++index) {
      auto entry = readEntry();
      if (isEntryInRange(entry)) {
        entries.push_back(entry);
      }
    }

    std::sort(entries.begin(), entries.end(),
              [](const EventListEntry& e1, const EventListEntry& e2) {
                return e1.timestamp < e2.timestamp;
              });

    // Reduce entry count to size limit
    if (entries.size() > limit) {
      result.bufferExceeded = true;
      entries.resize(limit);
    }

    // Determine minimum/maximum timestamps
    if (!entries.empty()) {
      result.minTimestamp = entries.front().timestamp;
      result.maxTimestamp = entries.back().timestamp;
    }

    // Write entries to buffer
    for (const auto& entry : entries) {
      append(entry);
    }
  } else {
    for (auto index = options.startIndex; index < options.endIndex; ++index) {
      if (offset >= options.targetEndIndex) {
        // Read entry count exceeds target range end index: abort
        result.bufferExceeded = true;
        break;
      }
      auto entry = readEntry();
      if (isEntryInRange(entry)) {
        append(entry);
        result.minTimestamp = std::min(result.minTimestamp, entry.timestamp);
        result.maxTimestamp = std::max(result.maxTimestamp, entry.timestamp);
      }
    }
  }

  result.count = offset - options.targetStartIndex;
  return result;
}

void EventListReader::readHeader() {
  device->seek(0);

  while (true) {
    // Buffer to hold a single packet
    std::array<char, 4> buffer;

    // Peek first byte to check if the packet type matches
    if (device->peek(buffer.data(), 1) < 1) {
      // End of file reached
      break;
    }

    if ((buffer[0] & 0xF0) == 0xB0) {
      // Start of data reached
      break;
    } else if (device->read(buffer.data(), buffer.size()) <
               (int64_t)buffer.size()) {
      throw vx::Exception("de.uni_stuttgart.Voxie.ExtDataT3R",
                          "Malformed T3R (unexpected end of file)");
    }

    // Increment data starting offset
    startOffset += buffer.size();
  }

  eventCount = device->size() / eventSize - startOffset;
}

EventListEntry EventListReader::processEntry(const unsigned char* data) {
  if ((data[0] & 0xF0) != 0xB0) {
    return InvalidEntry;
  }

  EventListEntry entry;

  uint32_t address =
      ((data[0] & 0x0F) << 12) | (data[1] << 4) | ((data[2] >> 4) & 0x0F);

  uint32_t pix = address & 0x07;
  entry.x = ((address >> 9) & 0x7F) * 2 + pix / 4;
  entry.y = ((address >> 3) & 0x3F) * 4 + pix % 4;

  uint32_t timeOverThreshold =
      ((data[4] & 0x3F) << 4) | ((data[5] >> 4) & 0x0F);

  if (timeOverThreshold >= lookup::TimeOverThreshold.size()) {
    return InvalidEntry;
  } else {
    entry.timeOverThreshold = lookup::TimeOverThreshold[timeOverThreshold];
  }

  uint64_t timeOfActivation =
      ((data[2] & 0x0F) << 10) | (data[3] << 2) | ((data[4] >> 6) & 0x03);

  if (timeOfActivation >= lookup::TimeOfActivation.size()) {
    return InvalidEntry;
  } else {
    timeOfActivation = lookup::TimeOfActivation[timeOfActivation] +
                       (data[6] << 22) + (data[7] << 14);
  }

  uint32_t fastTimeOfActivation =
      (data[5] & 0x0F) + lookup::ColumnShift[entry.x];

  entry.timestamp = timeOfActivation * 16 + fastTimeOfActivation;

  return entry;
}

EventListReader::Size EventListReader::getEventCount() const {
  return eventCount;
}

}  // namespace t3r
}  // namespace vx
