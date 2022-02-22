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

#include <Voxie/OldFilterMask/polygonData.hpp>
#include <Voxie/OldFilterMask/shapes.hpp>

namespace vx {
namespace filter {

/**
 * @brief The polygon class offers methods for creating a polygon shape.
 */
class Polygon : public Shape {
 public:
  Polygon(QVector<QPointF> polygonCoords);
  Polygon(const QJsonObject& json);

  virtual void translateOrigin(qreal x, qreal y) override;
  virtual void rotate(qreal angle) override;

  /**
   * @brief getTransformatedPolyCoords
   * @return return the transformated coordinates of the polygon.
   */
  polygonData getTransformatedPolyCoords();

  /**
   * @brief path creates a QPainterPath from the shape.
   * @return the shape as a QPainterPath Object
   */
  QPainterPath path();

  /**
   * @brief pointFVectorToPoly converts a QVector full of QPointF's to a Vector
   * with polygonPoint.
   * @return a QVector with polygonPoint.
   */
  QVector<polygonPoint> pointFVectorToPoly(QVector<QPointF>);

  QString getType() override {
    return "de.uni_stuttgart.Voxie.Shape2D.Polygon";
  }
  QJsonObject getJson() override;

 private:
  QVector<QPointF> polygonCoords;
  polygonData data;

  QMatrix transformationMatrix;

  QMatrix transformationRotate;
  QMatrix transformationScale;
  QMatrix transformationTranslate;
};

}  // namespace filter
}  // namespace vx
