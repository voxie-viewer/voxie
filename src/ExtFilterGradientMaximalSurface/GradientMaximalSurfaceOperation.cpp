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

#include "GradientMaximalSurfaceOperation.hpp"
#include <QtConcurrent>

GradientMaximalSurfaceOperation::GradientMaximalSurfaceOperation(
    vx::Array3<const float>& inputVolume, vx::Array2<float>& inputVertices,
    const QVector3D volumeOrigin, const QVector3D gridSpacing,
    const uint samplingPointCount, const float samplingDistance)
    : vertices(inputVertices),
      gradientCache(inputVertices.size<0>() * samplingPointCount),
      voxels(inputVolume) {
  this->volumeOrigin = volumeOrigin;
  this->gridSpacing = gridSpacing;
  this->samplingDistance = samplingDistance;
  this->samplingPointCount = samplingPointCount;
}

/**
 * @brief move vertices to position of maximal gradient
 */
void GradientMaximalSurfaceOperation::run(
    vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationRunFilter>&
        op) {
  // Go through all vertices of the surface and search the point with the
  // highest gradient norm on the normal of the vertice. Then move the vertice
  // to that point.

  size_t vertexCount = vertices.size<0>();

  // create list of vertices so that we can run QtConcurrent::map on it
  QList<QVector3D> verts;
  for (size_t i = 0; i < vertexCount; ++i) {
    verts.append(QVector3D(vertices(i, 0), vertices(i, 1), vertices(i, 2)));
  }

  // QFutureWatcher provides progress reports
  QFutureWatcher<void> watcher;
  QFutureWatcher<void>::connect(
      &watcher, &QFutureWatcher<void>::progressValueChanged,
      [&op, &watcher](int value) {
        HANDLEDBUSPENDINGREPLY(op.opGen().SetProgress(
            (double)(value - watcher.progressMinimum()) /
                (watcher.progressMaximum() - watcher.progressMinimum()),
            vx::emptyOptions()));
      });

  // we need an event loop so that the program pauses until the calculation is
  // finished but still handles events
  QEventLoop eventLoop;
  // stop the event loop when the calculation is done
  QObject::connect(&watcher, &QFutureWatcher<void>::finished, &eventLoop,
                   &QEventLoop::quit);
  watcher.setFuture(QtConcurrent::map(verts, [=](QVector3D& vert) {
    QVector3D voxelVert = surfaceSpaceToVoxelSpace(vert);

    // don't modify vertices that are outside the voxel grid; just skip them
    if (voxelVert.x() < 0 || voxelVert.y() < 0 || voxelVert.z() < 0 ||
        voxelVert.x() + 1 >= voxels.size<0>() ||
        voxelVert.y() + 1 >= voxels.size<1>() ||
        voxelVert.z() + 1 >= voxels.size<2>())
      return;

    // get the unit gradient at the vertex position. We will sample in the
    // direction of the gradient
    QVector3D unitGradient = getGradientTrilinear(voxelVert).normalized();

    // get the point along the gradient vector with the largest gradient norm.
    // We sample from -samplingDistance to +samplingDistance
    QVector3D maximalGradientPoint =
        findMaximalGradient(voxelVert - unitGradient * samplingDistance,
                            voxelVert + unitGradient * samplingDistance);

    vert = voxelSpaceToSurfaceSpace(maximalGradientPoint);
  }));

  eventLoop.exec();

  // copy back the vertices' positions from the list to the array2
  for (size_t i = 0; i < vertexCount; ++i) {
    vertices(i, 0) = verts[i].x();
    vertices(i, 1) = verts[i].y();
    vertices(i, 2) = verts[i].z();
  }
}

/**
 * @brief calculate maximal gradient of between start and end
 * @param start starting position
 * @param end ending position
 * @return QVector3D of maximal gradient
 */
QVector3D GradientMaximalSurfaceOperation::findMaximalGradient(QVector3D start,
                                                               QVector3D end) {
  // difference between the start and end point
  QVector3D diff = end - start;
  // difference between two sample points
  QVector3D sampleDiff = diff / (samplingPointCount - 1);

  QVector3D maxGradientPoint;
  float maxGradientNorm = std::numeric_limits<float>::quiet_NaN();

  // check the gradient norm at each sampling point
  for (size_t i = 0; i < samplingPointCount; ++i) {
    QVector3D samplePoint = start + sampleDiff * i;

    // skip sample points that are outside the voxel grid
    if (samplePoint.x() < 0 || samplePoint.y() < 0 || samplePoint.z() < 0 ||
        samplePoint.x() + 1 >= voxels.size<0>() ||
        samplePoint.y() + 1 >= voxels.size<1>() ||
        samplePoint.z() + 1 >= voxels.size<2>())
      continue;

    // get the gradient norm at the sample point
    QVector3D gradient = getGradientTrilinear(samplePoint);
    float newNorm = gradient.x() * gradient.x() + gradient.y() * gradient.y() +
                    gradient.z() + gradient.z();

    // if this is the first norm (maxGradientNorm is NaN) or this new norm is
    // larger than the largest we have found until now, then save the point of
    // this new norm
    if (maxGradientNorm != maxGradientNorm || newNorm > maxGradientNorm) {
      maxGradientNorm = newNorm;
      maxGradientPoint = samplePoint;
    }
  }

  return maxGradientPoint;
}

QVector3D GradientMaximalSurfaceOperation::surfaceSpaceToVoxelSpace(
    const QVector3D value) {
  return (value - volumeOrigin) / gridSpacing;
}

QVector3D GradientMaximalSurfaceOperation::voxelSpaceToSurfaceSpace(
    const QVector3D value) {
  return volumeOrigin + gridSpacing * value;
}

QVector3D GradientMaximalSurfaceOperation::computeGradient(
    vx::Array3<const float>& voxels, size_t x, size_t y, size_t z) {
  float gradientX = 0.0f;
  float gradientY = 0.0f;
  float gradientZ = 0.0f;

  // Corners
  if (x + 1 < voxels.size<0>() && y + 1 < voxels.size<1>() &&
      z + 1 < voxels.size<2>()) {
    gradientX += voxels(x + 1, y + 1, z + 1);
    gradientY += voxels(x + 1, y + 1, z + 1);
    gradientZ += voxels(x + 1, y + 1, z + 1);
  }

  if (x + 1 < voxels.size<0>() && y + 1 < voxels.size<1>() && z > 0) {
    gradientX += voxels(x + 1, y + 1, z - 1);
    gradientY += voxels(x + 1, y + 1, z - 1);
    gradientZ -= voxels(x + 1, y + 1, z - 1);
  }

  if (x + 1 < voxels.size<0>() && y > 0 && z + 1 < voxels.size<2>()) {
    gradientX += voxels(x + 1, y - 1, z + 1);
    gradientY -= voxels(x + 1, y - 1, z + 1);
    gradientZ += voxels(x + 1, y - 1, z + 1);
  }

  if (x + 1 < voxels.size<0>() && y > 0 && z > 0) {
    gradientX += voxels(x + 1, y - 1, z - 1);
    gradientY -= voxels(x + 1, y - 1, z - 1);
    gradientZ -= voxels(x + 1, y - 1, z - 1);
  }

  if (x > 0 && y + 1 < voxels.size<1>() && z + 1 < voxels.size<2>()) {
    gradientX -= voxels(x - 1, y + 1, z + 1);
    gradientY += voxels(x - 1, y + 1, z + 1);
    gradientZ += voxels(x - 1, y + 1, z + 1);
  }

  if (x > 0 && y + 1 < voxels.size<1>() && z > 0) {
    gradientX -= voxels(x - 1, y + 1, z - 1);
    gradientY += voxels(x - 1, y + 1, z - 1);
    gradientZ -= voxels(x - 1, y + 1, z - 1);
  }

  if (x > 0 && y > 0 && z + 1 < voxels.size<2>()) {
    gradientX -= voxels(x - 1, y - 1, z + 1);
    gradientY -= voxels(x - 1, y - 1, z + 1);
    gradientZ += voxels(x - 1, y - 1, z + 1);
  }

  if (x > 0 && y > 0 && z > 0) {
    gradientX -= voxels(x - 1, y - 1, z - 1);
    gradientY -= voxels(x - 1, y - 1, z - 1);
    gradientZ -= voxels(x - 1, y - 1, z - 1);
  }

  // Edges
  if (x + 1 < voxels.size<0>() && y + 1 < voxels.size<1>()) {
    gradientX += voxels(x + 1, y + 1, z) * 2;
    gradientY += voxels(x + 1, y + 1, z) * 2;
  }

  if (x + 1 < voxels.size<0>() && y > 0) {
    gradientX += voxels(x + 1, y - 1, z) * 2;
    gradientY -= voxels(x + 1, y - 1, z) * 2;
  }

  if (x + 1 < voxels.size<0>() && z + 1 < voxels.size<2>()) {
    gradientX += voxels(x + 1, y, z + 1) * 2;
    gradientZ += voxels(x + 1, y, z + 1) * 2;
  }

  if (x + 1 < voxels.size<0>() && z > 0) {
    gradientX += voxels(x + 1, y, z - 1) * 2;
    gradientZ -= voxels(x + 1, y, z - 1) * 2;
  }

  if (x > 0 && y + 1 < voxels.size<1>()) {
    gradientX -= voxels(x - 1, y + 1, z) * 2;
    gradientY += voxels(x - 1, y + 1, z) * 2;
  }

  if (x > 0 && y > 0) {
    gradientX -= voxels(x - 1, y - 1, z) * 2;
    gradientY -= voxels(x - 1, y - 1, z) * 2;
  }

  if (x > 0 && z + 1 < voxels.size<2>()) {
    gradientX -= voxels(x - 1, y, z + 1) * 2;
    gradientZ += voxels(x - 1, y, z + 1) * 2;
  }

  if (x > 0 && z > 0) {
    gradientX -= voxels(x - 1, y, z - 1) * 2;
    gradientZ -= voxels(x - 1, y, z - 1) * 2;
  }

  if (y + 1 < voxels.size<1>() && z + 1 < voxels.size<2>()) {
    gradientY += voxels(x, y + 1, z + 1) * 2;
    gradientZ += voxels(x, y + 1, z + 1) * 2;
  }

  if (y + 1 < voxels.size<1>() && z > 0) {
    gradientY += voxels(x, y + 1, z - 1) * 2;
    gradientZ -= voxels(x, y + 1, z - 1) * 2;
  }

  if (y > 0 && z + 1 < voxels.size<2>()) {
    gradientY -= voxels(x, y - 1, z + 1) * 2;
    gradientZ += voxels(x, y - 1, z + 1) * 2;
  }

  if (y > 0 && z > 0) {
    gradientY -= voxels(x, y - 1, z - 1) * 2;
    gradientZ -= voxels(x, y - 1, z - 1) * 2;
  }

  // Direct Neighbours
  if (x + 1 < voxels.size<0>()) {
    gradientX += voxels(x + 1, y, z) * 4;
  }

  if (x > 0) {
    gradientX -= voxels(x - 1, y, z) * 4;
  }

  if (y + 1 < voxels.size<1>()) {
    gradientY += voxels(x, y + 1, z) * 4;
  }

  if (y > 0) {
    gradientY -= voxels(x, y - 1, z) * 4;
  }

  if (z + 1 < voxels.size<2>()) {
    gradientZ += voxels(x, y, z + 1) * 4;
  }

  if (z > 0) {
    gradientZ -= voxels(x, y, z - 1) * 4;
  }

  return QVector3D(gradientX, gradientY, gradientZ);
}

QVector3D GradientMaximalSurfaceOperation::getGradientTrilinear(QVector3D pos) {
  // The fractional part (part after the .) of the position coordinate
  QVector3D fracPos(pos.x() - (qint64)pos.x(), pos.y() - (qint64)pos.y(),
                    pos.z() - (qint64)pos.z());

  // trilinear interpolation
  return getOrCalcGradient(pos.x(), pos.y(), pos.z()) * (1 - fracPos.x()) +
         getOrCalcGradient(pos.x() + 1, pos.y(), pos.z()) * fracPos.x() *
             (1 - fracPos.y()) +
         getOrCalcGradient(pos.x(), pos.y() + 1, pos.z()) * (1 - fracPos.x()) +
         getOrCalcGradient(pos.x() + 1, pos.y() + 1, pos.z()) * fracPos.x() *
             fracPos.y() * (1 - fracPos.z()) +
         getOrCalcGradient(pos.x(), pos.y(), pos.z() + 1) * (1 - fracPos.x()) +
         getOrCalcGradient(pos.x() + 1, pos.y(), pos.z() + 1) * fracPos.x() *
             (1 - fracPos.y()) +
         getOrCalcGradient(pos.x(), pos.y() + 1, pos.z() + 1) *
             (1 - fracPos.x()) +
         getOrCalcGradient(pos.x() + 1, pos.y() + 1, pos.z() + 1) *
             fracPos.x() * fracPos.y() * fracPos.z();
}

QVector3D GradientMaximalSurfaceOperation::getOrCalcGradient(size_t x, size_t y,
                                                             size_t z) {
  // we use locks to guarantee that the unordered_map can be read by multiple
  // threads at the same time but only one thread can write at once and also
  // that no thread is reading while a thread is writing
  Point3D point(x, y, z);
  std::shared_lock<std::shared_timed_mutex> readLock(mutex);
  std::unordered_map<Point3D, QVector3D, Point3D::Hasher>::const_iterator
      gradientIt = gradientCache.find(point);

  // if we do not find the point that we need the gradient of in the
  // unordered_map
  if (gradientIt == gradientCache.end()) {
    // then calculate the gradient for that point and save it in the
    // unordered_map
    readLock.unlock();
    QVector3D gradient = computeGradient(voxels, x, y, z);

    std::unique_lock<std::shared_timed_mutex> lock(mutex);
    gradientCache.insert({{x, y, z}, gradient});
    lock.unlock();
    return gradient;
  } else {
    // if we do find it in the map then just return it
    QVector3D gradient = gradientIt->second;
    readLock.unlock();
    return gradient;
  }
}
