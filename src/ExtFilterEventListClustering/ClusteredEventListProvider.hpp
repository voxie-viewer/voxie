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

#include "ClusteredEventListWorker.hpp"
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

class EventListCalibration;

// TODO extract common members of ExtDataT3R/EventListProvider and this class to
// some abstract superclass
class ClusteredEventListProvider : public RefCountedObject {
  Q_OBJECT
  VX_REFCOUNTEDOBJECT

 public:
  using Size = quint64;

  struct AttributeInfo {
    QString name;

    QString dataType;
    quint32 dataTypeSize = 0;
    QString byteOrder;

    QString displayName;
    QMap<QString, QDBusVariant> metadata;
    QMap<QString, QDBusVariant> options;
  };

  struct BoundingBox {
    double minX = 0;
    double minY = 0;
    double maxX = 0;
    double maxY = 0;

    void combine(const BoundingBox& box);
    void transform(const EventClusterer::DetectorData& detectorData);

    static BoundingBox fromDBusVariant(QDBusVariant variant);
    QDBusVariant toDBusVariant() const;
  };

  struct StreamInfo {
    bool valid = false;

    Size eventCount = 0;
    Timestamp minimumTimestamp = 0;
    Timestamp maximumTimestamp = 0;
    QList<AttributeInfo> attributes;
    QMap<QString, QDBusVariant> metadata;
    double pixelSize = 0;

    BoundingBox boundingBox;
  };

  ClusteredEventListProvider(vx::DBusClient& dbusClient);

  QMap<QString, QDBusVariant> getMetadata() const;

  void setClusteringSettings(ClusteringSettings clusteringSettings);

  void setInputAccessor(
      QSharedPointer<de::uni_stuttgart::Voxie::EventListDataAccessorOperations>
          accessor);

  std::size_t getStreamCount() const;
  std::size_t getDetectorCount() const;

  QSharedPointer<EventClusterer::DetectorData> getDetectorData(
      StreamID subStreamID) const;

  StreamInfo getStreamInfo(StreamID streamID) const;

  QSharedPointer<ClusteredEventListWorker> createWorker(
      ClusteredEventListWorker::ReadOptions options,
      std::tuple<QString, QDBusObjectPath> output);

 private:
  QList<StreamID> getSubStreamIDs(StreamID streamID) const;
  StreamInfo getSubStreamInfo(StreamID subStreamID) const;

  using ClusterList = std::vector<EventClusterer::Cluster>;

  QSharedPointer<de::uni_stuttgart::Voxie::EventListDataAccessorOperations>
      accessor;
  ClusteringSettings clusteringSettings;
  QMap<QString, QDBusVariant> metadata;
  vx::DBusClient& dbusClient;

  std::size_t streamCount = 0;
  QList<QSharedPointer<EventClusterer::DetectorData>> detectors;

  QSharedPointer<EventListBufferPool> bufferPool;
};

}  // namespace t3r
}  // namespace vx
