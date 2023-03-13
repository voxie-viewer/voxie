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

#include "DataNode.hpp"

#include <VoxieClient/DBusAdaptors.hpp>

#include <Voxie/IVoxie.hpp>

#include <VoxieBackend/Component/ComponentContainer.hpp>

#include <VoxieBackend/IO/Exporter.hpp>
#include <VoxieBackend/IO/Importer.hpp>

#include <VoxieBackend/Data/Data.hpp>

#include <VoxieBackend/Property/PropertyType.hpp>

using namespace vx;

namespace vx {
namespace {
class DataNodeAdaptorImpl : public DataNodeAdaptor, public DataObjectAdaptor {
  DataNode* object;

 public:
  DataNodeAdaptorImpl(DataNode* object)
      : DataNodeAdaptor(object), DataObjectAdaptor(object), object(object) {}
  virtual ~DataNodeAdaptorImpl() {}

  QString fileName() const override { return object->getFileInfo().filePath(); }

  QDBusObjectPath importer() const override {
    try {
      return ExportedObject::getPath(object->importer());
    } catch (Exception& e) {
      return e.handle(object);
    }
  }

  QMap<QString, QDBusVariant> importProperties() const override {
    try {
      auto properties = object->importProperties();

      QMap<QString, QDBusVariant> propertiesDBus;
      for (const auto& key : properties.keys()) {
        auto prop = object->importer()->getProperty(key, false);
        propertiesDBus[key] = prop->type()->rawToDBus(properties[key]);
      }

      return propertiesDBus;
    } catch (Exception& e) {
      return e.handle(object);
    }
  }

  QDBusObjectPath data() const override {
    try {
      return ExportedObject::getPath(object->data());
    } catch (Exception& e) {
      e.handle(object);
      return ExportedObject::getPath(nullptr);
    }
  }

  QDBusObjectPath defaultExporter() const override {
    try {
      return ExportedObject::getPath(object->defaultExporter());
    } catch (Exception& e) {
      e.handle(object);
      return ExportedObject::getPath(nullptr);
    }
  }
};
}  // namespace
}  // namespace vx

DataNode::DataNode(const QString& type,
                   const QSharedPointer<NodePrototype>& prototype)
    : Node(type, prototype) {
  new DataNodeAdaptorImpl(this);

  // Drop reference to data after node is destroyed
  QObject::connect(this, &Node::stateChanged, this,
                   [this](Node::State newState) {
                     if (newState != Node::State::Destroyed) return;
                     this->setData(QSharedPointer<Data>());
                   });

  // Forward from dataChanged to dataChangedFinished
  QObject::connect(this, &DataNode::dataChanged, this,
                   [this](const QSharedPointer<Data>& data,
                          const QSharedPointer<DataVersion>& newVersion,
                          DataChangedReason reason) {
                     if (reason != DataChangedReason::NewUpdate)
                       Q_EMIT this->dataChangedFinished(data, newVersion,
                                                        reason);
                   });

  if (!vx::voxieRoot().isHeadless()) vx::voxieRoot().createDataNodeUI(this);
}

QList<QString> DataNode::supportedDBusInterfaces() {
  return {
      "de.uni_stuttgart.Voxie.DataNode",
      "de.uni_stuttgart.Voxie.DataObject",
  };
}

void DataNode::setData(const QSharedPointer<Data>& data) {
  if (data && this->state() == Node::State::Destroyed)
    throw vx::Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                        "Data cannot be set after node has been destroyed");

  setDataImpl(data);

  if (dataChangedConnection) {
    QObject::disconnect(dataChangedConnection);
    dataChangedConnection = QMetaObject::Connection();
  }
  if (data) {
    dataChangedConnection = QObject::connect(
        data.data(), &Data::dataChanged, this,
        [this, data](const QSharedPointer<DataVersion>& newVersion,
                     DataChangedReason reason) {
          // qDebug() << "Data::dataChanged" << this << data << newVersion <<
          // newUpdate;
          Q_EMIT this->dataChanged(data, newVersion, reason);
        });
  }

  QSharedPointer<DataVersion> currentVersion;
  if (data) currentVersion = data->currentVersion();
  // qDebug() << "Data::dataChanged" << this << data << currentVersion
  // << "DataInstanceChanged";
  Q_EMIT this->dataChanged(data, currentVersion,
                           DataChangedReason::DataInstanceChanged);
}

bool DataNode::isAllowedChild(NodeKind kind) {
  return kind == NodeKind::Filter || kind == NodeKind::Visualizer ||
         kind == NodeKind::Object3D;
}

bool DataNode::isAllowedParent(NodeKind kind) {
  return kind == NodeKind::Filter;
}

void DataNode::setFileInfo(const QFileInfo& fileInfo,
                           const QSharedPointer<vx::io::Importer>& importer,
                           const QMap<QString, QVariant>& importProperties) {
  QMap<QString, QVariant> importProperties2 = importProperties;
  for (const auto& property : importer->properties()) {
    if (importProperties2[property->name()] == property->defaultValue())
      importProperties2.remove(property->name());
  }

  this->fileInfo = fileInfo;
  this->importer_ = importer;
  this->importProperties_ = importProperties2;

  fileInfoChanged(fileInfo);
}

bool DataNode::doTagsMatch(QSharedPointer<NodeProperty> property) {
  for (QSharedPointer<NodeTag> tag : property->inTags()) {
    if (!tags.contains(tag)) {
      return false;
    }
  }
  return true;
}

/**
 * @brief Return true if there is a property of childPrototype with matching
 * input tags or if there are no input tags.
 */
bool DataNode::hasMatchingTags(NodePrototype* childPrototype) {
  // TODO: doTagsMatch() should only be called if the property can be used to
  // connect this to childPrototype.
  bool allEmpty = true;
  for (QSharedPointer<NodeProperty> prop : childPrototype->nodeProperties()) {
    if (!prop->inTags().empty()) {
      allEmpty = false;
      if (doTagsMatch(prop)) {
        return true;
      }
    }
  }
  return allEmpty;
}

QSharedPointer<vx::io::Exporter> DataNode::defaultExporter() {
  // TODO: Currently the exporter only depends on the prototype
  // This means e.g. that a voxel and an non-voxel volume datasets will have the
  // same default exporter.
  // It might be a good idea to make this more flexible.

  // auto data = this->data();
  // if (!data) return QSharedPointer<vx::io::Exporter>();

  auto dataExporterName = this->prototype()->defaultExporterName();

  if (dataExporterName == "")
    return QSharedPointer<vx::io::Exporter>();  // No exporter available

  return vx::voxieRoot().components()->getComponentTyped<vx::io::Exporter>(
      dataExporterName, false);
}
