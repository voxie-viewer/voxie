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

#include "ContainerData.hpp"
#include <VoxieClient/DBusAdaptors.hpp>

#include <QtCore/QFileInfo>
#include <Voxie/Node/DataNode.hpp>
#include <iostream>
#include <vector>

using namespace vx;

QString ContainerData::getName() { return this->name; }

int ContainerData::getSize() { return dataMap.size(); }

QList<QString> ContainerData::supportedDBusInterfaces() {
  return {
      "de.uni_stuttgart.Voxie.ContainerData",
  };
}

void ContainerData::insertElement(QString key, QSharedPointer<Data> value,
                                  QSharedPointer<vx::DataUpdate> update) {
  Q_UNUSED(update);
  if (this->dataMap.contains(key)) {
    auto child = this->dataMap[key];
    dataMap.remove(key);
    this->removeChildData(child);
  }

  this->addChildData(value);
  this->dataMap[key] = value;
}

void ContainerData::removeElement(QString key,
                                  QSharedPointer<vx::DataUpdate> update) {
  Q_UNUSED(update);
  if (this->dataMap.contains(key)) {
    auto child = this->dataMap[key];
    dataMap.remove(key);
    this->removeChildData(child);
  } else {
    throw vx::Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                        QString("Could not remove element. Key: %1 is not "
                                "contained in ContainerData")
                            .arg(key));
  }
}

QSharedPointer<Data> ContainerData::getElement(QString key) {
  if (this->dataMap.contains(key))
    return this->dataMap[key];
  else
    throw vx::Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                        "Could not find element. Key: " + key +
                            " is not contained in ContainerData");
}

QList<QString> ContainerData::getKeys() { return this->dataMap.keys(); }

QList<QSharedPointer<Data>> ContainerData::getValues() {
  return this->dataMap.values();
}

QList<QSharedPointer<SharedMemory>> ContainerData::getSharedMemorySections() {
  QList<QSharedPointer<SharedMemory>> sharedMemory = {};

  QMap<QString, QSharedPointer<Data>>::iterator i;

  for (i = this->dataMap.begin(); i != this->dataMap.end(); ++i) {
    QList<QSharedPointer<SharedMemory>>::iterator j;
    QList<QSharedPointer<SharedMemory>> memoryData =
        (*((*i).data())).getSharedMemorySections();
    for (j = memoryData.begin(); j != memoryData.end(); ++j) {
      sharedMemory.append((*j));
    }
  }

  return sharedMemory;
}

class ContainerDataAdaptorImpl : public ContainerDataAdaptor {
  ContainerData* object;

 public:
  ContainerDataAdaptorImpl(ContainerData* object)
      : ContainerDataAdaptor(object), object(object) {}
  ~ContainerDataAdaptorImpl() override {}

  QDBusObjectPath GetElement(
      const QString& key, const QMap<QString, QDBusVariant>& options) override {
    try {
      vx::ExportedObject::checkOptions(options);

      return vx::ExportedObject::getPath(object->getElement(key));

    } catch (Exception& e) {
      e.handle(object);
      throw vx::Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                          "Error in getElement of ContainerDataAdaptorImpl");
    }
  }

  QString GetName(const QMap<QString, QDBusVariant>& options) override {
    try {
      vx::ExportedObject::checkOptions(options);

      return object->getName();

    } catch (Exception& e) {
      e.handle(object);
      throw vx::Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                          "Error in getName of ContainerDataAdaptorImpl");
    }
  }

  QList<QDBusObjectPath> GetValues(
      const QMap<QString, QDBusVariant>& options) override {
    auto values = QList<QDBusObjectPath>();

    try {
      vx::ExportedObject::checkOptions(options);
      for (QSharedPointer<Data> data : object->getValues()) {
        values.append(vx::ExportedObject::getPath(data));
      }

      return values;

    } catch (Exception& e) {
      e.handle(object);
      throw vx::Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                          "Error in getValues of ContainerDataAdaptorImpl");
    }
  }

  QStringList GetKeys(const QMap<QString, QDBusVariant>& options) override {
    try {
      vx::ExportedObject::checkOptions(options);
      return object->getKeys();

    } catch (Exception& e) {
      e.handle(object);
      throw vx::Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                          "Error in getValues of ContainerDataAdaptorImpl");
    }
  }

  void InsertElement(const QString& key, const QDBusObjectPath& value,
                     const QDBusObjectPath& update,
                     const QMap<QString, QDBusVariant>& options) override {
    try {
      vx::ExportedObject::checkOptions(options);

      return object->insertElement(key, Data::lookup(value),
                                   DataUpdate::lookup(update));

    } catch (Exception& e) {
      e.handle(object);
      throw vx::Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                          "Error in InsertElement of ContainerDataAdaptorImpl");
    }
  }
};

ContainerData::ContainerData(QString name) : Data(), DataContainer(this) {
  new ContainerDataAdaptorImpl(this);
  this->name = name;
}
