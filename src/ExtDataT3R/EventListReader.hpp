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

#include "EventListBufferGroup.hpp"
#include "EventListTypes.hpp"

class QIODevice;

namespace vx {
namespace t3r {

class EventListReader {
 public:
  using Size = uint64_t;

  struct ReadOptions {
    Size startIndex = 0;
    Size endIndex = 0;
    Timestamp minTimestamp = 0;
    Timestamp maxTimestamp = 0;
    Size targetStartIndex = 0;
    Size targetEndIndex = 0;
    bool sorted = false;
  };

  struct ReadResult {
    Size count = 0;
    Timestamp minTimestamp = 0;
    Timestamp maxTimestamp = 0;
    bool bufferExceeded = false;
  };

  EventListReader(QIODevice* device);

  /**
   * Returns the one-past-last index within the whole event list file.
   * This includes invalid data entries, such as padding data or dummy frames.
   * Use EventListCacheReader::getEventCount for a more accurate value!
   */
  Size getEventCount() const;

  void seek(Size eventIndex);
  bool endReached() const;

  EventListEntry readEntry();

  ReadResult readIntoBufferGroup(EventListBufferGroup& bufferGroup,
                                 ReadOptions options);

 private:
  static const Size eventSize;

  void readHeader();

  EventListEntry processEntry(const unsigned char* data);

  Size startOffset = 0;
  Size eventCount = 0;

  QIODevice* device;
};

}  // namespace t3r
}  // namespace vx
