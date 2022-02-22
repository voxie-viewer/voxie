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

#include <VoxieClient/Map.hpp>

#include <VoxieBackend/Data/FloatImage.hpp>
#include <VoxieBackend/Data/SliceImageContext.hpp>

#include <QtCore/QLineF>
#include <QtCore/QRectF>
#include <QtCore/QSize>

namespace vx {

/**
 * SliceImage is a FloatImage that was created from a Slice, appart from the
 * imagedate it stores a SliceImageContext that holds information about the
 * details from what this image was created.
 */
class VOXIEBACKEND_EXPORT SliceImage : public FloatImage {
 public:
  /**
   * @brief SliceImage
   * @param width
   * @param height
   * @param context
   */
  // throws Exception if enableSharedMemory is true
  SliceImage(size_t width, size_t height, SliceImageContext context,
             bool enableSharedMemory)
      : FloatImage(width, height, enableSharedMemory), _context(context) {}

  /**
   * @brief SliceImage constructs an empty sliceimage
   */
  SliceImage() : FloatImage(), _context(SliceImageContext()) {}

  /**
   * constructs a sliceimage from a floatimage, the slicimagecontext
   * is assumed to have a standard plane, a planearea starting at (0,0) and
   * extending to (imageWidth, imageHeight) (in meters), and a voxelSpacing of
   * (1,1,1) (cubic meter). The Floatimages Data is cloned.
   * @param floatimg for creating sliceimage from
   */
  // throws Exception if enableSharedMemory is true
  explicit SliceImage(const FloatImage& floatimg, bool enableSharedMemory)
      : FloatImage(floatimg.clone(enableSharedMemory)),
        _context(SliceImageContext()) {
    this->_context.planeArea = QRectF(QPointF(0, 0), this->getDimension());
    this->_context.voxelGridSpacing = QVector3D(1, 1, 1);
  }

  /**
   * @return the distance of 2 points on the image in meters
   * @param p1 point one
   * @param p2 point two
   */
  float distanceInMeter(const QPoint& p1, const QPoint& p2) const;

  /**
   * @return the point on the plane this image was created on corresponding
   * to a pixel point in the image.
   * @param pixelpoint point on image
   */
  QPointF pixelToPlanePoint(const QPoint& pixelpoint, bool invertYAxis) const;

  /**
   * @brief planePointToPixel turns a point on the plane to the corresponding
   * pixel in the image
   * @param planePoint
   * @return The Pixelpoint on image
   */
  QPointF planePointToPixel(const QPointF planePoint);

  /**
   * @return image's context
   */
  const SliceImageContext& context() const { return this->_context; }

  static void imagePoint2PlanePoint(ptrdiff_t x, ptrdiff_t y,
                                    const QSize& imgSize,
                                    const QRectF& planeArea,
                                    QPointF& targetPoint, bool invertYAxis);

  /**
   * @brief Calculates Pixel to Plane [m] coordinate transformation
   * @return transformation matrix
   */
  AffineMap<double, 2UL, 2UL> getPixelToPlaneTrafo(bool invertYAxis);

  /**
   * @return clone of this SliceImage
   */
  SliceImage clone(bool enableSharedMemory = false) const;

 private:
  SliceImageContext _context;
};

}  // namespace vx

Q_DECLARE_METATYPE(vx::SliceImage)
