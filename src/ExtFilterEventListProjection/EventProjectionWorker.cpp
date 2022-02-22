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

#include "EventProjectionWorker.hpp"

#include "EventProjectionProvider.hpp"
#include "EventProjectionTask.hpp"

// TODO fix include path
#include "../ExtDataT3R/EventListBufferGroup.hpp"

#include <VoxieClient/VoxieDBus.hpp>

using namespace vx;
using namespace vx::t3r;

static const double timestampToSec = 1e-9 * 25.0 / 16.0;

EventProjectionWorker::EventProjectionWorker(EventProjectionTask& task)
    : task(task), provider(task.getProvider()) {}

QThreadStorage<EventListBufferGroup> EventProjectionWorker::bufferStorage;

QSharedPointer<ProjectionImage> EventProjectionWorker::process() {
  vx::DBusClient& dbusClient = provider.dbusClient;
  const auto& attributes = task.attributes;

  QString service = dbusClient.uniqueName();

  if (!bufferStorage.hasLocalData() ||
      bufferStorage.localData().getProxy() == nullptr ||
      bufferStorage.localData().getBufferSize() < blockSize ||
      bufferStorage.localData().getProxy()->attributes() != attributes) {
    bufferStorage.setLocalData(executeOnMainThread([&]() {
      auto path = HANDLEDBUSPENDINGREPLY(dbusClient->CreateEventListDataBuffer(
          dbusClient.clientPath(), blockSize, attributes, vx::emptyOptions()));
      auto buffer =
          makeSharedQObject<de::uni_stuttgart::Voxie::EventListDataBuffer>(
              service, path.path(), dbusClient.connection());
      return EventListBufferGroup(buffer);
    }));
  }

  auto& bufferGroup = bufferStorage.localData();

  std::tuple<QString, QDBusObjectPath> bufferPath(
      service, QDBusObjectPath(bufferGroup.getProxy()->path()));

  auto projection = QSharedPointer<ProjectionImage>::create(task.region.width,
                                                            task.region.height);

  projection->setExposureTime(timestampToSec * (end - start));

  auto countProjection = QSharedPointer<ProjectionImage>::create(
      task.region.width, task.region.height);

  qlonglong lastReadTimestamp = 0;
  QString versionString;

  auto projectionAttribute = provider.getProjectionSettings().attribute;
  auto projectionMode = provider.getProjectionSettings().mode;

  // Process events in blocks
  while (true) {
    // TODO: Do this in a better way. Use a separate accessor for long-running
    // operations?
    provider.accessor->setTimeout(INT_MAX);

    qulonglong readEventCount =
        HANDLEDBUSPENDINGREPLY(provider.accessor->ReadEvents(
            streamID, start, end, 0, blockSize, bufferPath, vx::emptyOptions(),
            lastReadTimestamp, versionString));

    if (readEventCount == 0) {
      break;
    }

    // Increase minimum timestamp for next read
    start = std::max<Timestamp>(start, lastReadTimestamp) + 1;

    float scaleX = projection->getWidth() / boundingBox[2];
    float scaleY = projection->getHeight() / boundingBox[3];
    float offX = boundingBox[0] * scaleX + task.region.inputX;
    float offY = boundingBox[1] * scaleY + task.region.inputY;

    // Obtain attribute buffers
    auto bufferX = bufferGroup.getBuffer<const float>("x");
    auto bufferY = bufferGroup.getBuffer<const float>("y");
    auto bufferEnergy = bufferGroup.getBuffer<const float>("energy");

    auto performAggregation = [&](const auto& attribBuffer, auto func) {
      for (size_t i = 0; i < readEventCount; ++i) {
        float energy = bufferEnergy(i);
        if (energy >= minEnergy && energy <= maxEnergy) {
          ProjectionImage::Coord x = bufferX(i) * scaleX - offX;
          ProjectionImage::Coord y = bufferY(i) * scaleY - offY;
          if (qFuzzyIsNull(countProjection->get(x, y))) {
            // Initialize first value without aggregation function
            projection->set(x, y, projection->get(x, y));
            countProjection->add(x, y, 1);
          } else {
            // Apply aggregation function for subsequent values
            projection->set(x, y, func(projection->get(x, y), attribBuffer(i)));
          }
        }
      }
    };

    auto performProjection = [&](const auto& attribBuffer) {
      switch (projectionMode) {
        case ProjectionMode::Sum:
          for (size_t i = 0; i < readEventCount; ++i) {
            float energy = bufferEnergy(i);
            if (energy >= minEnergy && energy <= maxEnergy) {
              // Add events onto projection with subpixel accuracy
              projection->splatAdd(bufferX(i) * scaleX - offX,
                                   bufferY(i) * scaleY - offY, attribBuffer(i));
            }
          }
          break;
        case ProjectionMode::Mean:
          for (size_t i = 0; i < readEventCount; ++i) {
            float energy = bufferEnergy(i);
            if (energy >= minEnergy && energy <= maxEnergy) {
              float x = bufferX(i) * scaleX - offX;
              float y = bufferY(i) * scaleY - offY;
              // Add events onto projection with subpixel accuracy
              projection->splatAdd(x, y, attribBuffer(i));
              // Additionally, splat event count to compute per-pixel mean
              countProjection->splatAdd(x, y, 1);
            }
          }
          break;
        case ProjectionMode::Minimum:
          performAggregation(attribBuffer, &qMin<double>);
          break;
        case ProjectionMode::Maximum:
          performAggregation(attribBuffer, &qMax<double>);
          break;
      }
    };

    switch (projectionAttribute) {
      case ProjectionAttribute::Energy:
        performProjection(bufferEnergy);
        break;
      case ProjectionAttribute::EventCount:
        performProjection([](size_t) { return 1; });
        break;
      case ProjectionAttribute::TimeOfArrival: {
        auto bufferTime = bufferGroup.getBuffer<const Timestamp>("timestamp");
        performProjection([&bufferTime](size_t x) {
          // Convert 25/16ns timestamp units to seconds
          return bufferTime(x) * timestampToSec;
        });
      } break;
      default:
        break;
    }
  }

  // In case of 'mean' projection mode, normalize output to event count
  if (projectionMode == ProjectionMode::Mean) {
    for (ProjectionImage::Coord y = 0; y < projection->getHeight(); ++y) {
      for (ProjectionImage::Coord x = 0; x < projection->getWidth(); ++x) {
        double count = countProjection->get(x, y);
        if (!qFuzzyIsNull(count)) {
          projection->set(x, y, projection->get(x, y) / count);
        }
      }
    }
  }

  return projection;
}
