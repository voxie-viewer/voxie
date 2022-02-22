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

#include "ClusteredEventListWorker.hpp"
#include "ClusterXRFCorrector.hpp"

#include <VoxieClient/DBusAdaptors.hpp>
#include <VoxieClient/DBusUtil.hpp>
#include <VoxieClient/VoxieDBus.hpp>

namespace vx {
namespace t3r {
ClusteredEventListWorker::ClusteredEventListWorker() {}

void ClusteredEventListWorker::setIOOptions(IOOptions io) { this->io = io; }

const ClusteredEventListWorker::IOOptions&
ClusteredEventListWorker::getIOOptions() const {
  return io;
}

void ClusteredEventListWorker::setReadOptions(ReadOptions options) {
  this->options = options;
}

const ClusteredEventListWorker::ReadOptions&
ClusteredEventListWorker::getReadOptions() const {
  return options;
}

void ClusteredEventListWorker::setClusteringSettings(
    ClusteringSettings clusteringSettings) {
  this->clusteringSettings = clusteringSettings;
}

const ClusteringSettings& ClusteredEventListWorker::getClusteringSettings()
    const {
  return clusteringSettings;
}

ClusteredEventListWorker::ReadResult ClusteredEventListWorker::clusterEvents() {
  if (io.subStreams.empty()) {
    return ReadResult();
  }

  clusters.clear();

  upperTimestampLimit = options.lastTimestamp;

  for (auto& subStream : io.subStreams) {
    readSubStreamData(subStream);
  }

  // If passthrough mode is enabled, can bypass all clustering steps and
  // transform ToT into energy directly
  if (!clusteringSettings.passthrough) {
    for (const auto& subStream : io.subStreams) {
      clusterSubStream(subStream);
    }
  }

  return writeResultsToOutputBuffer();
}

void ClusteredEventListWorker::readSubStreamData(SubStream& subStream) {
  qlonglong maxTS = upperTimestampLimit;
  QString unusedVersionString;

  // TODO: Do this in a better way. Use a separate accessor for long-running
  // operations?
  io.accessor->setTimeout(INT_MAX);

  subStream.readEventCount = HANDLEDBUSPENDINGREPLY(io.accessor->ReadEvents(
      subStream.id, options.firstTimestamp, upperTimestampLimit, 0,
      options.readBufferSize,
      std::tuple<QString, QDBusObjectPath>(io.dbusClient->uniqueName(),
                                           subStream.inputProxyPath),
      vx::emptyOptions(), maxTS, unusedVersionString));

  if (subStream.readEventCount > 0) {
    upperTimestampLimit = std::min<Timestamp>(upperTimestampLimit, maxTS);
  }
}

void ClusteredEventListWorker::clusterSubStream(const SubStream& subStream) {
  auto bucketer = std::make_unique<EventBucketer>(subStream.input,
                                                  subStream.readEventCount);
  auto clusterer =
      std::make_unique<EventClusterer>(*subStream.detector, *bucketer);

  size_t existingClusterCount = clusters.size();

  // TODO use filter properties to decide parameters
  clusterer->clusterEvents(clusters, options.firstTimestamp,
                           upperTimestampLimit,
                           clusteringSettings.temporalMargin);

  auto subClusterIterator = clusters.begin() + existingClusterCount;

  // Sort clusters by timestamp (needed for XRF correction and merging)
  std::sort(subClusterIterator, clusters.end());

  if (clusteringSettings.xrf.enabled) {
    // Perform XRF correction
    ClusterXRFCorrector xrfCorrector(clusteringSettings.xrf);
    xrfCorrector.performXRFCorrection(clusters, existingClusterCount);
  }

  // Sort full cluster list
  std::inplace_merge(clusters.begin(), subClusterIterator, clusters.end());
}

ClusteredEventListWorker::ReadResult
ClusteredEventListWorker::writeResultsToOutputBuffer() {
  ReadResult result;
  result.valid = true;

  // Obtain attribute buffers
  auto bufferX = io.output.getBuffer<float>("x");
  auto bufferY = io.output.getBuffer<float>("y");
  auto bufferTime = io.output.getBuffer<Timestamp>("timestamp");
  auto bufferEnergy = io.output.getBuffer<float>("energy");

  if (options.targetEventOffset >= io.output.getBufferSize()) {
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.ExtFilterEventListClustering",
        "ClusteredEventListProvider: targetEventOffset out of bounds");
  } else if (options.targetEventCount > io.output.getBufferSize()) {
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.ExtFilterEventListClustering",
        "ClusteredEventListProvider: targetEventCount out of bounds");
  }

  Size offset = options.targetEventOffset;

  if (clusteringSettings.passthrough) {
    result.lastReadTimestamp = options.firstTimestamp;

    // Passthrough path: bypass actual clustering logic and internal buffers
    for (const auto& subStream : io.subStreams) {
      auto inputX = subStream.input.getBuffer<const Coord>("x");
      auto inputY = subStream.input.getBuffer<const Coord>("y");
      auto inputTime = subStream.input.getBuffer<const Timestamp>("timestamp");
      auto inputTot =
          subStream.input.getBuffer<const ShortTimestamp>("timeOverThreshold");

      Size eventCount = subStream.readEventCount;
      const auto& detector = *subStream.detector;

      for (Size index = 0; index < eventCount; ++index) {
        Coord x = inputX(index), y = inputY(index);
        float fx = x, fy = y;
        detector.transformPoint(fx, fy);
        Timestamp timestamp = inputTime(index);
        bufferX(offset) = fx;
        bufferY(offset) = fy;
        bufferTime(offset) = timestamp;
        bufferEnergy(offset) =
            detector.calibration.getEnergy(x, y, inputTot(index));
        result.lastReadTimestamp =
            std::max(timestamp, result.lastReadTimestamp);
        offset++;
      }

      result.readEventCount += eventCount;
    }
  } else {
    Size readEventCount =
        std::min<Size>(clusters.size(), options.targetEventCount);

    for (Size index = 0; index < readEventCount; ++index) {
      auto& cluster = clusters[index];
      bufferX(offset) = cluster.x;
      bufferY(offset) = cluster.y;
      bufferTime(offset) = cluster.timestamp;
      bufferEnergy(offset) = cluster.energy;
      offset++;
    }

    if (readEventCount != 0) {
      result.lastReadTimestamp = clusters[readEventCount - 1].timestamp;
    } else {
      // Return "reverse" timestamp bounds when no clusters were read
      result.lastReadTimestamp = options.firstTimestamp;
    }

    result.readEventCount = clusters.size();
  }

  vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion> version(
      *io.dbusClient,
      HANDLEDBUSPENDINGREPLY((*io.update)
                                 ->Finish(io.dbusClient->clientPath(),
                                          QMap<QString, QDBusVariant>())));

  result.versionString = version->versionString();

  return result;
}

}  // namespace t3r
}  // namespace vx
