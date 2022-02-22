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

#include "ResourcePool.hpp"

#include <VoxieClient/DBusClient.hpp>
#include <VoxieClient/DBusProxies.hpp>
#include <VoxieClient/DBusUtil.hpp>
#include <VoxieClient/QtUtil.hpp>
#include <VoxieClient/VoxieDBus.hpp>

#include <QDebug>
#include <QEnableSharedFromThis>
#include <QList>
#include <QMutex>
#include <QString>

#include <tuple>

namespace vx {
namespace t3r {

class EventListBufferPool : QEnableSharedFromThis<EventListBufferPool> {
 public:
  using AttributeList = QList<
      std::tuple<QString, std::tuple<QString, quint32, QString>, QString,
                 QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>;

  using Buffer = de::uni_stuttgart::Voxie::EventListDataBuffer;

 private:
  struct BufferData {
    QSharedPointer<Buffer> buffer;
    QString service;
    QDBusObjectPath path;
    AttributeList attributes;
    std::size_t capacity = 0;
  };

 public:
  class BufferRef {
   public:
    BufferRef(QSharedPointer<EventListBufferPool> pool,
              QSharedPointer<BufferData> data)
        : pool(pool), data(data) {}

    ~BufferRef() {
      if (auto poolRef = pool.toStrongRef()) {
        poolRef->giveBack(data);
      }
    }

    QSharedPointer<Buffer> buffer() const { return data->buffer; }
    QDBusObjectPath path() const { return data->path; }

   private:
    QWeakPointer<EventListBufferPool> pool;
    QSharedPointer<BufferData> data;
  };

  static QSharedPointer<EventListBufferPool> create(DBusClient& dbusClient) {
    return QSharedPointer<EventListBufferPool>::create(dbusClient);
  }

  QSharedPointer<BufferRef> findOrCreate(const AttributeList& attributes,
                                         std::size_t capacity,
                                         QString service) {
    QSharedPointer<BufferData> bufferData;

    {
      QMutexLocker lock(&mutex);
      bufferData = pool.extract([=](BufferData& data) {
        return data.capacity >= capacity && data.service == service &&
               data.attributes == attributes;
      });
    }

    if (!bufferData) {
      bufferData = QSharedPointer<BufferData>::create();
      bufferData->service = service;
      bufferData->capacity = capacity;
      bufferData->attributes = attributes;
      bufferData->path =
          HANDLEDBUSPENDINGREPLY(dbusClient->CreateEventListDataBuffer(
              dbusClient.clientPath(), capacity, attributes,
              vx::emptyOptions()));
      bufferData->buffer = makeSharedQObject<Buffer>(
          service, bufferData->path.path(), dbusClient.connection());
    }

    return QSharedPointer<BufferRef>::create(sharedFromThis(), bufferData);
  }

 private:
  EventListBufferPool(DBusClient& dbusClient)
      : dbusClient(dbusClient),
        pool([](const BufferData& a, const BufferData& b) {
          return a.capacity < b.capacity;
        }) {}

  void giveBack(QSharedPointer<BufferData> bufferData) {
    QMutexLocker lock(&mutex);
    pool.add(bufferData);
  }

  DBusClient& dbusClient;
  ResourcePool<BufferData> pool;

  QMutex mutex;

  friend class QSharedPointer<EventListBufferPool>;
};

}  // namespace t3r
}  // namespace vx
