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

#include <Voxie/OldFilterMask/rectangleData.hpp>
#include <Voxie/OldFilterMask/shapes.hpp>

#include <QtGui/QTransform>

namespace vx {
namespace filter {

/**
 * @brief The rectangle class offers methods for creating a rectangle shape for
 * the masks.
 */
class Rectangle : public Shape {
 public:
  Rectangle(qreal startX, qreal startY, qreal endX, qreal endY);
  Rectangle(const QJsonObject& json);

  virtual void rotate(qreal angle) override;
  virtual void translateOrigin(qreal x, qreal y) override;
  /**
   * @brief path creates a QPainterPath from the shape.
   * @return the shape as a QPainterPath Object
   */
  QPainterPath path();

  /**
   * @brief getTransformationMatrix
   * @return the inverted Transformation matrix.
   */
  rectangleData getTransformationMatrix();
  void isPointIn(QPointF point);

  QString getType() override {
    return "de.uni_stuttgart.Voxie.Shape2D.Rectangle";
  }
  QJsonObject getJson() override;

 private:
  QPointF leftUp = QPointF(0, 1);
  QPointF leftDown = QPointF(0, 0);
  QPointF rightUp = QPointF(1, 1);
  QPointF rightDown = QPointF(1, 0);

  rectangleData data;

  QTransform transformationMatrix;
  QTransform transformationRotate;
  QTransform transformationScale;
  QTransform transformationTranslate;
};

}  // namespace filter
}  // namespace vx
