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

#include <Voxie/Voxie.hpp>

#include <VoxieBackend/Data/SurfaceData.hpp>

#include <QtCore/QDebug>
#include <QtCore/QMap>

namespace vx {

class VOXIECORESHARED_EXPORT SurfaceBuilder : public QObject {
  Q_OBJECT

 public:
  using IndexType = SurfaceDataTriangleIndexed::IndexType;
  using Triangle = SurfaceDataTriangleIndexed::Triangle;

 private:
  struct Vec3Wrapper {
    QVector3D data;
    Vec3Wrapper() {}
    Vec3Wrapper(const QVector3D& data) : data(data) {}
    operator QVector3D() const { return data; }

    bool operator<(const Vec3Wrapper& second) const {
      if (data.x() < second.data.x())
        return true;
      else if (data.x() > second.data.x())
        return false;

      if (data.y() < second.data.y())
        return true;
      else if (data.y() > second.data.y())
        return false;

      if (data.z() < second.data.z())
        return true;
      else if (data.z() > second.data.z())
        return false;

      return false;
    }
  };

  std::vector<QVector3D> vertices_;
  std::vector<Triangle> triangles_;
  // QMap<Vec3Wrapper, IndexType> indices_;

 public:
  SurfaceBuilder(QObject* parent = nullptr);
  ~SurfaceBuilder();

  IndexType addVertex(QVector3D vertex) {
    /*
    auto it = indices_.constFind(vertex);
    if (it != indices_.constEnd()) {
        qWarning() << "Added vertex" << vertex << "multiple times";
        //return *it;
    }
    */

    IndexType index = vertices_.size();
    vertices_.push_back(vertex);
    // indices_.insert(vertex, index);
    return index;
  }

  void addTriangle(IndexType a, IndexType b, IndexType c) {
    /*
    if (a == b || a == c || b == c)
        qWarning() << "Warning: Got collapsed triangle with indices" << a << b
    << c;
    */
    triangles_.push_back({{a, b, c}});
  }

  void addTriangle(QVector3D a, QVector3D b, QVector3D c) {
    addTriangle(addVertex(a), addVertex(b), addVertex(c));
  }

  void clear();

  // Will clear() the builder object
  // TODO: rename to createSurface() and leave the builder alone (cannot reuse
  // the vectors anyway because of the SharedMemory stuff)
  QSharedPointer<SurfaceDataTriangleIndexed> createSurfaceClearBuilder(
      const QList<std::tuple<
          QString, QString, quint64, std::tuple<QString, quint32, QString>,
          QString, QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>&
          attributes = {});
};

}  // namespace vx
