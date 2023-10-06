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

#include <QPainter>
#include <QVector2D>

namespace vx {
/**
 * Represents a horizontal or vertical graph axis, displaying an arrow
 * indicating the axis direction, regularly spaced distance markers
 * with numeric labels, as well as a semi-transparent grid.
 *
 * It supports linear or logarithmic axis scaling.
 */
class LabeledAxis {
 public:
  enum Orientation {
    Horizontal,
    Vertical,
  };

  LabeledAxis(Orientation orientation = Horizontal);

  QRectF getRect() const;
  void setRect(QRectF value);

  QString getLabel() const;
  void setLabel(QString value);

  void setOrientation(Orientation value);
  Orientation getOrientation() const;

  void setColor(const QColor& value);
  QColor getColor() const;

  void setLowerValue(float value);
  float getLowerValue() const;

  void setUpperValue(float value);
  float getUpperValue() const;

  void setLogScale(bool value);
  bool isLogScale() const;

  void setIntegerLabels(bool value);
  bool isIntegerLabels() const;

  void draw(double dpiScale, QPainter& painter);

 private:
  Orientation orientation = Horizontal;

  QRectF rect;

  QString label;

  float lowerValue = 0.f;
  float upperValue = 0.f;
  bool logScale = false;
  bool integerLabels = false;

  QColor color = Qt::white;
};
}  // namespace vx
