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
#pragma once

#include <VoxieBackend/Data/Data.hpp>

#include <VoxieClient/DBusProxies.hpp>
#include <VoxieClient/DBusTypeList.hpp>

namespace vx {
class ImageDataPixel;

class ClientWrapper;
class ObjectWrapper;

class BusConnection;

class VOXIEBACKEND_EXPORT EventListDataAccessor : public Data {
  Q_OBJECT
  VX_REFCOUNTEDOBJECT

 public:
  EventListDataAccessor();
  virtual ~EventListDataAccessor();

  QList<QString> supportedDBusInterfaces() override;

 public:
  virtual qulonglong getStreamCount() const = 0;

  // Note: This method must not be called on the main thread (might cause
  // deadlocks)
  virtual QMap<QString, QDBusVariant> getMetadata(
      const QMap<QString, QDBusVariant>& options) const = 0;

  // Note: This method must not be called on the main thread (might cause
  // deadlocks)
  virtual qulonglong getStreamInfo(
      qulonglong streamID, const QMap<QString, QDBusVariant>& options,
      qlonglong& minimumTimestamp, qlonglong& maximumTimestamp,
      QList<std::tuple<QString, std::tuple<QString, quint32, QString>, QString,
                       QMap<QString, QDBusVariant>,
                       QMap<QString, QDBusVariant>>>& attributes,
      QMap<QString, QDBusVariant>& metadata) = 0;

  // Note: This method must not be called on the main thread (might cause
  // deadlocks)
  virtual qulonglong readEvents(qulonglong streamID, qlonglong firstTimestamp,
                                qlonglong lastTimestamp,
                                qulonglong targetEventOffset,
                                qulonglong targetEventCount,
                                std::tuple<QString, QDBusObjectPath> output,
                                const QMap<QString, QDBusVariant>& options,
                                qlonglong& lastReadTimestamp,
                                QString& versionString) = 0;
};

class VOXIEBACKEND_EXPORT EventListDataAccessorDBus
    : public EventListDataAccessor {
  Q_OBJECT
  VX_REFCOUNTEDOBJECT

  QSharedPointer<BusConnection> connection_;
  std::tuple<QString, QDBusObjectPath> provider_;
  QSharedPointer<de::uni_stuttgart::Voxie::EventListDataAccessorOperations>
      proxy;
  QSharedPointer<ObjectWrapper> wrapper;

  EventListDataAccessorDBus(
      const QSharedPointer<BusConnection>& connection,
      const std::tuple<QString, QDBusObjectPath>& provider);
  virtual ~EventListDataAccessorDBus();

  // Allow the ctor to be called using create()
  friend class vx::RefCountedObject;

 public:
  virtual qulonglong getStreamCount() const override;

  virtual QMap<QString, QDBusVariant> getMetadata(
      const QMap<QString, QDBusVariant>& options) const override;

  virtual qulonglong getStreamInfo(
      qulonglong streamID, const QMap<QString, QDBusVariant>& options,
      qlonglong& minimumTimestamp, qlonglong& maximumTimestamp,
      QList<std::tuple<QString, std::tuple<QString, quint32, QString>, QString,
                       QMap<QString, QDBusVariant>,
                       QMap<QString, QDBusVariant>>>& attributes,
      QMap<QString, QDBusVariant>& metadata) override;

  virtual qulonglong readEvents(qulonglong streamID, qlonglong firstTimestamp,
                                qlonglong lastTimestamp,
                                qulonglong targetEventOffset,
                                qulonglong targetEventCount,
                                std::tuple<QString, QDBusObjectPath> output,
                                const QMap<QString, QDBusVariant>& options,
                                qlonglong& lastReadTimestamp,
                                QString& versionString) override;

  QList<QSharedPointer<SharedMemory>> getSharedMemorySections() override;

 private:
  qulonglong streamCount = 0;
};

}  // namespace vx

Q_DECLARE_METATYPE(vx::EventListDataAccessor*)
Q_DECLARE_METATYPE(vx::EventListDataAccessorDBus*)
