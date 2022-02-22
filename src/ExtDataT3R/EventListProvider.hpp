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

#include "EventListCacheReader.hpp"
#include "EventListReader.hpp"
#include "EventListTypes.hpp"
#include "TimepixRawMetadata.hpp"

#include <VoxieClient/DBusAdaptors.hpp>
#include <VoxieClient/DBusClient.hpp>
#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <vector>

namespace vx {
namespace t3r {

class EventListProvider : public RefCountedObject {
  Q_OBJECT
  REFCOUNTEDOBJ_DECL(EventListProvider)

 public:
  struct Stream {
    QSharedPointer<EventListReader> reader;
    QSharedPointer<EventListCacheReader> cache;
    QMap<QString, QDBusVariant> metadata;
  };

  struct AttributeInfo {
    QString name;

    QString dataType;
    quint32 dataTypeSize = 0;
    QString byteOrder;

    QString displayName;
    QMap<QString, QDBusVariant> metadata;
    QMap<QString, QDBusVariant> options;
  };

  struct StreamInfo {
    bool valid = false;

    EventListReader::Size eventCount = 0;
    Timestamp minimumTimestamp = 0;
    Timestamp maximumTimestamp = 0;
    QList<AttributeInfo> attributes;
    QMap<QString, QDBusVariant> metadata;
  };

  struct ReadOptions {
    StreamID streamID = 0;
    Timestamp firstTimestamp = 0;
    Timestamp lastTimestamp = 0;
    EventListReader::Size targetEventOffset = 0;
    EventListReader::Size targetEventCount = 0;
    std::tuple<QString, QDBusObjectPath> output;
    bool sorted = true;
  };

  struct ReadResult {
    bool valid = false;
    QString versionString;
    Timestamp effectiveUpperBound = 0;
    EventListReader::ReadResult result;
  };

  EventListProvider(vx::DBusClient& dbusClient);

  void setMetadata(QMap<QString, QDBusVariant> metadata);
  QMap<QString, QDBusVariant> getMetadata() const;

  StreamID addStream(Stream stream);
  std::size_t getStreamCount() const;
  StreamInfo getStreamInfo(StreamID streamID) const;

  ReadResult readEvents(ReadOptions options);

 private:
  std::vector<Stream> streams;
  QMap<QString, QDBusVariant> metadata;
  vx::DBusClient& dbusClient;
};

}  // namespace t3r
}  // namespace vx
