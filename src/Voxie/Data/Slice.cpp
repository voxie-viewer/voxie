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

#include "Slice.hpp"

#include <Voxie/Gui/ErrorMessage.hpp>

#include <VoxieBackend/Data/PlaneInfo.hpp>
#include <VoxieBackend/Data/SliceImage.hpp>
#include <VoxieBackend/Data/VolumeDataVoxel.hpp>
#include <VoxieBackend/Data/VolumeDataVoxelInst.hpp>

using namespace vx;

SliceImage vx::generateSliceImage(VolumeData* data, const vx::PlaneInfo& plane,
                                  const QRectF& sliceArea,
                                  const QSize& imageSize,
                                  InterpolationMethod interpolation) {
  auto voxelData = dynamic_cast<VolumeDataVoxel*>(data);
  QVector3D spacing;
  if (voxelData)
    spacing = voxelData->getSpacing();
  else
    // TODO: Non-voxel datasets?
    spacing = QVector3D(1, 1, 1);

  SliceImageContext imgContext = {plane, sliceArea, spacing};

  SliceImage img((size_t)imageSize.width(), (size_t)imageSize.height(),
                 imgContext, false);

  QVector3D origin = plane.origin;
  origin += plane.tangent() * sliceArea.x();
  origin += plane.cotangent() * sliceArea.y();

  data->extractSlice(
      origin, plane.rotation, imageSize, sliceArea.width() / imageSize.width(),
      sliceArea.height() / imageSize.height(), interpolation, img);

  return img;
}
