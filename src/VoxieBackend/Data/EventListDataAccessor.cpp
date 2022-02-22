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

#include "EventListDataAccessor.hpp"

#include <QtCore/QCoreApplication>
#include <QtCore/QThread>

#include <VoxieBackend/DBus/ClientWrapper.hpp>
#include <VoxieBackend/DBus/ObjectWrapper.hpp>

#include <VoxieClient/DBusProxies.hpp>
#include <VoxieClient/DBusUtil.hpp>

#include <VoxieClient/ObjectExport/BusManager.hpp>
#include <VoxieClient/ObjectExport/DBusCallUtil.hpp>

using namespace vx;

class EventListDataAccessorOperationsImpl
    : public EventListDataAccessorOperationsAdaptor {
 public:
  EventListDataAccessorOperationsImpl(EventListDataAccessor& accessor)
      : EventListDataAccessorOperationsAdaptor(&accessor), accessor(accessor) {}

  virtual qulonglong streamCount() const override {
    return accessor.getStreamCount();
  }

 public Q_SLOTS:
  virtual QMap<QString, QDBusVariant> GetMetadata(
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      vx::handleDBusCallOnBackgroundThread(
          &accessor, [self = accessor.thisShared(), options]() {
            return QList<QDBusVariant>{
                vx::dbusMakeVariant(self->getMetadata(options))};
          });
    } catch (Exception& e) {
      e.handle(&accessor);
    }
    return QMap<QString, QDBusVariant>();
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

      minimumTimestamp = 0;
      maximumTimestamp = 0;
      attributes.clear();
      metadata.clear();

      vx::handleDBusCallOnBackgroundThread(
          &accessor, [self = accessor.thisShared(), streamID, options]() {
            qlonglong minimumTimestampResult;
            qlonglong maximumTimestampResult;
            QList<std::tuple<QString, std::tuple<QString, quint32, QString>,
                             QString, QMap<QString, QDBusVariant>,
                             QMap<QString, QDBusVariant>>>
                attributesResult;
            QMap<QString, QDBusVariant> metadataResult;
            qulonglong eventCountResult = self->getStreamInfo(
                streamID, options, minimumTimestampResult,
                maximumTimestampResult, attributesResult, metadataResult);
            return QList<QDBusVariant>{
                vx::dbusMakeVariant(eventCountResult),
                vx::dbusMakeVariant(minimumTimestampResult),
                vx::dbusMakeVariant(maximumTimestampResult),
                vx::dbusMakeVariant(attributesResult),
                vx::dbusMakeVariant(metadataResult),
            };
          });
    } catch (Exception& e) {
      e.handle(&accessor);
    }

    return 0;
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

      // TODO: This probably should not start another thread if it will just
      // wait for a result from DBus anyway (for EventListDataAccessorDBus)

      vx::handleDBusCallOnBackgroundThread(
          &accessor, [self = accessor.thisShared(), streamID, firstTimestamp,
                      lastTimestamp, targetEventOffset, targetEventCount,
                      output, options]() {
            qlonglong lastReadTimestampResult;
            QString versionStringResult;
            qulonglong readEventCountResult = self->readEvents(
                streamID, firstTimestamp, lastTimestamp, targetEventOffset,
                targetEventCount, output, options, lastReadTimestampResult,
                versionStringResult);
            return QList<QDBusVariant>{
                vx::dbusMakeVariant(readEventCountResult),
                vx::dbusMakeVariant(lastReadTimestampResult),
                vx::dbusMakeVariant(versionStringResult),
            };
          });
    } catch (Exception& e) {
      e.handle(&accessor);
    }

    return 0;
  }

 private:
  EventListDataAccessor& accessor;
};

EventListDataAccessor::EventListDataAccessor() {
  new EventListDataAccessorOperationsImpl(*this);
}

EventListDataAccessor::~EventListDataAccessor() {}

QList<QString> EventListDataAccessor::supportedDBusInterfaces() {
  return {
      "de.uni_stuttgart.Voxie.EventListDataAccessor",
  };
}

EventListDataAccessorDBus::EventListDataAccessorDBus(
    const QSharedPointer<BusConnection>& connection,
    const std::tuple<QString, QDBusObjectPath>& provider)
    : connection_(connection), provider_(provider) {
  auto client =
      createQSharedPointer<ClientWrapper>(connection, std::get<0>(provider));
  wrapper =
      createQSharedPointer<ObjectWrapper>(client, std::get<1>(provider), true);
  proxy = makeSharedQObject<
      de::uni_stuttgart::Voxie::EventListDataAccessorOperations>(
      std::get<0>(provider), std::get<1>(provider).path(),
      connection->connection());

  streamCount = proxy->streamCount();
}

EventListDataAccessorDBus::~EventListDataAccessorDBus() {}

qulonglong EventListDataAccessorDBus::getStreamCount() const {
  return streamCount;
}

QMap<QString, QDBusVariant> EventListDataAccessorDBus::getMetadata(
    const QMap<QString, QDBusVariant>& options) const {
  vx::checkNotOnMainThread("EventListDataAccessorDBus::getMetadata");

  return HANDLEDBUSPENDINGREPLY(proxy->GetMetadata(options));
}

qulonglong EventListDataAccessorDBus::getStreamInfo(
    qulonglong streamID, const QMap<QString, QDBusVariant>& options,
    qlonglong& minimumTimestamp, qlonglong& maximumTimestamp,
    QList<std::tuple<QString, std::tuple<QString, quint32, QString>, QString,
                     QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>&
        attributes,
    QMap<QString, QDBusVariant>& metadata) {
  vx::checkNotOnMainThread("EventListDataAccessorDBus::getStreamInfo");

  return HANDLEDBUSPENDINGREPLY(
      proxy->GetStreamInfo(streamID, options, minimumTimestamp,
                           maximumTimestamp, attributes, metadata));
}

qulonglong EventListDataAccessorDBus::readEvents(
    qulonglong streamID, qlonglong firstTimestamp, qlonglong lastTimestamp,
    qulonglong targetEventOffset, qulonglong targetEventCount,
    std::tuple<QString, QDBusObjectPath> output,
    const QMap<QString, QDBusVariant>& options, qlonglong& lastReadTimestamp,
    QString& versionString) {
  vx::checkNotOnMainThread("EventListDataAccessorDBus::readEvents");

  // TODO: Do this in a better way. Use a separate accessor for long-running
  // operations?
  proxy->setTimeout(INT_MAX);

  return HANDLEDBUSPENDINGREPLY(proxy->ReadEvents(
      streamID, firstTimestamp, lastTimestamp, targetEventOffset,
      targetEventCount, output, options, lastReadTimestamp, versionString));
}

QList<QSharedPointer<SharedMemory>>
EventListDataAccessorDBus::getSharedMemorySections() {
  return {};
}
