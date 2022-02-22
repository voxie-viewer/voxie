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

#include "ProjectionImage.hpp"

#include "EventProjectionCommon.hpp"

#include <VoxieClient/DBusClient.hpp>
#include <VoxieClient/DBusProxies.hpp>
#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <QPair>
#include <QSharedPointer>
#include <QThreadStorage>

#include <array>

namespace vx {
namespace t3r {

class EventProjectionTask;
class EventProjectionProvider;
class EventListBufferGroup;

class EventProjectionWorker {
 public:
  using Timestamp = int64_t;
  using Rect = std::array<float, 4>;

  EventProjectionWorker(EventProjectionTask& task);

  EventProjectionWorker(EventProjectionWorker&& other) = default;
  EventProjectionWorker& operator=(EventProjectionWorker&& other) = default;

  QSharedPointer<ProjectionImage> process();

  StreamID streamID = 0;
  Timestamp start = 0;
  Timestamp end = 0;
  quint64 blockSize = 0;
  Rect boundingBox;
  float minEnergy = 0;
  float maxEnergy = 0;

 private:
  EventProjectionTask& task;
  EventProjectionProvider& provider;

  static QThreadStorage<EventListBufferGroup> bufferStorage;
};

}  // namespace t3r
}  // namespace vx
