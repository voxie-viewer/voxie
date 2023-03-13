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

// QDBusConnection should be included as early as possible:
// https://bugreports.qt.io/browse/QTBUG-48351 /
// https://bugreports.qt.io/browse/QTBUG-48377
#include <QtDBus/QDBusConnection>

#include "ExternalOperation.hpp"

#include <VoxieClient/DBusAdaptors.hpp>
#include <VoxieClient/Exception.hpp>
#include <VoxieClient/VoxieDBus.hpp>

#include <VoxieBackend/Component/Extension.hpp>

#include <VoxieBackend/IO/Operation.hpp>
#include <VoxieBackend/IO/OperationImport.hpp>

#include <VoxieClient/ObjectExport/Client.hpp>

#include <QtCore/QDebug>

using namespace vx;
using namespace vx;
using namespace vx::io;

namespace vx {

class ExternalOperationAdaptorImpl : public ExternalOperationAdaptor {
  ExternalOperation* object;

 public:
  ExternalOperationAdaptorImpl(ExternalOperation* object);
  ~ExternalOperationAdaptorImpl() override;

  bool isCancelled() const override;

  // Must be called before doing anything else with the object
  // After the operation is finished, client.DecRefCount(operation) has to
  // be called.
  void ClaimOperation(const QDBusObjectPath& client,
                      const QMap<QString, QDBusVariant>& options) override;

  // Set the progress of the operation.
  // (progress should be between 0.0 and 1.0)
  void SetProgress(double progress,
                   const QMap<QString, QDBusVariant>& options) override;

  void FinishError(const QString& name, const QString& message,
                   const QMap<QString, QDBusVariant>& options) override;

  QString action() const override { return object->action(); }

  QString name() const override { return object->name(); }
};

ExternalOperation::ExternalOperation(
    const QSharedPointer<vx::io::Operation>& operation)
    : RefCountedObject("ExternalOperation"), operation_(operation) {
  auto adapter = new ExternalOperationAdaptorImpl(this);

  if (!operation)
    qCritical() << "ExternalOperation::ExternalOperation: operation is null";

  connect(operation.data(), &Operation::cancelled, adapter,
          [adapter]() { adapter->Cancelled(vx::emptyOptions()); });

  // Note: This cannot use this->initialProcessStatus() because the process is
  // not set yet.
  connect(
      this, &QObject::destroyed, this,
      [operation = this->operation(),
       initialProcessStatusContainer =
           this->initialProcessStatusContainer()]() {
        if (!operation->isFinished()) {
          // Script called ClaimOperation() but neither Finish() nor
          // FinishError()

          auto initialProcessStatus = *initialProcessStatusContainer;
          QString additionalError;
          if (!initialProcessStatus) {
            // No process information
            additionalError = "";
          } else if (!initialProcessStatus->isStarted()) {
            // Should not happen, because there should be no one to claim the
            // operation in this case.
            additionalError = " Initial process failed to start (?).";
          } else if (!initialProcessStatus->isExited()) {
            additionalError = " Initial process still running.";
          } else {
            additionalError =
                QString(
                    " Initial process exited with exit status %1 / exit code "
                    "%2.")
                    .arg(exitStatusToString(initialProcessStatus->exitStatus()))
                    .arg(initialProcessStatus->exitCode());
          }

          operation->finish(
              createQSharedPointer<vx::io::Operation::ResultError>(
                  createQSharedPointer<Exception>(
                      "de.uni_stuttgart.Voxie.Error",
                      "Script failed to return any data." + additionalError)));
        }
      });
}

ExternalOperation::~ExternalOperation() {
  // qDebug() << "ExternalOperation::~ExternalOperation";
}

ExternalOperationAdaptorImpl::ExternalOperationAdaptorImpl(
    ExternalOperation* object)
    : ExternalOperationAdaptor(object), object(object) {}
ExternalOperationAdaptorImpl::~ExternalOperationAdaptorImpl() {}

void ExternalOperation::setInitialProcess(QProcess* process) {
  vx::checkOnMainThread("ExternalOperation::setInitialProcess()");

  if (!process) {
    qWarning() << "ExternalOperation::setInitialProcess() called with nullptr";
    return;
  }

  if (this->initialProcessSet) {
    qWarning()
        << "ExternalOperation::setInitialProcess() called multiple times";
    return;
  }

  this->initialProcessSet = true;
  this->initialProcess_ = process;

  *this->initialProcessStatusContainer_ =
      makeSharedQObject<ProcessStatus>(process);
}

void ExternalOperation::checkClient() {
  if (!client)
    throw Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                    "ExternalOperation not yet claimed");
  if (client->uniqueConnectionName() == "") return;
  if (!calledFromDBus()) {
    qWarning() << "ExternalOperationAdaptor::checkClient() called from "
                  "non-DBus context:"
               << this;
    return;
  }
  if (client->uniqueConnectionName() != message().service()) {
    qWarning() << "ExternalOperationAdaptor::checkClient(): connection "
                  "mismatch, expected"
               << client->uniqueConnectionName() << "got" << message().service()
               << "for" << this;
  }
}

void ExternalOperation::cleanup() {}

bool ExternalOperation::isClaimed() {
  QSharedPointer<QSharedPointer<ExternalOperation>> ref(initialReference);
  if (!ref)
    // TODO: What should be done here?
    throw Exception("de.uni_stuttgart.Voxie.Error", "initialReference not set");
  return !*ref;
}

bool ExternalOperationAdaptorImpl::isCancelled() const {
  return object->operation()->isCancelled();
}

void ExternalOperationAdaptorImpl::ClaimOperation(
    const QDBusObjectPath& client, const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);

    // qDebug() << "ExternalOperation::ClaimOperation" << client.path();

    Client* clientPtr =
        qobject_cast<Client*>(vx::ExportedObject::lookupWeakObject(client));
    if (!clientPtr) {
      throw Exception("de.uni_stuttgart.Voxie.ObjectNotFound",
                      "Cannot find client object");
    }

    QSharedPointer<QSharedPointer<ExternalOperation>> ref(
        object->initialReference);
    if (!ref)
      throw Exception("de.uni_stuttgart.Voxie.Error",
                      "initialReference not set");
    if (!*ref)
      throw Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                      "ExternalOperation already claimed");
    auto ptr = *ref;
    if (ptr.data() != object)
      throw Exception("de.uni_stuttgart.Voxie.Error",
                      "initialReference set to invalid value");
    if (object->client)
      throw Exception("de.uni_stuttgart.Voxie.Error",
                      "object->client already set");
    object->client = clientPtr;

    ref->reset();

    Q_EMIT object->claimed();

    clientPtr->incRefCount(ptr);
    return;
  } catch (vx::Exception& e) {
    e.handle(object);
    return;
  }
}

void ExternalOperationAdaptorImpl::SetProgress(
    double progress, const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);

    object->checkClient();

    // qDebug() << "ExternalOperation::SetProgress" << progress;
    object->operation()->updateProgress(progress);
  } catch (vx::Exception& e) {
    e.handle(object);
    return;
  }
}

void ExternalOperationAdaptorImpl::FinishError(
    const QString& name, const QString& message,
    const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);

    object->checkClient();

    // qDebug() << "ExternalOperation::FinishError" << name << message;
    if (object->isFinished)
      throw Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                      "ExternalOperation is already finished");

    Q_EMIT object->error(Exception(name, message));
    object->isFinished = true;
    object->cleanup();
  } catch (vx::Exception& e) {
    e.handle(object);
    return;
  }
}

class ExternalOperationImportAdaptorImpl
    : public ExternalOperationImportAdaptor {
  ExternalOperationImport* object;

 public:
  ExternalOperationImportAdaptorImpl(ExternalOperationImport* object);
  ~ExternalOperationImportAdaptorImpl() override;

  void Finish(const QDBusObjectPath& data, const QDBusObjectPath& version,
              const QMap<QString, QDBusVariant>& options) override;

  QString filename() const override { return object->filename(); }

  QMap<QString, QDBusVariant> properties() const override {
    return object->properties();
  }
};

ExternalOperationImport::ExternalOperationImport(
    const QSharedPointer<vx::io::Operation>& operation, const QString& filename,
    const QMap<QString, QDBusVariant>& properties, const QString& name)
    : ExternalOperation(operation),
      filename_(filename),
      properties_(properties),
      name_(name) {
  new ExternalOperationImportAdaptorImpl(this);

  // TODO: Merge this code into the
  // FinishError() method
  connect(this, &ExternalOperation::error, this, [this](const Exception& err) {
    this->operation()->finish(createQSharedPointer<Operation::ResultError>(
        createQSharedPointer<Exception>(err)));
  });
}

ExternalOperationImport::~ExternalOperationImport() {}

QString ExternalOperationImport::action() { return "Import"; }

QString ExternalOperationImport::name() { return name_; }

ExternalOperationImportAdaptorImpl::ExternalOperationImportAdaptorImpl(
    ExternalOperationImport* object)
    : ExternalOperationImportAdaptor(object), object(object) {}
ExternalOperationImportAdaptorImpl::~ExternalOperationImportAdaptorImpl() {}

void ExternalOperationImportAdaptorImpl::Finish(
    const QDBusObjectPath& data, const QDBusObjectPath& version,
    const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);

    object->checkClient();

    // qDebug() << "ExternalOperation::Finish" << data.path();

    auto dataStorage = Data::lookup(data);
    auto versionObj = DataVersion::lookup(version);

    if (object->isFinished)
      throw Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                      "ExternalOperationImport is already finished");

    if (versionObj->data() != dataStorage)
      throw vx::Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                          "Given DataVersion is for another object");

    object->operation()->finish(
        createQSharedPointer<OperationImport::Result>(dataStorage, versionObj));

    object->isFinished = true;
    object->cleanup();
  } catch (vx::Exception& e) {
    e.handle(object);
    return;
  }
}

class ExternalOperationExportAdaptorImpl
    : public ExternalOperationExportAdaptor {
  ExternalOperationExport* object;

 public:
  ExternalOperationExportAdaptorImpl(ExternalOperationExport* object)
      : ExternalOperationExportAdaptor(object), object(object) {}
  ~ExternalOperationExportAdaptorImpl() {}

  void Finish(const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      object->checkClient();

      // qDebug() << "ExternalOperation::Finish" << data.path();

      if (object->operation()->isFinished())
        throw Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                        "ExternalOperationExport is already finished");

      object->operation()->finish(
          createQSharedPointer<Operation::ResultSuccess>());

      object->isFinished = true;
      object->cleanup();
    } catch (vx::Exception& e) {
      e.handle(object);
      return;
    }
  }

  QDBusObjectPath data() const override {
    return ExportedObject::getPath(object->data());
  }

  QString filename() const override { return object->filename(); }
};

ExternalOperationExport::ExternalOperationExport(
    const QSharedPointer<vx::io::Operation>& operation, const QString& filename,
    const QString& name, const QSharedPointer<vx::Data>& data)
    : ExternalOperation(operation),
      filename_(filename),
      name_(name),
      data_(data) {
  new ExternalOperationExportAdaptorImpl(this);

  // TODO: Change ExternalOperationImport and merge this code into the
  // FinishError() method
  connect(this, &ExternalOperation::error, this, [this](const Exception& err) {
    this->operation()->finish(createQSharedPointer<Operation::ResultError>(
        createQSharedPointer<Exception>(err)));
  });
}
ExternalOperationExport::~ExternalOperationExport() {}

QString ExternalOperationExport::action() { return "Export"; }

QString ExternalOperationExport::name() { return name_; }

}  // namespace vx
