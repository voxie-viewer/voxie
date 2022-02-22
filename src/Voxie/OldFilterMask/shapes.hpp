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

#include <QtGui/QPainterPath>

namespace vx {
namespace filter {
/**
 * @brief The shapes class is a interface of the shapes rectangle, polygon and
 * ellipse.
 */
class Shape {
 public:
  virtual ~Shape();

  /**
   * @brief translateOrigin translates the origin of the masks.
   * @param x amount of translation in X Direction.
   * @param y amount of translation in Y Direction.
   */
  virtual void translateOrigin(qreal x, qreal y) = 0;

  /**
   * @brief rotate the matrix
   * @param angle the angle for the rotation.
   */
  virtual void rotate(qreal angle) = 0;

  virtual QString getType() = 0;
  virtual QJsonObject getJson() = 0;
  QJsonObject getJsonFull();
  static Shape* fromJson(const QJsonObject& json);

  static QJsonValue qMatrixToJson(const QMatrix& matrix);
  static QMatrix qMatrixFromJson(const QJsonValue& json);
  static QJsonValue qTransformToJson(const QTransform& transform);
  static QTransform qTransformFromJson(const QJsonValue& json);
  static QJsonValue qPointFToJson(const QPointF& pointF);
  static QPointF qPointFFromJson(const QJsonValue& json);
};
}  // namespace filter
}  // namespace vx
