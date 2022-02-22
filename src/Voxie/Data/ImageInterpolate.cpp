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

#include "ImageInterpolate.hpp"

#include <VoxieBackend/Data/ImageDataPixel.hpp>
#include <VoxieBackend/Data/ImageDataPixelInst.hpp>

// TODO: support for other data types / multiple components?

using namespace vx;

template <typename T>
static inline T getPixelOr(const vx::Array2<std::array<T, 1>>& array, qint64 x,
                           qint64 y, T defaultValue) {
  if (x < 0 || (size_t)x >= array.template size<0>() || y < 0 ||
      (size_t)y >= array.template size<1>())
    return defaultValue;
  return std::get<0>(array(x, y));
}

void vx::imageInterpolate(const QSharedPointer<ImageDataPixel>& inputImage,
                          QRectF inputSize, QRectF areaToExtract,
                          const QSharedPointer<ImageDataPixel>& outputImage,
                          QRect outputPos,
                          vx::InterpolationMethod interpolation) {
  if (areaToExtract.width() == 0 || areaToExtract.height() == 0)
    qWarning() << "vx::imageInterpolate(): Attempt to interpolate data with "
                  "zero-sized areaToExtract";

  auto inputImageCasted =
      qSharedPointerDynamicCast<ImageDataPixelInst<float, 1>>(inputImage);
  if (!inputImageCasted)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Unsupported image type for imageInterpolate()");
  const auto& array = inputImageCasted->array();

  auto outputImageCasted =
      qSharedPointerDynamicCast<ImageDataPixelInst<float, 1>>(outputImage);
  if (!outputImageCasted)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Unsupported image type for imageInterpolate()");
  const auto& outputArray = outputImageCasted->array();

  QSize dataSize(inputImageCasted->width(), inputImageCasted->height());

  for (qint64 yOut = 0; yOut < outputPos.height(); yOut++) {
    for (qint64 xOut = 0; xOut < outputPos.width(); xOut++) {
      // TODO: performance of these calculations? (matrix?)
      float xPos = (xOut + 0.5f) / outputPos.width() * areaToExtract.width() +
                   areaToExtract.left();
      float yPos = (yOut + 0.5f) / outputPos.height() * areaToExtract.height() +
                   areaToExtract.top();

      float xIn =
          (xPos - inputSize.left()) / inputSize.width() * dataSize.width();
      float yIn =
          (yPos - inputSize.top()) / inputSize.height() * dataSize.height();

      if (xIn <= -1 || xIn >= dataSize.width() || yIn <= -1 ||
          yIn >= dataSize.height()) {
        outputArray(xOut, yOut) = {NAN};
      } else {
        if (interpolation == vx::InterpolationMethod::NearestNeighbor) {
          qint64 x = (qint64)(xIn + 0.5);
          qint64 y = (qint64)(yIn + 0.5);
          float value = getPixelOr<float>(array, x, y, NAN);
          outputArray(xOut, yOut) = {value};
        } else if (interpolation == vx::InterpolationMethod::Linear) {
          // Bilinear interpolation
          // TODO: Check whether this bilinear interpolation treats corner cases
          // the same as the 3D interpolation

          qint64 x0 = (qint64)xIn;
          qint64 x1 = x0 + 1;
          float xAlpha = xIn - x0;
          qint64 y0 = (qint64)yIn;
          qint64 y1 = y0 + 1;
          float yAlpha = yIn - y0;

          float value =
              (1 - xAlpha) * (1 - yAlpha) *
                  getPixelOr<float>(array, x0, y0, 0) +
              xAlpha * (1 - yAlpha) * getPixelOr<float>(array, x1, y0, 0) +
              (1 - xAlpha) * yAlpha * getPixelOr<float>(array, x0, y1, 0) +
              xAlpha * yAlpha * getPixelOr<float>(array, x1, y1, 0);
          outputArray(xOut, yOut) = {value};
        } else {
          throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                              "Unkown interpolation value");
        }
      }
    }
  }
}

QSharedPointer<ImageDataPixel> vx::imageInterpolate(
    const QSharedPointer<ImageDataPixel>& inputImage, QRectF inputSize,
    QRectF areaToExtract, QSize outputSize,
    vx::InterpolationMethod interpolation) {
  auto outputImage = ImageDataPixel::createInst(
      outputSize.width(), outputSize.height(), 1, DataType::Float32, false);
  imageInterpolate(inputImage, inputSize, areaToExtract, outputImage,
                   QRect(QPoint(0, 0), outputSize), interpolation);
  return outputImage;
}
