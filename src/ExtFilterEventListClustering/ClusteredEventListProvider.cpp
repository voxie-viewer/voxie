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

#include "ClusteredEventListProvider.hpp"

#include "ClusterXRFCorrector.hpp"
#include "DBusNumericUtil.hpp"
#include "EventBucketer.hpp"
#include "EventClusterer.hpp"
#include "EventListCalibration.hpp"

// TODO fix include path
#include "../ExtDataT3R/EventListBufferGroup.hpp"

#include <VoxieClient/DBusUtil.hpp>
#include <VoxieClient/JsonDBus.hpp>
#include <VoxieClient/VoxieDBus.hpp>

#include <VoxieClient/ObjectExport/DBusCallUtil.hpp>

namespace vx {
namespace t3r {

void ClusteredEventListProvider::BoundingBox::combine(const BoundingBox& box) {
  minX = std::min(minX, box.minX);
  minY = std::min(minY, box.minY);
  maxX = std::max(maxX, box.maxX);
  maxY = std::max(maxY, box.maxY);
}

void ClusteredEventListProvider::BoundingBox::transform(
    const EventClusterer::DetectorData& detectorData) {
  double tlX = minX, tlY = minY, blX = minX, blY = maxY;
  double trX = maxX, trY = minY, brX = maxX, brY = maxY;

  detectorData.transformPoint(tlX, tlY);
  detectorData.transformPoint(trX, trY);
  detectorData.transformPoint(blX, blY);
  detectorData.transformPoint(brX, brY);

  minX = std::min(std::min(tlX, trX), std::min(blX, brX));
  minY = std::min(std::min(tlY, trY), std::min(blY, brY));
  maxX = std::max(std::max(tlX, trX), std::max(blX, brX));
  maxY = std::max(std::max(tlY, trY), std::max(blY, brY));
}

ClusteredEventListProvider::BoundingBox
ClusteredEventListProvider::BoundingBox::fromDBusVariant(QDBusVariant variant) {
  BoundingBox box;
  auto variantList = dbusGetVariantValue<QList<QDBusVariant>>(variant);
  if (variantList.size() == 4) {
    box.minX = dbusGetNumber<double>(variantList[0]);
    box.minY = dbusGetNumber<double>(variantList[1]);
    box.maxX = dbusGetNumber<double>(variantList[2]) + box.minX;
    box.maxY = dbusGetNumber<double>(variantList[3]) + box.minY;
  }
  return box;
}

QDBusVariant ClusteredEventListProvider::BoundingBox::toDBusVariant() const {
  return jsonToDBus(
      QJsonValue(QJsonArray({minX, minY, maxX - minX, maxY - minY})));
}

class EventListDataAccessorOperationsAdaptorImpl
    : public EventListDataAccessorOperationsAdaptor {
 public:
  EventListDataAccessorOperationsAdaptorImpl(
      ClusteredEventListProvider* provider)
      : EventListDataAccessorOperationsAdaptor(provider), provider(provider) {}

  virtual qulonglong streamCount() const override {
    try {
      return provider->getStreamCount();
    } catch (Exception& e) {
      e.handle(provider);
      return 0;
    }
  }

 public Q_SLOTS:
  virtual QMap<QString, QDBusVariant> GetMetadata(
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);
      return QMap<QString, QDBusVariant>();
    } catch (Exception& e) {
      e.handle(provider);
      return QMap<QString, QDBusVariant>();
    }
  }

  virtual qulonglong GetStreamInfo(
      qulonglong streamID, const QMap<QString, QDBusVariant>& options,
      qlonglong& minimumTimestamp, qlonglong& maximumTimestamp,
      QList<std::tuple<QString, std::tuple<QString, quint32, QString>, QString,
                       QMap<QString, QDBusVariant>,
                       QMap<QString, QDBusVariant>>>& attributes,
      QMap<QString, QDBusVariant>& metadata) override {
    try {
      ExportedObject::checkOptions(options);

      auto streamInfo = provider->getStreamInfo(streamID);
      if (!streamInfo.valid) {
        throw vx::Exception(
            "de.uni_stuttgart.Voxie.ExtFilterEventListClustering.Error",
            QStringLiteral("Failed to get info for stream %1").arg(streamID));
      }

      minimumTimestamp = streamInfo.minimumTimestamp;
      maximumTimestamp = streamInfo.maximumTimestamp;

      for (auto& attrib : streamInfo.attributes) {
        attributes.push_back(std::make_tuple(
            attrib.name,
            std::make_tuple(attrib.dataType, attrib.dataTypeSize,
                            attrib.byteOrder),
            attrib.displayName, attrib.metadata, attrib.options));
      }

      metadata = streamInfo.metadata;

      return streamInfo.eventCount;
    } catch (Exception& e) {
      e.handle(provider);
      return 0;
    }
  }

  virtual qulonglong ReadEvents(
      qulonglong streamID, qlonglong firstTimestamp, qlonglong lastTimestamp,
      qulonglong targetEventOffset, qulonglong targetEventCount,
      const std::tuple<QString, QDBusObjectPath>& output,
      const QMap<QString, QDBusVariant>& options, qlonglong& lastReadTimestamp,
      QString& versionString) override {
    try {
      ExportedObject::checkOptions(options);

      lastReadTimestamp = 0;
      versionString.clear();

      ClusteredEventListWorker::ReadOptions readOptions;
      readOptions.streamID = streamID;
      readOptions.firstTimestamp = firstTimestamp;
      readOptions.lastTimestamp = lastTimestamp;
      readOptions.targetEventOffset = targetEventOffset;
      readOptions.targetEventCount = targetEventCount;
      readOptions.readBufferSize =
          targetEventCount / provider->getDetectorCount();

      // Initialize worker data on main thread
      auto worker = provider->createWorker(readOptions, output);

      vx::handleDBusCallOnBackgroundThread(provider, [worker]() {
        // Perform event reading/clustering in new thread
        auto result = worker->clusterEvents();

        if (!result.valid) {
          throw vx::Exception(
              "de.uni_stuttgart.Voxie.ExtFilterEventListClustering.Error",
              QStringLiteral("Failed to read events within stream %1")
                  .arg(worker->getReadOptions().streamID));
        }

        return QList<QDBusVariant>{
            vx::dbusMakeVariant<qulonglong>(result.readEventCount),
            vx::dbusMakeVariant<qlonglong>(result.lastReadTimestamp),
            vx::dbusMakeVariant(result.versionString),
        };
      });
    } catch (Exception& e) {
      e.handle(provider);
    }
    return 0;
  }

 private:
  ClusteredEventListProvider* provider;
};

ClusteredEventListProvider::ClusteredEventListProvider(
    vx::DBusClient& dbusClient)
    : RefCountedObject("EventListAccessor"),
      dbusClient(dbusClient),
      bufferPool(EventListBufferPool::create(dbusClient)) {
  new EventListDataAccessorOperationsAdaptorImpl(this);
}

QMap<QString, QDBusVariant> ClusteredEventListProvider::getMetadata() const {
  return metadata;
}

void ClusteredEventListProvider::setClusteringSettings(
    ClusteringSettings clusteringSettings) {
  this->clusteringSettings = clusteringSettings;
}

void ClusteredEventListProvider::setInputAccessor(
    QSharedPointer<de::uni_stuttgart::Voxie::EventListDataAccessorOperations>
        accessor) {
  this->accessor = accessor;
  detectors.clear();

  metadata = HANDLEDBUSPENDINGREPLY(accessor->GetMetadata(vx::emptyOptions()));

  // Get working directory
  QDir workingDirectory(
      dbusGetVariantValue<QString>(metadata.value("WorkingDirectory")));

  // Get detector metadata list
  auto detectorList =
      dbusGetVariantValue<QList<QDBusVariant>>(metadata.value("Detectors"));

  for (const auto& detectorEntry : detectorList) {
    detectors.append(QSharedPointer<EventClusterer::DetectorData>::create(
        workingDirectory,
        dbusGetVariantValue<QMap<QString, QDBusVariant>>(detectorEntry)));
  }

  streamCount = accessor->streamCount() / getDetectorCount();
}

std::size_t ClusteredEventListProvider::getStreamCount() const {
  return streamCount;
}

std::size_t ClusteredEventListProvider::getDetectorCount() const {
  return qMax<std::size_t>(detectors.size(), 1);
}

QSharedPointer<EventClusterer::DetectorData>
ClusteredEventListProvider::getDetectorData(StreamID subStreamID) const {
  if (detectors.empty()) {
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.ExtFilterEventListClustering.Error",
        "Missing detector metadata");
  }
  return detectors[subStreamID % detectors.size()];
}

ClusteredEventListProvider::StreamInfo
ClusteredEventListProvider::getStreamInfo(StreamID streamID) const {
  StreamInfo info;

  auto ids = getSubStreamIDs(streamID);
  if (ids.empty()) {
    return info;
  }

  info.valid = true;

  BoundingBox boundingBox;

  // Read information from all substreams and combine it
  for (StreamID id : ids) {
    auto subInfo = getSubStreamInfo(id);
    info.valid = info.valid && subInfo.valid;
    info.eventCount += subInfo.eventCount;
    info.minimumTimestamp =
        qMin(info.minimumTimestamp, subInfo.minimumTimestamp);
    info.maximumTimestamp =
        qMax(info.maximumTimestamp, subInfo.maximumTimestamp);
    info.metadata = subInfo.metadata;
    if (subInfo.pixelSize != 0) {
      if (info.pixelSize == 0)
        info.pixelSize = subInfo.pixelSize;
      else
        info.pixelSize = std::min(info.pixelSize, subInfo.pixelSize);
    }

    // Get transformed bounding box
    boundingBox.combine(subInfo.boundingBox);
  }

  // Override bounding box
  info.metadata.insert("BoundingBox", boundingBox.toDBusVariant());
  if (info.pixelSize != 0)
    info.metadata.insert("PixelSize", dbusMakeVariant<double>(info.pixelSize));

  auto addAttribute = [&](QString name, QString displayName, QString type,
                          quint32 size) {
    AttributeInfo attrib;
    attrib.name = name;
    attrib.dataType = type;
    attrib.dataTypeSize = size;
    attrib.byteOrder = size > 8 ? "little" : "none";
    attrib.displayName = displayName;
    info.attributes.push_back(attrib);
  };

  // TODO attribute metadata
  info.attributes.clear();
  addAttribute("x", "X", "float", 32);
  addAttribute("y", "Y", "float", 32);
  addAttribute("timestamp", "Time of Activation", "int", 64);
  addAttribute("energy", "Energy", "float", 32);

  return info;
}

QSharedPointer<ClusteredEventListWorker>
ClusteredEventListProvider::createWorker(
    ClusteredEventListWorker::ReadOptions options,
    std::tuple<QString, QDBusObjectPath> output) {
  ClusteredEventListWorker::IOOptions io;

  QString dbusService = dbusClient.uniqueName();

  io.dbusClient = &dbusClient;
  io.accessor = accessor;

  // Initialize output proxy
  auto proxy = makeSharedQObject<de::uni_stuttgart::Voxie::EventListDataBuffer>(
      dbusService, std::get<1>(output).path(), dbusClient.connection());

  auto proxyData = makeSharedQObject<de::uni_stuttgart::Voxie::Data>(
      dbusService, std::get<1>(output).path(), dbusClient.connection());

  io.update = QSharedPointer<
      vx::RefObjWrapper<de::uni_stuttgart::Voxie::ExternalDataUpdate>>::
      create(dbusClient,
             HANDLEDBUSPENDINGREPLY(proxyData->CreateUpdate(
                 dbusClient.clientPath(), QMap<QString, QDBusVariant>())));

  io.output = EventListBufferGroup(io.update->path(), proxy);

  for (auto subStreamID : getSubStreamIDs(options.streamID)) {
    ClusteredEventListWorker::SubStream subStream;
    subStream.id = subStreamID;
    subStream.detector = getDetectorData(subStreamID);

    QList<std::tuple<QString, std::tuple<QString, quint32, QString>, QString,
                     QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>
        attributes;

    {
      qlonglong unusedMinTS;
      qlonglong unusedMaxTS;
      QMap<QString, QDBusVariant> unusedMetadata;
      HANDLEDBUSPENDINGREPLY(
          accessor->GetStreamInfo(subStreamID, vx::emptyOptions(), unusedMinTS,
                                  unusedMaxTS, attributes, unusedMetadata));
    }

    subStream.bufferRef = bufferPool->findOrCreate(
        attributes, options.readBufferSize, dbusService);

    subStream.inputProxyPath = subStream.bufferRef->path();
    subStream.input = EventListBufferGroup(subStream.bufferRef->buffer());

    io.subStreams.append(subStream);
  }

  auto worker = QSharedPointer<ClusteredEventListWorker>::create();
  worker->setReadOptions(options);
  worker->setIOOptions(io);
  worker->setClusteringSettings(clusteringSettings);
  return worker;
}

QList<StreamID> ClusteredEventListProvider::getSubStreamIDs(
    StreamID streamID) const {
  QList<StreamID> ids;
  StreamID totalStreamCount = accessor->streamCount();
  StreamID mappedID = streamID * getDetectorCount();
  for (size_t i = 0; i < getDetectorCount() && mappedID < totalStreamCount;
       ++i) {
    ids.append(mappedID++);
  }
  return ids;
}

ClusteredEventListProvider::StreamInfo
ClusteredEventListProvider::getSubStreamInfo(StreamID subStreamID) const {
  ClusteredEventListProvider::StreamInfo info;
  info.valid = true;

  QList<std::tuple<QString, std::tuple<QString, quint32, QString>, QString,
                   QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>
      attributes;
  qlonglong minimumTimestamp = 0, maximumTimestamp = 0;

  // TODO this is the *unclustered* event count, as we do not know the cluster
  // count ahead of time
  info.eventCount = HANDLEDBUSPENDINGREPLY(
      accessor->GetStreamInfo(subStreamID, vx::emptyOptions(), minimumTimestamp,
                              maximumTimestamp, attributes, info.metadata));

  info.minimumTimestamp = minimumTimestamp;
  info.maximumTimestamp = maximumTimestamp;

  // Get bounding box from metadata
  if (info.metadata.contains("BoundingBox")) {
    info.boundingBox =
        BoundingBox::fromDBusVariant(info.metadata["BoundingBox"]);
    info.boundingBox.transform(*getDetectorData(subStreamID));
  }
  if (info.metadata.contains("PixelSize")) {
    info.pixelSize = dbusToJson(info.metadata["PixelSize"]).toDouble();
    auto detectorData = getDetectorData(subStreamID);
    info.pixelSize *=
        std::min(detectorData->pixelSize[0], detectorData->pixelSize[1]);
  }

  return info;
}

}  // namespace t3r
}  // namespace vx
