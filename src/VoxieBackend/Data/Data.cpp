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

#include "Data.hpp"

#include <VoxieClient/SharedFunPtr.hpp>

#include <VoxieClient/ObjectExport/Client.hpp>

#include <VoxieBackend/Data/DataProperty.hpp>
#include <VoxieBackend/Data/ReplaceMode.hpp>

#include <VoxieBackend/Property/PropertyType.hpp>

#include <VoxieBackend/IO/Exporter.hpp>

#include <VoxieClient/DBusAdaptors.hpp>
#include <VoxieClient/JsonDBus.hpp>

#include <QtCore/QMutexLocker>

using namespace vx;

static bool verboseContainers = false;

namespace vx {
namespace {
class DataAdaptorImpl : public DataAdaptor {
  Data* object;

 public:
  DataAdaptorImpl(Data* object) : DataAdaptor(object), object(object) {}
  ~DataAdaptorImpl() override {}

  QString currentVersionString() const override {
    auto currentVersion = object->currentVersion();
    if (!currentVersion) {
      qCritical() << "object->currentVersion() is nullptr";
      return "";
    }
    return currentVersion->versionString();
  }

  QDBusObjectPath CreateUpdate(
      const QDBusObjectPath& client,
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options, "ContainerUpdates");

      auto clientPtr =
          qobject_cast<Client*>(ExportedObject::lookupWeakObject(client));
      if (!clientPtr) {
        throw Exception("de.uni_stuttgart.Voxie.ObjectNotFound",
                        "Cannot find client object");
      }

      QMap<QDBusObjectPath, QSharedPointer<DataUpdate>> containerUpdates;
      auto containerUpdatesDBus = ExportedObject::getOptionValueOrDefault<
          QMap<QDBusObjectPath, QDBusObjectPath>>(options, "ContainerUpdates",
                                                  {});
      for (const auto& key : containerUpdatesDBus.keys())
        containerUpdates[key] = DataUpdate::lookup(containerUpdatesDBus[key]);

      auto update = object->createUpdate(containerUpdates);
      clientPtr->incRefCount(update);
      return ExportedObject::getPath(update);
    } catch (Exception& e) {
      e.handle(object);
      return ExportedObject::getPath(nullptr);
    }
  }

  QDBusObjectPath GetCurrentVersion(
      const QDBusObjectPath& client,
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);
      auto clientPtr =
          qobject_cast<Client*>(ExportedObject::lookupWeakObject(client));
      if (!clientPtr) {
        throw Exception("de.uni_stuttgart.Voxie.ObjectNotFound",
                        "Cannot find client object");
      }

      auto version = object->currentVersion();
      clientPtr->incRefCount(version);
      return ExportedObject::getPath(version);
    } catch (Exception& e) {
      e.handle(object);
      return ExportedObject::getPath(nullptr);
    }
  }

  QList<QDBusObjectPath> ListProperties(
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      QList<QDBusObjectPath> res;
      for (const auto& property : object->listProperties())
        res << ExportedObject::getPath(property);

      return res;
    } catch (Exception& e) {
      return e.handle(object);
    }
  }

  QDBusVariant GetProperty(
      const QDBusObjectPath& property,
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options, "AllowMissing");

      auto propertyObj = DataProperty::lookup(property);

      auto allowMissing = ExportedObject::getOptionValueOrDefault<bool>(
          options, "AllowMissing", false);

      auto valueRaw = object->getProperty(propertyObj, allowMissing);

      if (valueRaw.isValid()) {
        return propertyObj->type()->rawToDBus(valueRaw);
      } else {
        // https://bugreports.qt.io/browse/QTBUG-124919
        // return dbusMakeVariant<QDBusSignature>(QDBusSignature(""));
        return dbusMakeVariant<QDBusSignature>(QDBusSignature("n"));
      }
    } catch (Exception& e) {
      return e.handle(object);
    }
  }

  void SetProperty(const QDBusObjectPath& update,
                   const QDBusObjectPath& property, const QDBusVariant& value,
                   const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options, "ReplaceMode");

      auto updateObj = DataUpdate::lookup(update);
      auto propertyObj = DataProperty::lookup(property);

      auto replaceModeStr = ExportedObject::getOptionValueOrDefault<QString>(
          options, "ReplaceMode", "de.uni_stuttgart.Voxie.ReplaceMode.Insert");
      auto replaceMode = parseReplaceMode(replaceModeStr);

      QVariant valueRaw;
      if (dbusGetVariantSignature(value) != QDBusSignature("g") ||
          (dbusGetVariantValue<QDBusSignature>(value).signature() != "" &&
           dbusGetVariantValue<QDBusSignature>(value).signature() != "n")) {
        valueRaw = propertyObj->type()->dbusToRaw(value);
      } else {
        valueRaw = QVariant();  // Use invalid QVariant to remove value
      }

      object->setProperty(updateObj, propertyObj, valueRaw, replaceMode);
    } catch (Exception& e) {
      e.handle(object);
    }
  }
};
}  // namespace
}  // namespace vx

static QString reasonToString(DataChangedReason reason) {
  switch (reason) {
    case DataChangedReason::Initialized:
      return "de.uni_stuttgart.Voxie.Data.DataChangedReason.Initialized";
    case DataChangedReason::NewUpdate:
      return "de.uni_stuttgart.Voxie.Data.DataChangedReason.NewUpdate";
    case DataChangedReason::UpdateFinished:
      return "de.uni_stuttgart.Voxie.Data.DataChangedReason.UpdateFinished";

      // Note: DataChangedReason::DataInstanceChanged should never happen here

    default:
      throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                          "Invalid DataChangedReason value");
  }
}

Data::Data() : vx::DynamicObject("Data") {
  lastUpdateId = 0;
  auto adaptor = new DataAdaptorImpl(this);
  QObject::connect(
      this, &Data::dataChanged, adaptor,
      [adaptor](const QSharedPointer<DataVersion>& newVersion,
                DataChangedReason reason) {
        try {
          Q_EMIT adaptor->DataChanged(
              vx::ExportedObject::getPath(newVersion),
              newVersion->versionString(), newVersion->updateIsRunning(),
              newVersion->date(), reasonToString(reason),
              QMap<QString, QDBusVariant>());
        } catch (Exception& e) {
          qWarning() << "Exception while emitting DataChanged event:"
                     << e.message();
        }
      });
}

void Data::initialize() {
  DynamicObject::initialize();

  executeOnThread(this, [this] {
    vx::SharedFunPtr<void()> finishAction;
    {
      QMutexLocker locker(&this->lock);
      updateVersionIntern(locker, DataChangedReason::Initialized, QJsonObject(),
                          finishAction);
    }
    if (finishAction) finishAction();
  });
}

QSharedPointer<DataVersion> Data::currentVersion() {
  QMutexLocker locker(&this->lock);
  return currentVersion_;
}

QSharedPointer<DataVersion> Data::updateVersionIntern(
    const QMutexLocker& locker, DataChangedReason reason,
    const QJsonObject& metadata, vx::SharedFunPtr<void()>& finishAction) {
  // Must be on main thread so that DataVersion::create() can be called without
  // deadlock
  vx::checkOnMainThread("Data::updateVersionIntern");

  if (locker.mutex() != &this->lock) {
    qCritical() << "Data::updateVersionIntern() called with wrong mutex";
    return QSharedPointer<DataVersion>();
  }
  lastUpdateId++;
  bool haveUpdates = updates.count() > 0;
  auto versionStr = QString::number(lastUpdateId) + (haveUpdates ? "+" : "");
  auto date = QDateTime::currentDateTimeUtc().toString(Qt::ISODate /*WithMs*/);
  auto currentVersionObj = DataVersion::create(thisShared(), versionStr,
                                               haveUpdates, date, metadata);
  currentVersion_ = currentVersionObj;

  /*
  // Enqueue for execution on main thread. This is done so that the handlers
  // will always be executed after the lock has been released and that the
  // handlers are still called in order. Note that this means that handlers
  // might be called slightly later, e.g. the dbus signal will be emitted after
  // the call return for ExternalDataUpdate.Finish() has been sent.
  enqueueOnThread(this, [this, currentVersionObj, reason] {
    Q_EMIT this->dataChanged(currentVersionObj, reason);
  });
  */

  // updateVersionIntern() is called on the main thread, set action to be called
  // immediately after the lock has been released.
  finishAction = [this, currentVersionObj, reason] {
    Q_EMIT this->dataChanged(currentVersionObj, reason);
  };

  return currentVersionObj;
}

QSharedPointer<DataUpdate> Data::createUpdate(
    const QMap<QDBusObjectPath, QSharedPointer<DataUpdate>>& containerUpdates) {
  return DataUpdate::create(thisShared(), containerUpdates);
}

void Data::addContainer(ExportedObject* container) {
  auto path = container->getPath();

  QMutexLocker locker1(&this->lock);

  bool haveUpdates = updates.count() > 0;
  if (haveUpdates)
    throw Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                    "Attempting to add a Data object to a data container while "
                    "it has updates running");

  if (!containers.contains(path)) {
    QWeakPointer<Data> weakSelf = this->thisShared();
    auto connection = QObject::connect(
        container, &QObject::destroyed,
        // Note: Don't pass this in as receiver because the
        // signal should not be queued. Use a weak reference instead.
        [weakSelf, path]() {
          auto self = weakSelf.lock();
          if (!self) {
            // Nothing to do, Data is already destroyed or being destroyed
            return;
          }

          QMutexLocker locker2(&self->lock);
          if (!self->containers.contains(path)) {
            qWarning() << "Attempting to remove" << self << "from container"
                       << path.path()
                       << "on container destruction but the container does not "
                          "contain the data";
            return;
          }

          self->containers.remove(path);
        });
    containers.insert(path, ContainerInfo(1, connection));
  } else {
    auto& info = containers[path];
    quint64 old = info.refCount;
    quint64 newVal = old + 1;
    if (newVal < old)
      throw Exception("de.uni_stuttgart.Voxie.Overflow",
                      "Reference counter overflow");
    info.refCount = newVal;
  }
}

void Data::removeContainer(const QDBusObjectPath& path) {
  QMutexLocker locker(&this->lock);

  if (!containers.contains(path)) {
    qCritical() << "Attempting to remove" << this << "from container"
                << path.path() << "but the container does not contain the data";
    return;
  } else {
    auto& info = containers[path];
    if (info.refCount == 1) {
      QObject::disconnect(info.connection);
      containers.remove(path);
    } else if (info.refCount == 0) {
      qCritical() << "Got 0 in containers map";
    } else {
      info.refCount = info.refCount - 1;
    }
  }
}

QList<QSharedPointer<DataProperty>> Data::listProperties() {
  vx::checkOnMainThread("Data::listProperties");

  QList<QSharedPointer<DataProperty>> res;
  for (const auto& entry : this->dataProperties.values()) res << entry.property;
  return res;
}

QVariant Data::getProperty(const QSharedPointer<DataProperty>& property,
                           bool allowMissing) {
  vx::checkOnMainThread("Data::getProperty");

  QString key = property->name();

  if (!this->dataProperties.contains(key)) {
    if (allowMissing) return QVariant();
    throw vx::Exception("de.uni_stuttgart.Voxie.KeyNotFound",
                        "Failed to look up data property");
  }
  const auto& val = this->dataProperties[key];

  if (!val.value.isValid())
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "Got invalid data property value");

  if (val.property != property) {
    // TODO: Consider similar data property objects the same to some extend?
    throw vx::Exception("de.uni_stuttgart.Voxie.DifferentPropertyDefinition",
                        "Data property has different definition");
  }

  return val.value;
}

void Data::setProperty(const QSharedPointer<DataUpdate>& update,
                       const QSharedPointer<DataProperty>& property,
                       const QVariant& value, ReplaceMode replaceMode) {
  vx::checkOnMainThread("Data::setProperty");

  update->validateCanUpdate(this);

  // TODO: Consider similar data property objects the same to some extend?

  QString key = property->name();

  bool haveNewData = value.isValid();

  // Check data
  if (haveNewData) property->type()->verifyValueWithoutProperty(value);

  bool contains = this->dataProperties.contains(key);

  switch (replaceMode) {
    case ReplaceMode::Insert: {
      if (contains)
        throw vx::Exception("de.uni_stuttgart.Voxie.DuplicateKey",
                            "Attempting to insert duplicate data property");
      break;
    }
    case ReplaceMode::InsertOrReplace: {
      break;
    }
    case ReplaceMode::ReplaceExisting: {
      if (!contains)
        throw vx::Exception("de.uni_stuttgart.Voxie.KeyNotFound",
                            "Failed to look up key to replace");
      break;
    }
    case ReplaceMode::InsertOrSame: {
      if (contains) {
        const auto& oldValue = this->dataProperties[key];
        if (oldValue.property != property || oldValue.value != value)
          throw vx::Exception("de.uni_stuttgart.Voxie.DuplicateKey",
                              "Attempting to insert duplicate data property "
                              "with different definition or data");
        // No need to continue if data is the same
        return;
      }
      break;
    }
  }

  if (haveNewData) {
    DataPropertyValue val;
    val.property = property;
    val.value = value;
    this->dataProperties[key] = val;
  } else {
    this->dataProperties.remove(key);
  }
}

namespace vx {
namespace {
class DataVersionAdaptorImpl : public DataVersionAdaptor {
  DataVersion* object;

 public:
  DataVersionAdaptorImpl(DataVersion* object)
      : DataVersionAdaptor(object), object(object) {}
  ~DataVersionAdaptorImpl() override {}

  QDBusObjectPath data() const override {
    return vx::ExportedObject::getPath(object->data());
  }
  QString date() const override { return object->date(); }
  bool updateIsRunning() const override { return object->updateIsRunning(); }
  QString versionString() const override { return object->versionString(); }
  QMap<QString, QDBusVariant> metadata() const override {
    return vx::jsonToDBus(object->metadata());
  }
};
}  // namespace
}  // namespace vx

DataVersion::DataVersion(const QSharedPointer<Data>& data,
                         const QString& versionString, bool updateIsRunning,
                         const QString& date, const QJsonObject& metadata)
    : RefCountedObject("DataVersion"),
      data_(data),
      versionString_(versionString),
      updateIsRunning_(updateIsRunning),
      date_(date),
      metadata_(metadata) {
  new DataVersionAdaptorImpl(this);
}

namespace vx {
namespace {
class ExternalDataUpdateAdaptorImpl : public ExternalDataUpdateAdaptor {
  DataUpdate* object;

 public:
  ExternalDataUpdateAdaptorImpl(DataUpdate* object)
      : ExternalDataUpdateAdaptor(object), object(object) {}
  ~ExternalDataUpdateAdaptorImpl() override {}

  QDBusObjectPath data() const override {
    return vx::ExportedObject::getPath(object->data());
  }

  QDBusObjectPath Finish(const QDBusObjectPath& client,
                         const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options, "Metadata");
      auto clientPtr =
          qobject_cast<Client*>(ExportedObject::lookupWeakObject(client));
      if (!clientPtr) {
        throw Exception("de.uni_stuttgart.Voxie.ObjectNotFound",
                        "Cannot find client object");
      }

      QJsonObject metadata = QJsonObject();
      if (ExportedObject::hasOption(options, "Metadata")) {
        metadata = dbusToJson(
            ExportedObject::getOptionValue<QMap<QString, QDBusVariant>>(
                options, "Metadata"));
      }

      auto version = object->finish(metadata);
      if (!version)
        throw Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                        "Finish() has already been called");

      clientPtr->incRefCount(version);
      return ExportedObject::getPath(version);
    } catch (Exception& e) {
      e.handle(object);
      return ExportedObject::getPath(nullptr);
    }
  }
};
}  // namespace
}  // namespace vx

DataUpdate::DataUpdate(
    const QSharedPointer<Data>& data,
    const QMap<QDBusObjectPath, QSharedPointer<DataUpdate>>& containerUpdates)
    : RefCountedObject("DataUpdate"),
      data_(data),
      running_(false),
      origContainerUpdates_(containerUpdates) {
  new ExternalDataUpdateAdaptorImpl(this);
}
void DataUpdate::initialize() {
  // Because this locks other data objects this must be done before the lock is
  // taken
  for (const auto& container : data()->containers.keys()) {
    if (!this->origContainerUpdates_.contains(container))
      throw vx::Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                          "Data object " + data()->getPath().path() +
                              " is in container" + container.path() +
                              " but there is no update for the container "
                              "when creating an update for the data object");
    auto outerUpdate = this->origContainerUpdates_[container];
    if (!outerUpdate)
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "outerUpdate is nullptr");
    auto outerUpdateData = outerUpdate->data();
    if (!outerUpdateData)
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "outerUpdateData is nullptr");
    if (outerUpdateData->getPath() != container)
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "ContainerUpdate is for wrong data object");

    if (containerUpdates_.contains(container))
      throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                          "containerUpdates_.contains([container])");
    containerUpdates_[container].update = outerUpdate;
    // Note: This will throw if the update is already finished
    containerUpdates_[container].finishPreventer =
        outerUpdate->createFinishPreventer();
  }
  // Remove unused updates
  origContainerUpdates_.clear();

  executeOnThread(this, [this] {
    vx::SharedFunPtr<void()> finishAction;

    {
      QMutexLocker locker(&data()->lock);

      if (verboseContainers)
        qDebug() << "Creating update" << this->getPath().path()
                 << "for data object" << data()->getPath().path() << "which has"
                 << data()->containers.count() << "containers";

      this->running_ = true;
      data()->updates[this->getPath()] = thisShared();
      // Keep current metadata when creating new update
      data()->updateVersionIntern(locker, DataChangedReason::NewUpdate,
                                  data()->currentVersion_->metadata(),
                                  finishAction);
    }

    if (finishAction) finishAction();
  });
}
DataUpdate::~DataUpdate() {
  // TODO: handle this differently, e.g. should
  // DataObject::dataChangedFinished(*) be emitted here?
  auto version = finish(QJsonObject{
      {"Status",
       QJsonObject{
           {"Error",
            QJsonObject{
                {"Name", "de.uni_stuttgart.Voxie.DataUpdateAborted"},
                {"Name", "The update was not finished properly"},
            }},
       }},
  });

  if (version)
    qWarning() << "Update" << this->getPath().path()
               << "is being destroyed without being finished";
}
QSharedPointer<DataUpdate::FinishPreventer>
DataUpdate::createFinishPreventer() {
  QMutexLocker locker(&data()->lock);

  if (!running_)
    throw Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                    "DataUpdate has already finished");

  QSharedPointer<FinishPreventer> current = this->finishPreventer_.lock();
  if (current) return current;

  // Safe because the lock is held and createFinishPreventer() or finish()
  // cannot be running here
  current = createQSharedPointer<FinishPreventer>();
  this->finishPreventer_ = current;
  return current;
}
QSharedPointer<DataVersion> DataUpdate::finish(const QJsonObject& metadata) {
  QSharedPointer<DataVersion> result;
  bool wasRunning = executeOnThread(this, [&] {
    vx::SharedFunPtr<void()> finishAction;

    {
      QMutexLocker locker(&data()->lock);

      if (this->finishPreventer_.lock())
        throw Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                        "Attempting to finish a DataUpdate which is still in "
                        "use elsewhere");

      if (!running_) return false;
      running_ = false;

      // Remove update from list
      if (!data()->updates.contains(this->getPath())) {
        qWarning() << "Unable to remove DataUpdate from list";
      }
      data()->updates.remove(this->getPath());

      result = data()->updateVersionIntern(
          locker, DataChangedReason::UpdateFinished, metadata, finishAction);
    }  // release lock

    if (finishAction) finishAction();

    return true;
  });
  if (!wasRunning)
    return QSharedPointer<DataVersion>();  // return nullptr to indicate
                                           // that no update was done

  // Note: This can cause other updates to be destroyed (and therefore maybe to
  // be finished) and must be done without the lock
  containerUpdates_.clear();

  return result;
}
bool DataUpdate::running() {
  QMutexLocker locker(&this->data()->lock);
  return this->running_;
}

void DataUpdate::validateCanUpdate(Data* data) {
  if (this->data().data() != data)
    throw vx::Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                        "Given DataUpdate is for another object");
  if (!this->running())
    throw vx::Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                        "Given DataUpdate is already finished");
}

DataContainer::DataContainer(vx::ExportedObject* self) : self(self) {}
DataContainer::~DataContainer() {}

void DataContainer::addChildData(const QSharedPointer<Data>& data) {
  if (!data)
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError", "data is null");

  if (verboseContainers)
    qDebug() << "Adding child" << data->getPath().path() << "to container"
             << self->getPath().path();
  data->addContainer(self);
}
void DataContainer::removeChildData(const QSharedPointer<Data>& data) {
  if (!data)
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError", "data is null");

  if (verboseContainers)
    qDebug() << "Removing child" << data->getPath().path() << "from container"
             << self->getPath().path();
  data->removeContainer(self->getPath());
}
