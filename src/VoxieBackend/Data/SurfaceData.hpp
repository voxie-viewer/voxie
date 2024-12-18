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

#include <QtCore/QObject>
#include <QtCore/QVector>

#include <VoxieBackend/Data/Data.hpp>
#include <VoxieBackend/Data/SharedMemory.hpp>
#include <VoxieBackend/Data/SharedMemoryArray.hpp>
#include <VoxieBackend/VoxieBackend.hpp>

#include <QtGui/QVector3D>

#include <QQuaternion>

#include <array>
#include <cmath>

namespace vx {

class SurfaceBuilder;
class SurfaceAttribute;

/**
 * @brief Base class for all surface representations (currently only
 * @ref SurfaceDataTriangleIndexed).
 */
class VOXIEBACKEND_EXPORT SurfaceData : public Data {
  Q_OBJECT
  VX_REFCOUNTEDOBJECT

 public:
  SurfaceData();
  ~SurfaceData();

  virtual QList<QSharedPointer<SurfaceAttribute>> listAttributes() = 0;

  virtual QSharedPointer<SurfaceAttribute> getAttribute(
      const QString& name) = 0;
  virtual QSharedPointer<SurfaceAttribute> getAttributeOrNull(
      const QString& name) = 0;
};

/**
 * @brief The Surface class holds a list of vertices and triangles
 * The surface class holds all base data of a surface.
 */
class VOXIEBACKEND_EXPORT SurfaceDataTriangleIndexed : public SurfaceData {
  Q_OBJECT
  VX_REFCOUNTEDOBJECT

  friend class SurfaceBuilder;

 public:
  typedef quint32 IndexType;
  typedef std::array<IndexType, 3> Triangle;

  static const IndexType invalidIndex = -1;

 private:
  SharedMemoryArray<QVector3D> vertices_;
  QSharedPointer<SharedMemoryArray<Triangle>>
      triangles_;  // Can be shared with other SurfaceDataTriangleIndexed if
                   // trianglesWritable_ is false everywhere

  std::map<QString, QSharedPointer<SurfaceAttribute>> attributes_;

  QVector3D size_;
  QVector3D origin_;

  bool trianglesWritable_;

  void setMinMax();
  void checkVertexIndices();

  /**
   * @brief Create a new SurfaceDataTriangleIndexed object with a certain number
   * of triangles and vertices.
   * @param triangleCount The number of triangles
   * @param vertexCount The number of vertices
   * @param triangleSource If triangleSource is not nullptr, the triangles will
   * be copied from this object. The triangleCount must match
   * triangleSource->triangles().size()
   * @param trianglesWritable When false, the triangles will be read-only. Can
   * only be false when triangleSource is not nullptr. the surface
   */
  SurfaceDataTriangleIndexed(
      uint64_t triangleCount, uint64_t vertexCount,
      const QSharedPointer<SurfaceDataTriangleIndexed>& triangleSource,
      bool trianglesWritable,
      const QList<std::tuple<
          QString, QString, quint64, std::tuple<QString, quint32, QString>,
          QString, QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>&
          attributes);

  /**
   * @brief create Will move the data from the vectors to the newly created
   * SurfaceDataTriangleIndexed object; if you want to get hold of a new surface
   * object, this is the way to go
   * @param vertices a vector of 3d coordinates
   * @param triangles a vector of vertex indices representing triangle faces of
   * the surface
   */
  SurfaceDataTriangleIndexed(
      const std::vector<QVector3D>& vertices,
      const std::vector<Triangle>& triangles, bool trianglesWritable,
      const QList<std::tuple<
          QString, QString, quint64, std::tuple<QString, quint32, QString>,
          QString, QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>&
          attributes);

  // Allow the ctor to be called using create()
  friend class vx::RefCountedObject;

 public:
  ~SurfaceDataTriangleIndexed();

  QList<QString> supportedDBusInterfaces() override;

  /**
   * @brief Returns true if the @ref triangles() data may be written to.
   */
  bool trianglesWritable() { return trianglesWritable_; }

  SharedMemoryArray<QVector3D>& vertices() { return vertices_; }

  // triangles() may only be written to if trianglesWritable() is true
  SharedMemoryArray<Triangle>& triangles() { return *triangles_; }

  QVector3D size() { return this->size_; }

  QVector3D origin() { return this->origin_; }

  double diagonalSize() {
    return std::sqrt(std::pow(this->size_.x(), 2) +
                     std::pow(this->size_.y(), 2) +
                     std::pow(this->size_.z(), 2));
  }

  QList<QSharedPointer<SurfaceAttribute>> listAttributes() override;
  QSharedPointer<SurfaceAttribute> getAttribute(const QString& name) override;
  QSharedPointer<SurfaceAttribute> getAttributeOrNull(
      const QString& name) override;

  QList<QSharedPointer<SharedMemory>> getSharedMemorySections() override;
};

}  // namespace vx
