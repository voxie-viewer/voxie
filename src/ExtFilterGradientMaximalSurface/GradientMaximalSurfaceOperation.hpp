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

#include <VoxieClient/Array.hpp>
#include <VoxieClient/ClaimedOperation.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <future>
#include <iostream>
#include <list>
#include <numeric>
#include <shared_mutex>
#include <sstream>
#include <unordered_map>
#include <vector>

#include <VoxieBackend/Data/SurfaceData.hpp>

/**
 * @brief This class contains the code that actually does the calculations of
 * the maximal gradient filter.
 */
class GradientMaximalSurfaceOperation {
 public:
  GradientMaximalSurfaceOperation(vx::Array3<const float>& inputVolume,
                                  vx::Array2<float>& inputVertices,
                                  const QVector3D volumeOrigin,
                                  const QVector3D gridSpacing,
                                  const uint samplingPointCount,
                                  const float samplingDistance);

  void run(vx::ClaimedOperation<
           de::uni_stuttgart::Voxie::ExternalOperationRunFilter>& op);

 private:
  struct Point3D {
    size_t x;
    size_t y;
    size_t z;

    Point3D(size_t x, size_t y, size_t z) {
      this->x = x;
      this->y = y;
      this->z = z;
    }

    bool operator==(const Point3D other) const {
      return x == other.x && y == other.y && z == other.z;
    }

    /**
     * @brief Hasher for the Point3D class.
     */
    class Hasher {
     public:
      size_t operator()(const Point3D& point) const noexcept {
        std::hash<size_t> hasher;

        // this is how hashes are combined by the hash_combine function in the
        // boost library so this should provide a fairly good implementation
        size_t hash = hasher(point.x) + 0x9e3779b9;
        hash = hasher(point.y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash = hasher(point.z) + 0x9e3779b9 + (hash << 6) + (hash >> 2);

        return hash;
      }
    };
  };
  /**
   * @brief computeGradient Computes the gradient of a voxel in the voxels array
   * located at x, y, z.
   * @param voxels The voxel volume
   * @param x x location of voxel to calc the gradient for
   * @param y y location of voxel to calc the gradient for
   * @param z z location of voxel to calc the gradient for
   * @return 3D Vector containing the gradient
   */
  static QVector3D computeGradient(vx::Array3<const float>& voxels, size_t x,
                                   size_t y, size_t z);
  /**
   * @brief getGradientTrilinear Calculates the gradient at an arbitrary point
   * by using trilinear interpolation.
   * @param position Position to calculate the gradient for
   * @return 3D vector containing the gradient
   */
  QVector3D getGradientTrilinear(QVector3D position);
  /**
   * @brief getOrCalcGradient Helper method. Gets the gradient of a voxel. If
   * the gradient is already saved in the gradients array it will take it from
   * there, otherwise it will calculate it and save it there.
   * @param x The x location of the voxel
   * @param y The y location of the voxel
   * @param z The z location of the voxel
   * @return 3D vector containing the gradient
   */
  QVector3D getOrCalcGradient(size_t x, size_t y, size_t z);
  /**
   * @brief findMaximalGradient Finds the point of maximal gradient on a line
   * going from the specified start to end point.
   * @param start The start point.
   * @param end The end point.
   * @return 3D vector withe the coordinates of the point that has the largest
   * gradient norm along the line.
   */
  QVector3D findMaximalGradient(const QVector3D start, const QVector3D end);
  /**
   * @brief surfaceSpaceToVoxelSpace Converts surface coord space to voxel coord
   * space.
   * @param value The coordinate in surface space
   * @return The coordinate in voxel space
   */
  QVector3D surfaceSpaceToVoxelSpace(const QVector3D value);
  /**
   * @brief voxelSpaceToSurfaceSpace Converts voxel coord space to surface coord
   * space.
   * @param value The coordinate in voxel space.
   * @return The coordinate in surface space.
   */
  QVector3D voxelSpaceToSurfaceSpace(const QVector3D value);

  /**
   * @brief vertices Contains the vertices of the surface that we want to
   * modify.
   */
  vx::Array2<float>& vertices;

  /**
   * @brief gradients Used to cache already calculated gradient values for
   * voxels.
   */
  std::unordered_map<Point3D, QVector3D, Point3D::Hasher> gradientCache;
  /**
   * @brief mutex Mutex to provide locks for the gradientCache.
   */
  std::shared_timed_mutex mutex;

  /**
   * @brief voxels Contains the voxels of the volume that we use as input data.
   */
  vx::Array3<const float>& voxels;

  /**
   * @brief volumeOrigin Origin of the volume used to convert between surface
   * and voxel coordinates.
   */
  QVector3D volumeOrigin;
  /**
   * @brief gridSpacing Grid spacing of the volume used to convert between
   * surface and voxel coordinates.
   */
  QVector3D gridSpacing;

  /**
   * @brief samplingPointCount Sampling point count parameter.
   */
  uint samplingPointCount;
  /**
   * @brief samplingDistance Sampling distance parameter.
   */
  float samplingDistance;
};
