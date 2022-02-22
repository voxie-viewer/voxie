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

#include "ClusteringSettings.hpp"
#include "EventClusterer.hpp"
#include "EventListBufferPool.hpp"

// TODO fix include path
#include "../ExtDataT3R/EventListTypes.hpp"

#include <VoxieClient/DBusAdaptors.hpp>
#include <VoxieClient/DBusClient.hpp>
#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <memory>
#include <vector>

namespace vx {
namespace t3r {

/**
 * Performs a single clustering operation, including reading, clustering,
 * merging and writing back to a buffer.
 */
class ClusteredEventListWorker {
 public:
  using Size = quint64;

  struct SubStream {
    StreamID id = 0;
    QSharedPointer<EventClusterer::DetectorData> detector;
    EventListBufferGroup input;
    QDBusObjectPath inputProxyPath;
    Size readEventCount = 0;

    QSharedPointer<EventListBufferPool::BufferRef> bufferRef;
  };

  struct IOOptions {
    QList<SubStream> subStreams;

    EventListBufferGroup output;

    QSharedPointer<
        vx::RefObjWrapper<de::uni_stuttgart::Voxie::ExternalDataUpdate>>
        update;
    QSharedPointer<de::uni_stuttgart::Voxie::EventListDataAccessorOperations>
        accessor;

    vx::DBusClient* dbusClient = nullptr;
  };

  struct ReadOptions {
    StreamID streamID = 0;
    Timestamp firstTimestamp = 0;
    Timestamp lastTimestamp = 0;
    Size targetEventOffset = 0;
    Size targetEventCount = 0;
    Size readBufferSize = 0;
  };

  struct ReadResult {
    bool valid = false;

    Timestamp lastReadTimestamp = 0;
    Size readEventCount = 0;
    QString versionString;
  };

  ClusteredEventListWorker();

  void setIOOptions(IOOptions io);
  const IOOptions& getIOOptions() const;

  void setReadOptions(ReadOptions options);
  const ReadOptions& getReadOptions() const;

  void setClusteringSettings(ClusteringSettings clusteringSettings);
  const ClusteringSettings& getClusteringSettings() const;

  ReadResult clusterEvents();

 private:
  void readSubStreamData(SubStream& subStream);
  void clusterSubStream(const SubStream& subStream);
  ReadResult writeResultsToOutputBuffer();

  IOOptions io;
  ReadOptions options;
  ClusteringSettings clusteringSettings;

  std::vector<EventClusterer::Cluster> clusters;

  // Maximum timestamp for filtering clusters. Set to the lowest maximum read
  // timestamp among all substreams.
  Timestamp upperTimestampLimit = 0;
};

}  // namespace t3r
}  // namespace vx
