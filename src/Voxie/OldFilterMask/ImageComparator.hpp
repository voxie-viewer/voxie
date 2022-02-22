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

#include <VoxieBackend/Data/FloatImage.hpp>

#include <Voxie/OldFilterMask/Selection2DMask.hpp>
#include <Voxie/OldFilterMask/ellipseData.hpp>
#include <Voxie/OldFilterMask/polygonData.hpp>
#include <Voxie/OldFilterMask/rectangleData.hpp>

#include <VoxieBackend/OpenCL/CLInstance.hpp>

namespace vx {
namespace filter {
/**
 * This class is used by applying a filter and also applying a mask on it.
 * Therefor are 2 methods, compareImage. This method works on GPU only. But it
 * has a fallback methods, when the GPU code won't works for some problems like,
 * there is no GPU or the memory is not enough.
 * @brief The ImageComparator class is a class with the static methods for
 * applying the masks of a Filter. The methods are for GPU and a fallback method
 * for CPU.
 */
class ImageComparator {
 public:
  /**
   * @brief compareImage compares if a point is inside the mask and writes the
   * result on the filterImage. Works on GPU
   * @param sourceImage the normal image without an applied filter.
   * @param filterImage the image which a filter is applied on. Contains the
   * result in the end.
   * @param area the area where the picture is on the plane.
   * @param mask the masks of the given filter.
   */
  static void compareImage(vx::FloatImage sourceImage,
                           vx::FloatImage filterImage, QRectF area,
                           Selection2DMask* mask);

  /**
   * @brief compareImageCPU do the same as compareImage, but it is a fallback
   * when GPU won't work or do a failure.
   * @param sourceImage the normal image without an applied filter.
   * @param filterImage the image which a filter is applied on. Contains the
   * result in the end.
   * @param area the area where the picture is on the plane.
   * @param mask the masks of the given filter.
   */
  static void compareImageCPU(vx::FloatImage sourceImage,
                              vx::FloatImage filterImage, QRectF area,
                              Selection2DMask* mask);
};
}  // namespace filter
}  // namespace vx
