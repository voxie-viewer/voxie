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

#include <VoxieBackend/Data/Data.hpp>

#include <VoxieClient/ObjectExport/Client.hpp>
#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <QtCore/QPointer>

#include <QtDBus/QDBusAbstractAdaptor>
#include <QtDBus/QDBusObjectPath>

namespace vx {
class NodePrototype;
namespace io {
class Operation;
class RunFilterOperation;
}  // namespace io
class Exception;

class ExternalOperationAdaptorImpl;
class VOXIEBACKEND_EXPORT ExternalOperation : public vx::RefCountedObject {
  Q_OBJECT
  REFCOUNTEDOBJ_DECL(ExternalOperation)

  friend class ExternalOperationAdaptorImpl;

  QPointer<vx::Client> client = nullptr;

  QSharedPointer<vx::io::Operation> operation_;

 protected:
  // TODO: Get rid of this / replace it by operation()->isFinished()?
  bool isFinished = false;

  void checkClient();

  virtual void cleanup();

 public:
  explicit ExternalOperation(
      const QSharedPointer<vx::io::Operation>& operation);
  ~ExternalOperation() override;

  QWeakPointer<QSharedPointer<ExternalOperation>> initialReference;

  virtual QString action() = 0;

  virtual QString name() = 0;

  const QSharedPointer<vx::io::Operation>& operation() const {
    return operation_;
  }

  bool isClaimed();

 Q_SIGNALS:
  void error(const vx::Exception& error);

  // Emitted when the operation is claimed
  void claimed();
};

// TODO: Should probably be moved to ExtensionImporter / ExtensionExporter

class ExternalOperationImportAdaptorImpl;
class VOXIEBACKEND_EXPORT ExternalOperationImport : public ExternalOperation {
  Q_OBJECT
  REFCOUNTEDOBJ_DECL(ExternalOperationImport)

  friend class ExternalOperationImportAdaptorImpl;

  QString filename_;
  QMap<QString, QDBusVariant> properties_;
  QString name_;

 public:
  explicit ExternalOperationImport(
      const QSharedPointer<vx::io::Operation>& operation,
      const QString& filename, const QMap<QString, QDBusVariant>& properties,
      const QString& name);
  ~ExternalOperationImport() override;

  QString action() override;

  QString name() override;

  const QString& filename() { return filename_; }
  const QMap<QString, QDBusVariant>& properties() { return properties_; }

 Q_SIGNALS:
  void finished(const QSharedPointer<vx::Data>& data);
};

class ExternalOperationExportAdapterImpl;
class VOXIEBACKEND_EXPORT ExternalOperationExport : public ExternalOperation {
  Q_OBJECT
  REFCOUNTEDOBJ_DECL(ExternalOperationExport)

  friend class ExternalOperationExportAdaptorImpl;

  QString filename_;
  QString name_;
  QSharedPointer<vx::Data> data_;

 public:
  explicit ExternalOperationExport(
      const QSharedPointer<vx::io::Operation>& operation,
      const QString& filename, const QString& name,
      const QSharedPointer<vx::Data>& data);
  ~ExternalOperationExport() override;

  QString action() override;

  QString name() override;

  // TODO: data
  const QString& filename() { return filename_; }

  const QSharedPointer<vx::Data>& data() { return data_; }

 Q_SIGNALS:
  void finished();
};

}  // namespace vx
