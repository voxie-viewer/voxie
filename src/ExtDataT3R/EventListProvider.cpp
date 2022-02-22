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

#include "EventListProvider.hpp"

namespace vx {
namespace t3r {

class EventListDataAccessorOperationsAdaptorImpl
    : public EventListDataAccessorOperationsAdaptor {
 public:
  EventListDataAccessorOperationsAdaptorImpl(EventListProvider* provider)
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
      return provider->getMetadata();
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
            "de.uni_stuttgart.Voxie.ExtDataT3R.Error",
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
      // TODO handle call asynchronously
      ExportedObject::checkOptions(options, {"Sorted"});

      EventListProvider::ReadOptions readOptions;
      readOptions.streamID = streamID;
      readOptions.firstTimestamp = firstTimestamp;
      readOptions.lastTimestamp = lastTimestamp;
      readOptions.targetEventOffset = targetEventOffset;
      readOptions.targetEventCount = targetEventCount;
      readOptions.output = output;
      readOptions.sorted =
          ExportedObject::getOptionValueOrDefault(options, "Sorted", false);

      auto result = provider->readEvents(readOptions);
      if (!result.valid) {
        throw vx::Exception(
            "de.uni_stuttgart.Voxie.ExtDataT3R.Error",
            QStringLiteral("Failed to read events within stream %1")
                .arg(streamID));
      }

      lastReadTimestamp = result.effectiveUpperBound;
      versionString = result.versionString;
      return result.result.count;
    } catch (Exception& e) {
      e.handle(provider);
      return 0;
    }
  }

 private:
  EventListProvider* provider;
};

EventListProvider::EventListProvider(vx::DBusClient& dbusClient)
    : RefCountedObject("EventListAccessor"), dbusClient(dbusClient) {
  new EventListDataAccessorOperationsAdaptorImpl(this);
}

void EventListProvider::setMetadata(QMap<QString, QDBusVariant> metadata) {
  this->metadata = metadata;
}

QMap<QString, QDBusVariant> EventListProvider::getMetadata() const {
  return metadata;
}

StreamID EventListProvider::addStream(Stream stream) {
  streams.push_back(stream);
  return streams.size() - 1;
}

std::size_t EventListProvider::getStreamCount() const { return streams.size(); }

EventListProvider::StreamInfo EventListProvider::getStreamInfo(
    StreamID streamID) const {
  StreamInfo info;

  if (streamID >= streams.size()) {
    return info;
  }

  auto& stream = streams[streamID];

  info.valid = true;
  info.eventCount = stream.cache->getEventCount();
  info.minimumTimestamp = stream.cache->getMinimumTimestamp();
  info.maximumTimestamp = stream.cache->getMaximumTimestamp();

  auto addAttribute = [&](QString name, QString displayName, bool isSigned,
                          quint32 size) {
    AttributeInfo attrib;
    attrib.name = name;
    attrib.dataType = isSigned ? "int" : "uint";
    attrib.dataTypeSize = size;
    attrib.byteOrder = size > 8 ? "little" : "none";
    attrib.displayName = displayName;
    info.attributes.push_back(attrib);
  };

  // TODO attribute metadata
  addAttribute("x", "X", false, 8);
  addAttribute("y", "Y", false, 8);
  addAttribute("timestamp", "Time of Activation", true, 64);
  addAttribute("timeOverThreshold", "Time over Threshold", false, 16);

  info.metadata = stream.metadata;

  return info;
}

EventListProvider::ReadResult EventListProvider::readEvents(
    ReadOptions readOptions) {
  ReadResult readResult;
  readResult.result.maxTimestamp = readOptions.firstTimestamp;
  readResult.result.minTimestamp = readOptions.lastTimestamp;

  if (readOptions.streamID >= streams.size()) {
    return readResult;
  }

  auto& stream = streams[readOptions.streamID];

  readResult.valid = true;

  // Obtain intervals to be read
  auto intervals = stream.cache->lookUpBoundedIntervals(
      readOptions.firstTimestamp, readOptions.lastTimestamp,
      readOptions.targetEventCount);

  auto proxy = makeSharedQObject<de::uni_stuttgart::Voxie::EventListDataBuffer>(
      dbusClient.uniqueName(), std::get<1>(readOptions.output).path(),
      dbusClient.connection());

  auto proxyData = makeSharedQObject<de::uni_stuttgart::Voxie::Data>(
      dbusClient.uniqueName(), std::get<1>(readOptions.output).path(),
      dbusClient.connection());

  vx::RefObjWrapper<de::uni_stuttgart::Voxie::ExternalDataUpdate> update(
      dbusClient, HANDLEDBUSPENDINGREPLY(proxyData->CreateUpdate(
                      dbusClient.clientPath(), QMap<QString, QDBusVariant>())));

  EventListBufferGroup bufferGroup(update.path(), proxy);

  EventListReader::ReadOptions options;
  options.minTimestamp = readOptions.firstTimestamp;
  options.maxTimestamp = readOptions.lastTimestamp;
  options.targetStartIndex = readOptions.targetEventOffset;
  options.targetEndIndex =
      readOptions.targetEventOffset + readOptions.targetEventCount;
  options.sorted = readOptions.sorted;

  readResult.effectiveUpperBound = readOptions.lastTimestamp;

  // Compute total number of events across all applicable intervals
  std::size_t totalEntryCount = 0;
  for (auto& interval : intervals) {
    totalEntryCount += interval.entryCount;
  }

  // Enable sorted mode if event count exceeds buffer size
  if (totalEntryCount > readOptions.targetEventCount) {
    options.sorted = true;
  }

  for (auto& interval : intervals) {
    // Select indices to read from
    options.startIndex = interval.start;
    options.endIndex = interval.end;
    auto subResult = stream.reader->readIntoBufferGroup(bufferGroup, options);

    // Shift buffer read start offset forward for next read
    options.targetStartIndex += subResult.count;

    // Add read event count to total
    readResult.result.count += subResult.count;

    // Adjust result timestamps
    readResult.result.minTimestamp =
        std::min(readResult.result.minTimestamp, subResult.minTimestamp);
    readResult.result.maxTimestamp =
        std::max(readResult.result.maxTimestamp, subResult.maxTimestamp);

    // If the buffer was exceeded, the effective upper bound must be adjusted,
    // and no further intervals should be read
    if (readResult.result.bufferExceeded) {
      readResult.effectiveUpperBound = readResult.result.maxTimestamp;
      break;
    }
  }

  vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion> version(
      dbusClient, HANDLEDBUSPENDINGREPLY(update->Finish(
                      dbusClient.clientPath(), QMap<QString, QDBusVariant>())));

  readResult.versionString = version->versionString();

  return readResult;
}

}  // namespace t3r
}  // namespace vx
