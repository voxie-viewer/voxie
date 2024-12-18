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

#include "SurfaceData.hpp"

#include <VoxieBackend/DebugOptions.hpp>

#include <QPushButton>

#include <QtWidgets/QAction>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <VoxieBackend/Data/SurfaceAttribute.hpp>

#include <VoxieClient/ArrayInfo.hpp>
#include <VoxieClient/DBusAdaptors.hpp>
#include <VoxieClient/DBusTypeList.hpp>

using namespace vx;

class SurfaceDataAdaptorImpl : public SurfaceDataAdaptor {
  SurfaceData* object;

 public:
  SurfaceDataAdaptorImpl(SurfaceData* object)
      : SurfaceDataAdaptor(object), object(object) {}
  ~SurfaceDataAdaptorImpl() override {}

  QList<std::tuple<QString, QString, quint64,
                   std::tuple<QString, quint32, QString>, QString,
                   QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>
  attributes() const override {
    QList<std::tuple<QString, QString, quint64,
                     std::tuple<QString, quint32, QString>, QString,
                     QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>
        list;
    for (const auto& attribute : object->listAttributes())
      list << attribute->dbusData();
    return list;
  }
};

SurfaceData::SurfaceData() { new SurfaceDataAdaptorImpl(this); }
SurfaceData::~SurfaceData() {}

class SurfaceDataTriangleIndexedAdaptorImpl
    : public SurfaceDataTriangleIndexedAdaptor {
  SurfaceDataTriangleIndexed* object;

  vx::Array2Info getTriangles(bool writable) {
    typedef SurfaceDataTriangleIndexed::Triangle T3;
    typedef T3::value_type T;

    vx::Array2Info info;

    object->triangles().getSharedMemory()->getHandle(writable, info.handle);
    info.offset = 0;

    info.dataType = "uint";  // TODO: don't hardcode this?
    info.dataTypeSize = sizeof(T) * 8;
    if (info.dataTypeSize == 1) {
      info.byteorder = "none";
    } else {
      if (QSysInfo::Endian::ByteOrder == QSysInfo::Endian::BigEndian) {
        info.byteorder = "big";
      } else if (QSysInfo::Endian::ByteOrder ==
                 QSysInfo::Endian::LittleEndian) {
        info.byteorder = "little";
      } else {
        info.byteorder = "unknown";
      }
    }

    info.sizeX = object->triangles().size();
    info.sizeY = 3;
    info.strideX = sizeof(T3);
    info.strideY = sizeof(T);

    return info;
  }

  vx::Array2Info getVertices(bool writable) {
    typedef QVector3D T3;
    typedef float T;

    vx::Array2Info info;

    object->vertices().getSharedMemory()->getHandle(writable, info.handle);
    info.offset = 0;

    info.dataType = "float";  // TODO: don't hardcode this?
    info.dataTypeSize = sizeof(T) * 8;
    if (info.dataTypeSize == 1) {
      info.byteorder = "none";
    } else {
      if (QSysInfo::Endian::ByteOrder == QSysInfo::Endian::BigEndian) {
        info.byteorder = "big";
      } else if (QSysInfo::Endian::ByteOrder ==
                 QSysInfo::Endian::LittleEndian) {
        info.byteorder = "little";
      } else {
        info.byteorder = "unknown";
      }
    }

    info.sizeX = object->vertices().size();
    info.sizeY = 3;
    info.strideX = sizeof(T3);
    info.strideY = sizeof(T);

    return info;
  }

  vx::Array2Info getAttribute(const QString& name, bool writable) {
    auto attribute = object->getAttribute(name);

    vx::Array2Info info;

    attribute->getSharedMemory()->getHandle(writable, info.handle);
    info.offset = 0;

    getDataTypeInfo(attribute->dataType(), info.dataType, info.dataTypeSize,
                    info.byteorder);

    info.sizeX = attribute->count();
    info.sizeY = attribute->componentCount();
    info.strideX = getElementSizeBytes(attribute->dataType()) *
                   attribute->componentCount();
    info.strideY = getElementSizeBytes(attribute->dataType());

    return info;
  }

 public:
  SurfaceDataTriangleIndexedAdaptorImpl(SurfaceDataTriangleIndexed* object)
      : SurfaceDataTriangleIndexedAdaptor(object), object(object) {}
  ~SurfaceDataTriangleIndexedAdaptorImpl() override {}

  bool trianglesWritable() const override {
    return object->trianglesWritable();
  }

  std::tuple<QMap<QString, QDBusVariant>, qint64,
             std::tuple<QString, quint32, QString>,
             std::tuple<quint64, quint64>, std::tuple<qint64, qint64>,
             QMap<QString, QDBusVariant>>
  GetTrianglesReadonly(const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      return getTriangles(false).toDBus();
    } catch (Exception& e) {
      e.handle(object);
      return vx::Array2Info().toDBus();
    }
  }

  std::tuple<QMap<QString, QDBusVariant>, qint64,
             std::tuple<QString, quint32, QString>,
             std::tuple<quint64, quint64>, std::tuple<qint64, qint64>,
             QMap<QString, QDBusVariant>>
  GetTrianglesWritable(const QDBusObjectPath& update,
                       const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      auto updateObj = vx::DataUpdate::lookup(update);
      updateObj->validateCanUpdate(object);

      if (!object->trianglesWritable())
        throw vx::Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                            "Triangle data not writable");

      return getTriangles(true).toDBus();
    } catch (Exception& e) {
      e.handle(object);
      return vx::Array2Info().toDBus();
    }
  }

  std::tuple<QMap<QString, QDBusVariant>, qint64,
             std::tuple<QString, quint32, QString>,
             std::tuple<quint64, quint64>, std::tuple<qint64, qint64>,
             QMap<QString, QDBusVariant>>
  GetVerticesReadonly(const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      return getVertices(false).toDBus();
    } catch (Exception& e) {
      e.handle(object);
      return vx::Array2Info().toDBus();
    }
  }

  std::tuple<QMap<QString, QDBusVariant>, qint64,
             std::tuple<QString, quint32, QString>,
             std::tuple<quint64, quint64>, std::tuple<qint64, qint64>,
             QMap<QString, QDBusVariant>>
  GetVerticesWritable(const QDBusObjectPath& update,
                      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      auto updateObj = vx::DataUpdate::lookup(update);
      updateObj->validateCanUpdate(object);

      return getVertices(true).toDBus();
    } catch (Exception& e) {
      e.handle(object);
      return vx::Array2Info().toDBus();
    }
  }

  std::tuple<QMap<QString, QDBusVariant>, qint64,
             std::tuple<QString, quint32, QString>,
             std::tuple<quint64, quint64>, std::tuple<qint64, qint64>,
             QMap<QString, QDBusVariant>>
  GetAttributeReadonly(const QString& name,
                       const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      return getAttribute(name, false).toDBus();
    } catch (Exception& e) {
      e.handle(object);
      return vx::Array2Info().toDBus();
    }
  }
  std::tuple<QMap<QString, QDBusVariant>, qint64,
             std::tuple<QString, quint32, QString>,
             std::tuple<quint64, quint64>, std::tuple<qint64, qint64>,
             QMap<QString, QDBusVariant>>
  GetAttributeWritable(const QDBusObjectPath& update, const QString& name,
                       const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      auto updateObj = vx::DataUpdate::lookup(update);
      updateObj->validateCanUpdate(object);

      return getAttribute(name, true).toDBus();
    } catch (Exception& e) {
      e.handle(object);
      return vx::Array2Info().toDBus();
    }
  }

  quint64 triangleCount() const override {
    try {
      return object->triangles().size();
    } catch (Exception& e) {
      e.handle(object);
      return 0;
    }
  }

  quint64 vertexCount() const override {
    try {
      return object->vertices().size();
    } catch (Exception& e) {
      e.handle(object);
      return 0;
    }
  }
};

void SurfaceDataTriangleIndexed::checkVertexIndices() {
  for (size_t i = 0; i < triangles().size(); i++) {
    auto triangle = triangles()[i];
    for (size_t j = 0; j < 3; j++)
      if (triangle[j] >= vertices().size())
        throw vx::Exception("de.uni_stuttgart.Voxie.InvalidVertexIndex",
                            "Vertex index out of range");
  }
}

SurfaceDataTriangleIndexed::SurfaceDataTriangleIndexed(
    uint64_t triangleCount, uint64_t vertexCount,
    const QSharedPointer<SurfaceDataTriangleIndexed>& triangleSource,
    bool trianglesWritable,
    const QList<std::tuple<
        QString, QString, quint64, std::tuple<QString, quint32, QString>,
        QString, QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>&
        attributes)
    : vertices_(vertexCount), trianglesWritable_(trianglesWritable) {
  new SurfaceDataTriangleIndexedAdaptorImpl(this);
  connect(this, &Data::dataChanged, this,
          [this](const QSharedPointer<DataVersion>& newVersion,
                 DataChangedReason reason) {
            Q_UNUSED(newVersion);
            // TODO: this should probably this be done in a way to make sure
            // that it will be done before DataUpdate::Finish() return
            // (currently this is not the case because Data::dataChanged is
            // emitted asynchronously)
            if (reason != DataChangedReason::NewUpdate)
              Q_EMIT this->setMinMax();
          });

  if (triangleSource) {
    if (triangleCount != triangleSource->triangles().size())
      throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                          "Triangle count does not match");
    if (!trianglesWritable && !triangleSource->trianglesWritable()) {
      // Link
      triangles_ = triangleSource->triangles_;
    } else {
      // Either source or destination is writable, copy
      triangles_ =
          createQSharedPointer<SharedMemoryArray<Triangle>>(triangleCount);
      // TODO: faster copy?
      for (size_t i = 0; i < triangles_->size(); i++)
        (*triangles_)[i] = triangleSource->triangles()[i];
      if (triangleSource->trianglesWritable() && !trianglesWritable)
        checkVertexIndices();
    }
  } else {
    if (!trianglesWritable)
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.InvalidArgument",
          "Attempting to create a surface with non-writable triangles without "
          "triangle source");
    triangles_ =
        createQSharedPointer<SharedMemoryArray<Triangle>>(triangleCount);
  }

  for (const auto& attributeData : attributes) {
    auto attribute =
        createQSharedPointer<SurfaceAttribute>(this, attributeData);
    this->attributes_[attribute->name()] = attribute;
  }

  // TODO: ?
  setMinMax();
}

SurfaceDataTriangleIndexed::SurfaceDataTriangleIndexed(
    const std::vector<QVector3D>& vertices,
    const std::vector<Triangle>& triangles, bool trianglesWritable,
    const QList<std::tuple<
        QString, QString, quint64, std::tuple<QString, quint32, QString>,
        QString, QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>&
        attributes)
    : vertices_(vertices.size()),
      triangles_(
          createQSharedPointer<SharedMemoryArray<Triangle>>(triangles.size())),
      trianglesWritable_(trianglesWritable) {
  new SurfaceDataTriangleIndexedAdaptorImpl(this);
  connect(this, &Data::dataChanged, this,
          [this](const QSharedPointer<DataVersion>& newVersion,
                 DataChangedReason reason) {
            Q_UNUSED(newVersion);
            // TODO: this should probably this be done in a way to make sure
            // that it will be done before DataUpdate::Finish() return
            // (currently this is not the case because Data::dataChanged is
            // emitted asynchronously)
            if (reason != DataChangedReason::NewUpdate)
              Q_EMIT this->setMinMax();
          });

  for (size_t i = 0; i < vertices_.size(); i++) vertices_[i] = vertices[i];
  for (size_t i = 0; i < triangles_->size(); i++)
    (*triangles_)[i] = triangles[i];

  checkVertexIndices();

  for (const auto& attributeData : attributes) {
    auto attribute =
        createQSharedPointer<SurfaceAttribute>(this, attributeData);
    this->attributes_[attribute->name()] = attribute;
  }

  setMinMax();
}

void SurfaceDataTriangleIndexed::setMinMax() {
  // TODO: what should be set for 0 vertices?
  float min = std::numeric_limits<float>::min();
  float max = std::numeric_limits<float>::max();

  float minX = vertices().size() == 0 ? 0 : max;
  float minY = vertices().size() == 0 ? 0 : max;
  float minZ = vertices().size() == 0 ? 0 : max;
  float maxX = vertices().size() == 0 ? 0 : min;
  float maxY = vertices().size() == 0 ? 0 : min;
  float maxZ = vertices().size() == 0 ? 0 : min;

  for (size_t i = 0; i < vertices().size(); i++) {
    QVector3D vertex = vertices().data()[i];
    if (vertex.x() < minX) minX = vertex.x();
    if (vertex.y() < minY) minY = vertex.y();
    if (vertex.z() < minZ) minZ = vertex.z();
    if (vertex.x() > maxX) maxX = vertex.x();
    if (vertex.y() > maxY) maxY = vertex.y();
    if (vertex.z() > maxZ) maxZ = vertex.z();
  }

  this->size_.setX(maxX - minX);
  this->size_.setY(maxY - minY);
  this->size_.setZ(maxZ - minZ);

  this->origin_.setX(minX);
  this->origin_.setY(minY);
  this->origin_.setZ(minZ);

  if (vx::debug_option::Log_SurfaceBoundingBox()->get())
    qDebug() << "Surface: Got bounding box:" << this->origin_ << this->size_;
}

SurfaceDataTriangleIndexed::~SurfaceDataTriangleIndexed() {}

QList<QString> SurfaceDataTriangleIndexed::supportedDBusInterfaces() {
  return {
      "de.uni_stuttgart.Voxie.SurfaceDataTriangleIndexed",
      "de.uni_stuttgart.Voxie.SurfaceData",
  };
}

QList<QSharedPointer<SurfaceAttribute>>
SurfaceDataTriangleIndexed::listAttributes() {
  QList<QSharedPointer<SurfaceAttribute>> res;
  for (const auto& it : this->attributes_) res << it.second;
  std::sort(res.begin(), res.end(), [](const auto& i1, const auto& i2) {
    return i1->name() < i2->name();
  });
  return res;
}
QSharedPointer<SurfaceAttribute> SurfaceDataTriangleIndexed::getAttribute(
    const QString& name) {
  const auto& attributes = this->attributes_;
  auto it = attributes.find(name);
  if (it == attributes.end())
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Could not find surface attribute '" + name + "'");
  return it->second;
}
QSharedPointer<SurfaceAttribute> SurfaceDataTriangleIndexed::getAttributeOrNull(
    const QString& name) {
  const auto& attributes = this->attributes_;
  auto it = attributes.find(name);
  if (it == attributes.end()) return QSharedPointer<SurfaceAttribute>();
  return it->second;
}

QList<QSharedPointer<SharedMemory>>
SurfaceDataTriangleIndexed::getSharedMemorySections() {
  QList<QSharedPointer<SharedMemory>> result;

  result << vertices().getSharedMemory();
  result << triangles().getSharedMemory();

  for (const auto& entry : attributes_)
    result << entry.second->getSharedMemory();

  return result;
}

const SurfaceDataTriangleIndexed::IndexType
    SurfaceDataTriangleIndexed::invalidIndex;
