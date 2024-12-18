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

#include "GeometricPrimitiveData.hpp"

#include <VoxieClient/DBusAdaptors.hpp>

#include <VoxieBackend/Data/GeometricPrimitive.hpp>
#include <VoxieBackend/Data/GeometricPrimitiveType.hpp>

using namespace vx;

// TODO: locking?

class GeometricPrimitiveDataAdaptorImpl : public GeometricPrimitiveDataAdaptor {
  GeometricPrimitiveData* object;

 public:
  GeometricPrimitiveDataAdaptorImpl(GeometricPrimitiveData* object)
      : GeometricPrimitiveDataAdaptor(object), object(object) {}
  ~GeometricPrimitiveDataAdaptorImpl() override {}

  QList<std::tuple<quint64, QDBusObjectPath, QString,
                   QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>
  GetPrimitives(qulonglong firstID, qulonglong lastID,
                const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      QList<
          std::tuple<quint64, QDBusObjectPath, QString,
                     QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>
          result;
      const auto& map = object->primitives();
      for (uint64_t id : map.keys()) {
        // TODO: do this faster by not looking up unneeded values?
        if (id < firstID || id > lastID) continue;
        const auto& primitive = map[id];
        result << std::make_tuple(
            id, ExportedObject::getPath(primitive->primitiveType()),
            primitive->name(), primitive->primitiveValues(),
            QMap<QString, QDBusVariant>());
      }
      std::sort(result.begin(), result.end(),
                [](const auto& v1, const auto& v2) {
                  return std::get<0>(v1) < std::get<0>(v2);
                });
      return result;
    } catch (Exception& e) {
      e.handle(object);
      return QList<std::tuple<quint64, QDBusObjectPath, QString,
                              QMap<QString, QDBusVariant>,
                              QMap<QString, QDBusVariant>>>();
    }
  }

  qulonglong AddPrimitive(const QDBusObjectPath& update,
                          const QDBusObjectPath& primitiveType,
                          const QString& displayName,
                          const QMap<QString, QDBusVariant>& primitiveValues,
                          const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      auto updateObj = vx::DataUpdate::lookup(update);
      updateObj->validateCanUpdate(object);

      auto primitiveTypeObj = dynamic_cast<GeometricPrimitiveType*>(
          ExportedObject::lookupWeakObject(primitiveType));
      if (!primitiveTypeObj)
        throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                            "Invalid primitiveType parameter");

      auto primitive =
          primitiveTypeObj->createFunction()(displayName, primitiveValues);
      return object->addPrimitive(updateObj, primitive);
    } catch (Exception& e) {
      e.handle(object);
      return 0;
    }
  }
};

GeometricPrimitiveData::GeometricPrimitiveData() {
  new GeometricPrimitiveDataAdaptorImpl(this);
}
GeometricPrimitiveData::~GeometricPrimitiveData() {}

QList<QString> GeometricPrimitiveData::supportedDBusInterfaces() {
  return {
      "de.uni_stuttgart.Voxie.GeometricPrimitiveData",
  };
}

QList<quint64> GeometricPrimitiveData::getPrimitiveIDs() {
  QList<quint64> keys = primitives_.keys();
  std::sort(keys.begin(), keys.end());
  return keys;
}

QSharedPointer<GeometricPrimitive> GeometricPrimitiveData::getPrimitive(
    quint64 id) {
  if (!primitives_.contains(id))
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Primitive ID not found");
  return primitives_[id];
}

QSharedPointer<GeometricPrimitive> GeometricPrimitiveData::getPrimitiveOrNull(
    quint64 id) {
  if (!primitives_.contains(id)) return QSharedPointer<GeometricPrimitive>();
  return primitives_[id];
}

quint64 GeometricPrimitiveData::addPrimitive(
    const QSharedPointer<DataUpdate>& update,
    const QSharedPointer<GeometricPrimitive>& primitive) {
  if (!update)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error", "update is nullptr");
  if (!primitive)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error", "primitive is nullptr");
  update->validateCanUpdate(this);

  lastId++;
  auto id = lastId;
  primitives_.insert(id, primitive);
  return id;
}

void GeometricPrimitiveData::addOrReplacePrimitive(
    const QSharedPointer<DataUpdate>& update, quint64 id,
    const QSharedPointer<GeometricPrimitive>& newPrimitive) {
  if (!update)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error", "update is nullptr");
  /*
  if (!newPrimitive)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                                 "newPrimitive is nullptr");
  */
  update->validateCanUpdate(this);
  if (id == 0) throw vx::Exception("de.uni_stuttgart.Voxie.Error", "id is 0");

  if (id > lastId) lastId = id;
  if (newPrimitive)
    primitives_[id] = newPrimitive;
  else
    primitives_.remove(id);
}

QList<QSharedPointer<SharedMemory>>
GeometricPrimitiveData::getSharedMemorySections() {
  return {};
}
