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

#include "Exporter.hpp"

#include <VoxieClient/ObjectExport/Client.hpp>

#include <VoxieBackend/IO/Operation.hpp>
#include <VoxieBackend/IO/OperationResult.hpp>

#include <VoxieClient/DBusAdaptors.hpp>
#include <VoxieClient/DBusUtil.hpp>
#include <VoxieClient/Exception.hpp>

using namespace vx;
using namespace vx::io;

namespace vx {
namespace io {
class ExporterAdaptorImpl : public ExporterAdaptor {
  Exporter* object;

 public:
  ExporterAdaptorImpl(Exporter* object)
      : ExporterAdaptor(object), object(object) {}
  ~ExporterAdaptorImpl() override {}

  QMap<QString, QDBusVariant> filter() const override {
    return object->filter();
  }

  QDBusObjectPath StartExport(
      const QDBusObjectPath& client, const QDBusObjectPath& data,
      const QString& fileName,
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      Client* clientPtr =
          qobject_cast<Client*>(ExportedObject::lookupWeakObject(client));
      if (!clientPtr) {
        throw Exception("de.uni_stuttgart.Voxie.ObjectNotFound",
                        "Cannot find client object");
      }

      auto dataObj = vx::Data::lookup(data);

      auto operationResult = object->exportData(dataObj, fileName);

      clientPtr->incRefCount(operationResult);
      return ExportedObject::getPath(operationResult);
    } catch (Exception& e) {
      e.handle(object);
      return ExportedObject::getPath(nullptr);
    }
  }

  void Export(const QDBusObjectPath& data, const QString& fileName,
              const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      auto dataObj = vx::Data::lookup(data);

      auto operationResult = object->exportData(dataObj, fileName);

      operationResult->operation()->dbusDelayedReturn(object);
    } catch (Exception& e) {
      e.handle(object);
    }
  }

  QString FilterAddExtension(
      const QString& filename,
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      return object->filter().addExtension(filename);
    } catch (Exception& e) {
      e.handle(object);
      return "";
    }
  }
};
}  // namespace io
}  // namespace vx

Exporter::Exporter(const QString& name, const FilenameFilter& filter,
                   const QList<QString>& targetPrototypeNames)
    // TODO: Pass json (e.g. for TroveClassifiers)
    : vx::plugin::Component(ComponentTypeInfo<Exporter>::name(), name, {}),
      filter_(filter),
      targetPrototypeNames_(targetPrototypeNames) {
  new ExporterAdaptorImpl(this);
}

Exporter::~Exporter() {}

void Exporter::setSelf(const QSharedPointer<Exporter>& ptr) {
  if (ptr.data() != this) {
    qCritical() << "Exporter::setSelf called with incorrect object";
    return;
  }
  if (self) {
    qCritical() << "Exporter::setSelf called with multiple times";
    return;
  }
  self = ptr;
}

bool Exporter::matches(const QString& prototypeName) {
  // qDebug() << "match" << prototype;
  for (const auto& name : targetPrototypeNames()) {
    // qDebug() << "match2" << name << prototype->name();
    if (name == prototypeName) return true;
  }
  return false;
}

QSharedPointer<OperationResult> Exporter::runThreaded(
    const QString& description,
    std::function<void(const QSharedPointer<Operation>&)> f) {
  return Operation::runThreaded<vx::OperationSimple>(
      description, [fun = std::move(f)](const QSharedPointer<Operation>& op) {
        fun(op);
        return createQSharedPointer<Operation::ResultSuccess>();
      });
}

QList<QString> Exporter::supportedComponentDBusInterfaces() {
  return {"de.uni_stuttgart.Voxie.Exporter"};
}
