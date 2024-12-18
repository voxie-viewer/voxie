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

#include <VoxieBackend/DebugOptions.hpp>

#include <VoxieBackend/Data/FloatImage.hpp>
#include <VoxieBackend/Data/InterpolationMethod.hpp>
#include <VoxieBackend/Data/PlaneInfo.hpp>
#include <VoxieBackend/Data/SliceImage.hpp>
#include <VoxieBackend/Data/VoxelAccessor.hpp>

#include <VoxieClient/Exception.hpp>
#include <VoxieClient/QtUtil.hpp>
#include <VoxieClient/RunParallel.hpp>

namespace vx {

template <typename Accessor, typename F>
void withInterpolation(const Accessor& accessor, vx::InterpolationMethod method,
                       const F& f) {
  if (method == vx::InterpolationMethod::NearestNeighbor)
    f(nearestInterpolation(accessor));
  else if (method == vx::InterpolationMethod::Linear)
    f(linearInterpolation(accessor));
  else
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "Invalid interpolation method");
}

// TODO: Provide Task and CancellationToken parameter?
template <typename F>
void runParallelExtractSlice(size_t items, const F& f) {
  // Non-parallel
  if (!vx::debug_option::ExtractSlice_UseMultiThreading()->get()) {
    f([&](const auto& f2) {
      for (size_t i = 0; i < items; i++) {
        f2(i);
      }
    });
    return;
  }

  if (vx::debug_option::ExtractSlice_UseStaticScheduling()->get()) {
    runParallelStaticPrepare(nullptr, nullptr, items, f);
  } else {
    runParallelDynamicPrepare(nullptr, nullptr, items, f);
  }
}

template <typename Data>
void extractSliceCpu(Data& data, const QVector3D& origin1,
                     const QQuaternion& rotation, const QSize& outputSize,
                     double pixelSizeX, double pixelSizeY,
                     InterpolationMethod method, FloatImage& outputImage) {
  PlaneInfo plane(origin1, rotation);
  QRectF sliceArea(0, 0, outputSize.width() * pixelSizeX,
                   outputSize.height() * pixelSizeY);

  if ((size_t)outputSize.width() > outputImage.getWidth() ||
      (size_t)outputSize.height() > outputImage.getHeight())
    throw vx::Exception("de.uni_stuttgart.Voxie.IndexOutOfRange",
                        "Index is out of range");

  if (outputImage.getMode() != SliceImage::STDMEMORY_MODE) {
    outputImage.switchMode(false);  // switch mode without syncing memory
  }
  FloatBuffer buffer = outputImage.getBuffer();

  runParallelExtractSlice(outputSize.height(), [&](const auto& cb) {
    auto accessor = data.accessor();
    // TODO: Always convert to float? Also convert to float for nearest?
    auto accessorConv = convertedVoxelAccessor<float>(accessor);

    withInterpolation(accessorConv, method, [&](const auto& accessorInterpol) {
      // for (size_t y = 0; y < (size_t)outputSize.height(); y++) {
      // qDebug() << y_min << y_max;
      // for (size_t y = y_min; y < y_max; y++) {
      cb([&](size_t y) {
        for (size_t x = 0; x < (size_t)outputSize.width(); x++) {
          // TODO: Use vx::Vector<>
          QPointF planePoint;
          SliceImage::imagePoint2PlanePoint(x, y, outputSize, sliceArea,
                                            planePoint, false);
          QVector3D volumePoint =
              plane.get3DPoint(planePoint.x(), planePoint.y());
          buffer[y * outputImage.getWidth() + x] =
              accessorInterpol
                  .getVoxelInterpolatedObject(
                      vectorCast<double>(toVector(volumePoint)))
                  .value_or(std::numeric_limits<float>::quiet_NaN());
        }
      });
      //}
      // qDebug() << y_min << y_max << "done";
    });
  });
}

}  // namespace vx
