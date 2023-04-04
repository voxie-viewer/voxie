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

#include "SurfaceData3D.hpp"

#include <VoxieBackend/Data/SurfaceAttribute.hpp>
#include <VoxieBackend/Data/SurfaceData.hpp>

// #include <PluginVis3D/DebugOptions.hpp>
#include <Voxie/DebugOptions.hpp>

using namespace vx;

void SurfaceData3D::setVisibility(bool isVisible) {
  if (this->_isVisible == isVisible) return;
  this->_isVisible = isVisible;
  Q_EMIT visibilityChanged(this);
  Q_EMIT changed();
}

/*
 * @brief Create a copy of the surface where the controlling vertex (the last
 * one) of every triangle is not the controlling vertex of any other triangle
 */
static QSharedPointer<vx::SurfaceDataTriangleIndexed>
makeControllingVertexUnique(vx::SurfaceDataTriangleIndexed* surface) {
  // Array indicating whether a vertex has already been used as a controlling
  // vertex
  std::vector<bool> isUsed(surface->vertices().size(), false);

  // The new triangle values, initialize with old values
  std::vector<SurfaceDataTriangleIndexed::Triangle> triangles;
  surface->triangles().writeToVector(triangles);

  // The new vertex values, initialize with old values
  std::vector<QVector3D> vertices;
  surface->vertices().writeToVector(vertices);

  std::vector<SurfaceDataTriangleIndexed::IndexType> newVerticesSource;

  for (size_t i = 0; i < triangles.size(); i++) {
    SurfaceDataTriangleIndexed::Triangle& triangle = triangles[i];
    if (!isUsed[triangle[2]]) {
      // Last triangle is not yet used, use it, no shift needed
      isUsed[triangle[2]] = true;
    } else if (!isUsed[triangle[0]]) {
      // First triangle is not yet used, use it, original vertex 1 is now vertex
      // 0
      isUsed[triangle[0]] = true;
      std::swap(triangle[0], triangle[1]);
      std::swap(triangle[1], triangle[2]);
    } else if (!isUsed[triangle[1]]) {
      // Second triangle is not yet used, use it, original vertex 2 is now
      // vertex 0
      isUsed[triangle[1]] = true;
      std::swap(triangle[1], triangle[2]);
      std::swap(triangle[0], triangle[1]);
    } else {
      // All vertices already used, copy vertex 2, no shift
      auto pos = vertices.size();
      vertices.push_back(vertices[triangle[2]]);
      newVerticesSource.push_back(triangle[2]);
      triangle[2] = pos;
    }
  }

  // TODO: What should happen to attributes here?
  QList<std::tuple<QString, QString, quint64,
                   std::tuple<QString, quint32, QString>, QString,
                   QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>
      attributes;
  for (const auto& srcAttr : surface->listAttributes()) {
    if (srcAttr->kind() !=
            "de.uni_stuttgart.Voxie.SurfaceAttributeKind.Vertex" &&
        srcAttr->kind() !=
            "de.uni_stuttgart.Voxie.SurfaceAttributeKind.Triangle") {
      qWarning() << "makeControllingVertexUnique: Unknown triangle kind:"
                 << srcAttr->kind();
      continue;
    }
    attributes << srcAttr->dbusData();
  }
  auto res = SurfaceDataTriangleIndexed::create(vertices, triangles, false,
                                                attributes);
  for (const auto& dstAttr : res->listAttributes()) {
    auto srcAttr = surface->getAttribute(dstAttr->name());
    if (srcAttr->kind() ==
        "de.uni_stuttgart.Voxie.SurfaceAttributeKind.Vertex") {
      for (uint64_t i = 0; i < dstAttr->count(); i++) {
        if (i < srcAttr->count())
          dstAttr->copyEntryFrom(i, srcAttr, i);
        else
          dstAttr->copyEntryFrom(i, srcAttr,
                                 newVerticesSource.at(i - srcAttr->count()));
      }
    } else if (srcAttr->kind() ==
               "de.uni_stuttgart.Voxie.SurfaceAttributeKind.Triangle") {
      for (uint64_t i = 0; i < dstAttr->count(); i++)
        dstAttr->copyEntryFrom(i, srcAttr, i);
      /*
      size_t bytes = dstAttr->getByteSize();
      if (bytes != srcAttr->getByteSize())
        throw Exception("de.uni_stuttgart.Voxie.InternalError",
                        "bytes != srcAttr->getByteSize()");
      memcpy(dstAttr->getSharedMemory().getData(), srcAttr->getBytes(), bytes);
      */
    } else {
      qWarning() << "makeControllingVertexUnique: Unknown triangle kind:"
                 << srcAttr->kind();
      continue;
    }
  }
  return res;
}

/*
 * @brief Reorder vertices so that the vertex ID of the controlling (last)
 * vertex is always the same as the triangle ID. The controlling vertex must be
 * unique.
 */
static QSharedPointer<vx::SurfaceDataTriangleIndexed> reorderVertices(
    const QSharedPointer<vx::SurfaceDataTriangleIndexed>& surfaceShared) {
  auto surface = surfaceShared.data();

  // Array mapping the old vertex ID to the new vertex ID
  std::vector<SurfaceDataTriangleIndexed::IndexType> newID(
      surface->vertices().size(), SurfaceDataTriangleIndexed::invalidIndex);

  // Go through all triangles and make sure that the new controlling vertex ID
  // is the triangle ID
  for (size_t i = 0; i < surface->triangles().size(); i++) {
    auto ctrl = surface->triangles()[i][2];
    if (newID[ctrl] != SurfaceDataTriangleIndexed::invalidIndex) {
      qCritical() << "reorderVertices(): Controlling vertex is not unique";
      return surfaceShared;
    }
    newID[ctrl] = i;
  }

  // Go through all vertices and give every vertex which does not have an ID a
  // new ID
  SurfaceDataTriangleIndexed::IndexType nextID = surface->triangles().size();
  for (size_t i = 0; i < surface->vertices().size(); i++) {
    if (newID[i] == SurfaceDataTriangleIndexed::invalidIndex) {
      newID[i] = nextID;
      nextID++;
    }
  }
  if (nextID != surface->vertices().size()) {
    qCritical() << "reorderVertices(): nextID != surface->vertices().size()";
    return surfaceShared;
  }

  // The new triangle values
  std::vector<SurfaceDataTriangleIndexed::Triangle> triangles(
      surface->triangles().size());

  // The new vertex values
  std::vector<QVector3D> vertices(surface->vertices().size());

  // Copy and change triangles
  for (size_t i = 0; i < triangles.size(); i++) {
    auto old = surface->triangles()[i];
    triangles[i] = {{newID[old[0]], newID[old[1]], newID[old[2]]}};
  }

  // Reorder vertices
  for (size_t i = 0; i < vertices.size(); i++) {
    vertices[newID[i]] = surface->vertices()[i];
  }

  // TODO: What should happen to attributes here?
  QList<std::tuple<QString, QString, quint64,
                   std::tuple<QString, quint32, QString>, QString,
                   QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>
      attributes;
  for (const auto& srcAttr : surface->listAttributes()) {
    if (srcAttr->kind() !=
            "de.uni_stuttgart.Voxie.SurfaceAttributeKind.Vertex" &&
        srcAttr->kind() !=
            "de.uni_stuttgart.Voxie.SurfaceAttributeKind.Triangle") {
      qWarning() << "reorderVertices: Unknown triangle kind:"
                 << srcAttr->kind();
      continue;
    }
    attributes << srcAttr->dbusData();
  }
  auto res = SurfaceDataTriangleIndexed::create(vertices, triangles, false,
                                                attributes);
  for (const auto& dstAttr : res->listAttributes()) {
    auto srcAttr = surface->getAttribute(dstAttr->name());
    if (srcAttr->kind() ==
        "de.uni_stuttgart.Voxie.SurfaceAttributeKind.Vertex") {
      for (uint64_t i = 0; i < srcAttr->count(); i++) {
        // qDebug() << "copyEntryFrom(MU)" << newID.at(i) << i;
        dstAttr->copyEntryFrom(newID.at(i), srcAttr, i);
      }
    } else if (srcAttr->kind() ==
               "de.uni_stuttgart.Voxie.SurfaceAttributeKind.Triangle") {
      for (uint64_t i = 0; i < dstAttr->count(); i++)
        dstAttr->copyEntryFrom(i, srcAttr, i);
      /*
      size_t bytes = dstAttr->getByteSize();
      if (bytes != srcAttr->getByteSize())
        throw Exception("de.uni_stuttgart.Voxie.InternalError",
                        "bytes != srcAttr->getByteSize()");
      memcpy(dstAttr->getSharedMemory().getData(), srcAttr->getBytes(), bytes);
      */
    } else {
      qWarning() << "reorderVertices: Unknown triangle kind:"
                 << srcAttr->kind();
      continue;
    }
  }
  return res;
}

const QSharedPointer<vx::SurfaceDataTriangleIndexed>&
SurfaceData3D::createSurfaceModified() {
  if (surfaceMod_) return surfaceMod_;

  /*
  // Trivial implementation which will produce 3 times as many vertices as there
  are triangles auto old = getSurface(); QScopedPointer<SurfaceBuilder> sb(new
  SurfaceBuilder()); for (size_t i = 0; i < old->triangles().size(); i++) { auto
  triangle = old->triangles()[i]; sb->addVertex(old->vertices()[triangle[0]]);
    sb->addTriangle(old->triangles().size() + i, old->triangles().size() * 2 +
  i, i);
  }
  for (size_t i = 0; i < old->triangles().size(); i++) {
    auto triangle = old->triangles()[i];
    sb->addVertex(old->vertices()[triangle[1]]);
  }
  for (size_t i = 0; i < old->triangles().size(); i++) {
    auto triangle = old->triangles()[i];
    sb->addVertex(old->vertices()[triangle[2]]);
  }
  surfaceMod_ = sb->createSurfaceClearBuilder();
  */

  // TODO: somehow reduce the number of copies of the surface created here?
  surfaceMod_ =
      reorderVertices(makeControllingVertexUnique(getSurface().data()));

  if (vx::debug_option::Log_Vis3D_CreateModifiedSurface()->enabled())
    qDebug() << "Modified surface has" << surfaceMod_->vertices().size()
             << "vertices instead of" << getSurface()->vertices().size()
             << "and" << surfaceMod_->triangles().size() << "triangles";

  return surfaceMod_;
}
