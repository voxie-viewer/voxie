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

#include "Importer.hpp"

#include <VoxieClient/ObjectExport/Client.hpp>

#include <VoxieBackend/IO/Operation.hpp>
#include <VoxieBackend/IO/OperationImport.hpp>
#include <VoxieBackend/IO/OperationResult.hpp>

#include <VoxieBackend/Property/PropertyBase.hpp>
#include <VoxieBackend/Property/PropertyType.hpp>

#include <VoxieClient/DBusAdaptors.hpp>
#include <VoxieClient/DBusUtil.hpp>
#include <VoxieClient/Exception.hpp>

using namespace vx;
using namespace vx::io;

namespace vx {
namespace io {
class Importer;

static QMutex staticMutex;
static Importer::CreateNodeHandler createNodeHandler;
void Importer::registerCreateNodeHandler(
    const Importer::CreateNodeHandler& fun) {
  QMutexLocker locker(&staticMutex);
  if (createNodeHandler)
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "createNodeHandler already registered");
  createNodeHandler = fun;
}
static Importer::CreateNodeHandler getCreateNodeHandler() {
  QMutexLocker locker(&staticMutex);
  return createNodeHandler;
}

class ImporterAdaptorImpl : public ImporterAdaptor {
  Importer* object;

 public:
  ImporterAdaptorImpl(Importer* object);
  ~ImporterAdaptorImpl() override;

  QMap<QString, QDBusVariant> filter() const override;

  QDBusObjectPath StartImport(
      const QDBusObjectPath& client, const QString& fileName,
      const QMap<QString, QDBusVariant>& options) override;

  QList<QDBusObjectPath> ListProperties(
      const QMap<QString, QDBusVariant>& options) override;
};

}  // namespace io
}  // namespace vx

vx::io::ImporterAdaptorImpl::ImporterAdaptorImpl(Importer* object)
    : ImporterAdaptor(object), object(object) {}
vx::io::ImporterAdaptorImpl::~ImporterAdaptorImpl() {}

QMap<QString, QDBusVariant> vx::io::ImporterAdaptorImpl::filter() const {
  return object->filter();
}

// TODO: A lot of code here is duplicated from InstanceAdaptorImpl::Import()
QDBusObjectPath vx::io::ImporterAdaptorImpl::StartImport(
    const QDBusObjectPath& client, const QString& fileName,
    const QMap<QString, QDBusVariant>& options) {
  auto absFileName = QFileInfo(fileName).absoluteFilePath();

  // Get a strong reference to the importer (needed because loading happens on
  // a separate thread and might not finish before the main thread drops its
  // reference on the importer)
  QSharedPointer<Importer> ptr(object->getSelf());
  try {
    if (!ptr)
      throw Exception("de.uni_stuttgart.Voxie.ObjectNotFound",
                      "Could not get strong reference to importer object");
    if (ptr.data() != object)
      throw Exception("de.uni_stuttgart.Voxie.Error", "object->self != object");
  } catch (vx::Exception& e) {
    e.handle(object);
    return vx::ExportedObject::getPath(nullptr);
  }

  try {
    auto handler = getCreateNodeHandler();

    QSet<QString> validOptions{"Properties", "AllowCompatibilityNames"};
    if (handler) validOptions.insert("CreateNode");
    ExportedObject::checkOptions(options, validOptions);

    bool allowCompatibilityNames =
        ExportedObject::getOptionValueOrDefault<bool>(
            options, "AllowCompatibilityNames", true);
    bool createNode = handler && ExportedObject::getOptionValueOrDefault<bool>(
                                     options, "CreateNode", false);

    Client* clientPtr =
        qobject_cast<Client*>(ExportedObject::lookupWeakObject(client));
    if (!clientPtr) {
      throw Exception("de.uni_stuttgart.Voxie.ObjectNotFound",
                      "Cannot find client object");
    }

    auto values =
        ExportedObject::getOptionValueOrDefault<QMap<QString, QDBusVariant>>(
            options, "Properties", {});
    QVariantMap properties;
    for (const auto& key : values.keys()) {
      auto property = object->getProperty(key, allowCompatibilityNames);
      auto valueRaw = property->type()->dbusToRaw(QDBusVariant(values[key]));
      if (properties.contains(property->name()))
        // Can happen with compatibility names
        throw vx::Exception(
            "de.uni_stuttgart.Voxie.DuplicateProperty",
            "Property '" + property->name() + "' was given multiple time");
      properties[property->name()] = valueRaw;
    }
    for (const auto& prop : object->properties()) {
      if (!properties.contains(prop->name()))
        properties[prop->name()] = prop->defaultValue();
    }

    auto operationResult = ptr->import(absFileName, createNode, properties);

    if (createNode) handler(operationResult, absFileName, ptr, properties);

    clientPtr->incRefCount(operationResult);
    return ExportedObject::getPath(operationResult);
  } catch (Exception& e) {
    return e.handle(object);
  }
}

QList<QDBusObjectPath> ImporterAdaptorImpl::ListProperties(
    const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);

    QList<QDBusObjectPath> propPaths;
    for (auto prop : object->properties()) {
      propPaths.append(vx::ExportedObject::getPath(prop));
    }
    return propPaths;
  } catch (Exception& e) {
    return e.handle(object);
  }
}

Importer::Importer(const QString& name, const FilenameFilter& filter,
                   const QList<QSharedPointer<PropertyBase>>& properties)
    : vx::plugin::Component(ComponentTypeInfo<Importer>::name(), name),
      filter_(filter),
      properties_(properties) {
  new ImporterAdaptorImpl(this);
}

Importer::~Importer() {}

void Importer::setSelf(const QSharedPointer<Importer>& ptr) {
  if (ptr.data() != this) {
    qCritical() << "Importer::setSelf called with incorrect object";
    return;
  }
  if (self) {
    qCritical() << "Importer::setSelf called with multiple times";
    return;
  }
  self = ptr;
}

QSharedPointer<vx::OperationImport::ResultWrapper> Importer::runThreaded(
    const QString& description, bool doCreateNode,
    std::function<QSharedPointer<vx::Data>(const QSharedPointer<Operation>&)>
        f) {
  using OpType = vx::OperationImport;
  auto op = OpType::create();
  auto result = OpType::ResultWrapper::create(op, doCreateNode);
  // auto result = Operation::runThreaded<OpType>(
  Operation::runThreaded(
      op, description,
      [fun = std::move(f), op](/*const QSharedPointer<Operation>& op*/) {
        auto data = fun(op);
        auto version =
            data->currentVersion();  // TODO: Return version from fun?
        return createQSharedPointer<OperationImport::Result>(data, version);
      });
  return result;
}

QSharedPointer<PropertyBase> Importer::getProperty(
    const QString& name, bool allowCompatibilityNames) {
  for (const auto& property : this->properties()) {
    if (property->name() == name) return property;
    if (allowCompatibilityNames)
      for (const auto& name2 : property->compatibilityNames())
        if (name2 == name) return property;
  }
  throw vx::Exception(
      "de.uni_stuttgart.Voxie.InvalidProperty",
      "Unknown property '" + name + "' for object '" + this->name() + "'");
}

QList<QString> Importer::supportedComponentDBusInterfaces() {
  return {"de.uni_stuttgart.Voxie.Importer"};
}
