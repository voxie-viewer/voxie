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

#include <VoxieBackend/VoxieBackend.hpp>

#include <VoxieBackend/Data/DataChangedReason.hpp>

#include <VoxieBackend/DBus/DynamicObject.hpp>

#include <QtCore/QJsonObject>
#include <QtCore/QMutex>

namespace vx {
class Data;
class DataUpdate;
class DataVersion;
class DataContainer;
class SharedMemory;
template <typename T>
class SharedFunPtr;
namespace io {
class Exporter;
}

/**
 * @brief The Data class serves as a common parent class to all data
 * storing classes of DataObjects (Surface, VolumeDataVoxel(-Inst), ...) it's
 * main purpose is to serve as a common class to return for the importer plugins
 */
class VOXIEBACKEND_EXPORT Data : public vx::DynamicObject {
  Q_OBJECT
  REFCOUNTEDOBJ_DECL(Data)

  friend class DataUpdate;
  friend class DataContainer;

  struct ContainerInfo {
    ContainerInfo() {}
    ContainerInfo(quint64 refCount, const QMetaObject::Connection& connection)
        : refCount(refCount), connection(connection) {}
    quint64 refCount;
    QMetaObject::Connection connection;
  };

  QMutex lock;
  // All these variables can be changed by other threads, accesses have to be
  // protected by locking 'lock'
  // updates is a map from the object path of the update to a weak pointer to
  // the update.
  QMap<QDBusObjectPath, QWeakPointer<DataUpdate>> updates;
  QSharedPointer<DataVersion> currentVersion_;
  uint64_t lastUpdateId;
  // List of containers containing this Data object
  QMap<QDBusObjectPath, ContainerInfo> containers;

  // Must be called on main thread.
  // Expects 'lock' to be held (by the main thread).
  // finishAction is callback which has to be executed (on the main thread)
  // after the lock has been released.
  QSharedPointer<DataVersion> updateVersionIntern(
      const QMutexLocker& locker, DataChangedReason reason,
      const QJsonObject& metadata, vx::SharedFunPtr<void()>& finishAction);

  // Throws if there is an update running.
  // Called from DataContainer
  void addContainer(ExportedObject* container);
  void removeContainer(const QDBusObjectPath& path);

 protected:
  void initialize() override;

 public:
  Data();

  QSharedPointer<DataVersion> currentVersion();

  QSharedPointer<DataUpdate> createUpdate(
      const QMap<QDBusObjectPath, QSharedPointer<DataUpdate>>&
          containerUpdates = {});

  // TODO: Should this (and the SharedMemory object) be exposed over DBus?
  /**
   * @brief Get the shared memory sections used by the data object
   *
   * Currently it is assumed that this list never changes over the lifetime of
   * the Data object.
   *
   * This is used to display the amount of memory used.
   */
  virtual QList<QSharedPointer<SharedMemory>> getSharedMemorySections() = 0;

 Q_SIGNALS:
  /**
   * @param newVersion The new data version
   * @param reason The reason for the update
   * was created
   */
  void dataChanged(const QSharedPointer<DataVersion>& newVersion,
                   DataChangedReason reason);
};

// This is currently implementing the DBus interface ExternalDataUpdate. TODO:
// Rename DataUpdate class to ExternalDataUpdate or split this into two classes,
// so the internal updates are not externally visible? (A separate DataUpdate
// class for both kinds of updates might be created, but this currently would
// only contain the Data member and not really be useful).
class VOXIEBACKEND_EXPORT DataUpdate : public vx::RefCountedObject {
  Q_OBJECT
  REFCOUNTEDOBJ_DECL(DataUpdate)

  struct FinishPreventer {};
  struct UpdateInfo {
    // Note: the FinishPreventer must be second so that it is destroyed before
    // the update
    QSharedPointer<DataUpdate> update;
    QSharedPointer<FinishPreventer> finishPreventer;
  };

  // Uses data()->lock for locking
  QSharedPointer<Data> data_;
  bool running_;
  QMap<QDBusObjectPath, QSharedPointer<DataUpdate>> origContainerUpdates_;
  QMap<QDBusObjectPath, UpdateInfo> containerUpdates_;
  // As long as this is not nullptr, Finish() cannot be called
  QWeakPointer<FinishPreventer> finishPreventer_;

  QSharedPointer<FinishPreventer> createFinishPreventer();

  DataUpdate(const QSharedPointer<Data>& data,
             const QMap<QDBusObjectPath, QSharedPointer<DataUpdate>>&
                 containerUpdates);
  ~DataUpdate();

  // Allow the ctor to be called using create()
  friend class vx::RefCountedObject;

 protected:
  void initialize() override;

 public:
  QSharedPointer<Data> data() { return data_; }

  QSharedPointer<DataVersion> finish(const QJsonObject& metadata);

  bool running();
};

class VOXIEBACKEND_EXPORT DataVersion : public vx::RefCountedObject {
  Q_OBJECT
  REFCOUNTEDOBJ_DECL(DataVersion)

  friend class Data;

  QWeakPointer<Data> data_;
  QString versionString_;
  bool updateIsRunning_;
  QString date_;
  QJsonObject metadata_;

  DataVersion(const QSharedPointer<Data>& data, const QString& versionString,
              bool updateIsRunning, const QString& date,
              const QJsonObject& metadata);

  // Allow the ctor to be called using create()
  friend class vx::RefCountedObject;

 public:
  // Can return nullptr if the data object was already destroyed
  QSharedPointer<Data> data() { return dataWeak().lock(); }

  const QWeakPointer<Data> dataWeak() { return data_; }
  const QString& versionString() { return versionString_; }
  bool updateIsRunning() { return updateIsRunning_; }
  const QString& date() { return date_; }
  const QJsonObject& metadata() { return metadata_; }
};

/**
 * @brief A class which can contain Data objects and prevents the contained data
 * from being changed without the knowledge of the container.
 * This class is intended to be used with multiple inheritance, any class
 * inheriting from DataContainer should also inherit from ExportedObject.
 */
// TODO: This probably also should be used by DataObject
class VOXIEBACKEND_EXPORT DataContainer {
  vx::ExportedObject* self;

 protected:
  DataContainer(vx::ExportedObject* self);
  virtual ~DataContainer();

  void addChildData(const QSharedPointer<Data>& data);
  void removeChildData(const QSharedPointer<Data>& data);
};
}  // namespace vx

Q_DECLARE_METATYPE(vx::Data*)
Q_DECLARE_METATYPE(QSharedPointer<vx::Data>)
