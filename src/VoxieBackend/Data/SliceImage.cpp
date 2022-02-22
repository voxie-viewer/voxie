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

#include "SliceImage.hpp"

using namespace vx;

float SliceImage::distanceInMeter(const QPoint& p1, const QPoint& p2) const {
  // Value for invertYAxis does not matter for distanceInMeter(), use false
  return QLineF(pixelToPlanePoint(p1, false), pixelToPlanePoint(p2, false))
      .length();
}

void SliceImage::imagePoint2PlanePoint(ptrdiff_t x, ptrdiff_t y,
                                       const QSize& imgSize,
                                       const QRectF& planeArea,
                                       QPointF& targetPoint, bool invertYAxis) {
  qreal relX = (x + 0.5) / (imgSize.width() * 1.0);
  qreal relY = (y + 0.5) / (imgSize.height() * 1.0);

  targetPoint.setX(relX * planeArea.width() + planeArea.left());
  if (invertYAxis)
    targetPoint.setY(-relY * planeArea.height() + planeArea.bottom());
  else
    targetPoint.setY(relY * planeArea.height() + planeArea.top());
}

QPointF SliceImage::pixelToPlanePoint(const QPoint& pixelpoint,
                                      bool invertYAxis) const {
  QPointF p;
  imagePoint2PlanePoint(pixelpoint.x(), pixelpoint.y(), this->getDimension(),
                        this->context().planeArea, p, invertYAxis);
  return p;
}

QPointF SliceImage::planePointToPixel(const QPointF planePoint) {
  QPoint pixel;
  pixel.setY(-(planePoint.y() - this->context().planeArea.bottom()) *
             this->getDimension().height() /
             (this->context().planeArea.height()));
  pixel.setX((planePoint.x() - this->context().planeArea.left()) *
             this->getDimension().width() /
             (this->context().planeArea.width()));
  return pixel;
}

AffineMap<double, 2UL, 2UL> SliceImage::getPixelToPlaneTrafo(bool invertYAxis) {
  auto planeArea = this->context().planeArea;
  auto imgSize = this->getDimension();

  double xTrans;
  double yTrans;
  double xScale;
  double yScale;

  xTrans = planeArea.left();
  xScale = planeArea.width() / imgSize.width();

  if (invertYAxis) {
    yTrans = planeArea.bottom();
    yScale = -planeArea.height() / imgSize.height();

  } else {
    yTrans = planeArea.top();
    yScale = planeArea.height() / imgSize.height();
  }
  auto trans = createTranslation(Vector<double, 2>(xTrans, yTrans));
  auto scale = Matrix<double, 2, 2>({xScale, 0}, {0, yScale});

  return trans * createLinearMap(scale);
}

SliceImage SliceImage::clone(bool enableSharedMemory) const {
  SliceImage _clone;
  _clone.imageData->width = this->getWidth();
  _clone.imageData->height = this->getHeight();
  _clone._context = this->_context;

  bool clFailed = false;
  if (this->getMode() == CLMEMORY_MODE) {
    try {
      _clone.imageData->clPixels = this->getCLBufferCopy();  // might throw
      _clone.imageData->pixels = this->imageData->pixels.copy(
          enableSharedMemory);  // not the same as getBufferCopy
      _clone.imageData->clInstance = this->imageData->clInstance;
      _clone.imageData->mode = CLMEMORY_MODE;
    } catch (opencl::CLException&) {
      clFailed = true;
    }
  }
  if (this->getMode() == STDMEMORY_MODE || clFailed) {
    _clone.imageData->pixels = this->getBufferCopy();
  }
  return _clone;
}
