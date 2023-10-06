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

#include "Cuberille.hpp"

#include <Voxie/Data/SurfaceBuilder.hpp>

#include <VoxieBackend/Data/VolumeDataVoxel.hpp>
#include <VoxieBackend/Data/VolumeDataVoxelInst.hpp>

#include <VoxieBackend/IO/Operation.hpp>

#include <QtCore/QPointer>

using namespace vx;

Cuberille::Cuberille(QObject* parent) : SurfaceExtractor(parent) {}
Cuberille::~Cuberille() {}

namespace {
// Caches the vertex indices for all the vertices for 1 plane
//
// This is used to make sure that there are no duplicate vertices in
// the resulting surface
class VertexCache {
  size_t dimX, dimY;
  size_t planeCount;
  QVector<SurfaceDataTriangleIndexed::IndexType> plane1;
  QVector<SurfaceDataTriangleIndexed::IndexType> plane2;
  SurfaceDataTriangleIndexed::IndexType* planeLow;
  SurfaceDataTriangleIndexed::IndexType* planeUpp;

 public:
  VertexCache(size_t dimX, size_t dimY)
      : dimX(dimX),
        dimY(dimY),
        planeCount((dimX + 1) * (dimY + 1)),
        plane1(planeCount),
        plane2(planeCount) {
    planeLow = plane1.data();
    planeUpp = plane2.data();

    memset(planeUpp, -1,
           planeCount * sizeof(SurfaceDataTriangleIndexed::IndexType));
  }

  void nextPlane() {
    std::swap(planeLow, planeUpp);
    memset(planeUpp, -1,
           planeCount * sizeof(SurfaceDataTriangleIndexed::IndexType));
  }

  SurfaceDataTriangleIndexed::IndexType& get(size_t x, size_t y, size_t z) {
    // qDebug() << x << (dimX+1) << y << (dimY+1) << z;
    if (x >= (dimX + 1) || y >= (dimY + 1) || z >= 2) abort();

    SurfaceDataTriangleIndexed::IndexType* ptr = z ? planeUpp : planeLow;

    return ptr[x + (dimX + 1) * y];
  }
};
}  // namespace

// Return a vertex at (x, y, zBase+z)
static SurfaceDataTriangleIndexed::IndexType addVertex(SurfaceBuilder* sb,
                                                       VertexCache& cache,
                                                       VolumeDataVoxel* data,
                                                       size_t x, size_t y,
                                                       size_t z, size_t zBase) {
  SurfaceDataTriangleIndexed::IndexType& cacheItem = cache.get(x, y, z);
  if (cacheItem != SurfaceDataTriangleIndexed::invalidIndex) return cacheItem;

  QVector3D pos(x, y, zBase + z);
  pos = data->origin() + data->getSpacing() * pos;
  return cacheItem = sb->addVertex(pos);
}

static void addQuad(SurfaceBuilder* sb, VertexCache& cache,
                    VolumeDataVoxel* data, vx::VectorSizeT3 pos,
                    const std::array<int, 3>& a, const std::array<int, 3>& b,
                    const std::array<int, 3>& c, const std::array<int, 3>& d) {
  size_t x = pos.x;
  size_t y = pos.y;
  size_t z = pos.z;
  auto va = addVertex(sb, cache, data, x + a[0], y + a[1], a[2], z);
  auto vb = addVertex(sb, cache, data, x + b[0], y + b[1], b[2], z);
  auto vc = addVertex(sb, cache, data, x + c[0], y + c[1], c[2], z);
  auto vd = addVertex(sb, cache, data, x + d[0], y + d[1], d[2], z);

  sb->addTriangle(va, vb, vc);
  sb->addTriangle(va, vc, vd);
}

static void genCube(const vx::VectorSizeT3& position, int sides,
                    SurfaceBuilder* sb, VertexCache& cache,
                    VolumeDataVoxel* data) {
  // Front (+z)
  if (sides & 1) {
    addQuad(sb, cache, data, position, {{0, 0, 1}}, {{1, 0, 1}}, {{1, 1, 1}},
            {{0, 1, 1}});
  }

  // Back (-z)
  if (sides & 2) {
    addQuad(sb, cache, data, position, {{0, 0, 0}}, {{0, 1, 0}}, {{1, 1, 0}},
            {{1, 0, 0}});
  }

  // Left (+x)
  if (sides & 4) {
    addQuad(sb, cache, data, position, {{1, 0, 0}}, {{1, 1, 0}}, {{1, 1, 1}},
            {{1, 0, 1}});
  }

  // Right (-x)
  if (sides & 8) {
    addQuad(sb, cache, data, position, {{0, 0, 0}}, {{0, 0, 1}}, {{0, 1, 1}},
            {{0, 1, 0}});
  }

  // Top (+y)
  if (sides & 16) {
    addQuad(sb, cache, data, position, {{0, 1, 0}}, {{0, 1, 1}}, {{1, 1, 1}},
            {{1, 1, 0}});
  }

  // Bottom (-y)
  if (sides & 32) {
    addQuad(sb, cache, data, position, {{0, 0, 0}}, {{1, 0, 0}}, {{1, 0, 1}},
            {{0, 0, 1}});
  }
}

QSharedPointer<SurfaceDataTriangleIndexed> Cuberille::extract(
    const QSharedPointer<vx::io::Operation>& operation_,
    vx::VolumeDataVoxel* notGenericData, vx::VolumeDataVoxel*, float threshold,
    bool invert) {
  return notGenericData->performInGenericContext([operation_, threshold,
                                                  invert](auto& data) {
    auto operation = operation_.data();

    auto dim = data.getDimensions();

    auto upper = dim;
    upper.x -= 1;
    upper.y -= 1;
    upper.z -= 1;

    QScopedPointer<SurfaceBuilder> sb(new SurfaceBuilder());

    VertexCache cache(dim.x, dim.y);

    for (size_t z = 0; z < dim.z; z++) {
      cache.nextPlane();

      for (size_t y = 0; y < dim.y; y++) {
        for (size_t x = 0; x < dim.x; x++) {
          operation->throwIfCancelled();

          auto voxel = data.getVoxel(x, y, z);
          if ((voxel < threshold) ^ invert) continue;

          int majoraMask = 0xFF;
          if ((x > 0) && ((data.getVoxel(x - 1, y, z) >= threshold) ^ invert))
            majoraMask &= ~8;
          if ((y > 0) && ((data.getVoxel(x, y - 1, z) >= threshold) ^ invert))
            majoraMask &= ~32;
          if ((z > 0) && ((data.getVoxel(x, y, z - 1) >= threshold) ^ invert))
            majoraMask &= ~2;
          if ((x < upper.x) &&
              ((data.getVoxel(x + 1, y, z) >= threshold) ^ invert))
            majoraMask &= ~4;
          if ((y < upper.y) &&
              ((data.getVoxel(x, y + 1, z) >= threshold) ^ invert))
            majoraMask &= ~16;
          if ((z < upper.z) &&
              ((data.getVoxel(x, y, z + 1) >= threshold) ^ invert))
            majoraMask &= ~1;

          if ((majoraMask & 192) != 0) {
            genCube(vx::VectorSizeT3(x, y, z), majoraMask, sb.data(), cache,
                    &data);
          }
        }
        operation->updateProgress(1.0f * z / dim.z);
      }
    }

    return sb->createSurfaceClearBuilder();
  });
}
