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

#include "LabeledAxis.hpp"

#include <QApplication>

#include <algorithm>
#include <cmath>

using namespace vx;

LabeledAxis::LabeledAxis(Orientation orientation) : orientation(orientation) {}

QRectF LabeledAxis::getRect() const { return rect; }

void LabeledAxis::setRect(QRectF value) { rect = value; }

QString LabeledAxis::getLabel() const { return label; }

void LabeledAxis::setLabel(QString value) { label = value; }

void LabeledAxis::setOrientation(Orientation orientation) {
  this->orientation = orientation;
}

LabeledAxis::Orientation LabeledAxis::getOrientation() const {
  return orientation;
}

void LabeledAxis::setColor(const QColor& color) { this->color = color; }

QColor LabeledAxis::getColor() const { return color; }

void LabeledAxis::setLowerValue(float value) { lowerValue = value; }

float LabeledAxis::getLowerValue() const { return lowerValue; }

void LabeledAxis::setUpperValue(float value) { upperValue = value; }

float LabeledAxis::getUpperValue() const { return upperValue; }

void LabeledAxis::setLogScale(bool value) { logScale = value; }

bool LabeledAxis::isLogScale() const { return logScale; }

void LabeledAxis::setIntegerLabels(bool value) { integerLabels = value; }

bool LabeledAxis::isIntegerLabels() const { return integerLabels; }

void LabeledAxis::draw(QPainter& painter) {
  static const float axisStrokeWidth = 2;
  static const float gridStrokeWidth = 1;
  static const float arrowSize = 12;
  static const float arrowWidth = 6;

  static const float markerSize = 4;
  static const int markerFontSize = 10;
  static const int markerOffset = 8;
  static const float maxTextSize = 150;

  QColor gridColor = color;
  gridColor.setAlpha(gridColor.alpha() * 0.1f);

  QVector2D position(rect.x(), rect.y());
  QVector2D lineSize(orientation == Horizontal ? rect.width() : 0,
                     orientation == Vertical ? rect.height() : 0);
  QVector2D gridSize(orientation == Vertical ? rect.width() : 0,
                     orientation == Horizontal ? rect.height() : 0);

  QVector2D direction = lineSize.normalized();
  QVector2D head = position + lineSize;
  QVector2D arrowBase = position + lineSize - direction * arrowSize;
  QVector2D perpendicular(direction.y(), direction.x());

  // Assign font
  auto font = QApplication::font();
  font.setPixelSize(markerFontSize);
  painter.setFont(font);

  // Draw axis line
  painter.setPen(QPen(color, axisStrokeWidth));
  painter.setBrush(Qt::NoBrush);
  painter.drawLine(position.toPointF(), arrowBase.toPointF());

  // Draw arrow head
  painter.setPen(Qt::NoPen);
  painter.setBrush(color);
  painter.drawPolygon(QPolygonF(QVector<QPointF>{
      (arrowBase + perpendicular * arrowWidth).toPointF(),
      (arrowBase - perpendicular * arrowWidth).toPointF(), head.toPointF()}));

  auto scale = [this](float value) { return logScale ? logf(value) : value; };
  auto unscale = [this](float value) { return logScale ? expf(value) : value; };

  float upper = scale(upperValue);
  float lower = scale(lowerValue);

  painter.setBrush(Qt::NoBrush);

  int markerCount = lineSize.length() / 40.f;

  // Draw axis markers
  for (int i = 0; i < markerCount; ++i) {
    painter.setPen(QPen(color, axisStrokeWidth));

    float fraction = i / float(markerCount);
    float value = unscale(fraction * (upper - lower) + lower);

    QVector2D pos = fraction * head + (1 - fraction) * position;

    // Draw axis marker
    painter.drawLine(pos.toPointF(),
                     (pos + perpendicular * markerSize).toPointF());

    // Don't draw axis markers if they would be "NaN"
    if (qIsFinite(upper) && qIsFinite(lower) && !qFuzzyIsNull(upper - lower)) {
      // Compute bounding rectangle for axis marker text
      QRectF textRect(
          (pos + perpendicular * (markerOffset + maxTextSize * 0.5f) -
           QVector2D(maxTextSize, maxTextSize) * 0.5f)
              .toPointF(),
          QSizeF(maxTextSize, maxTextSize));

      // Compute axis marker text
      QString text = isIntegerLabels() ? QString::number(qint64(value))
                                       : QString::number(value, 'g', 5);

      // Define orientation-dependent axis marker text alignment
      QTextOption textOption;
      textOption.setWrapMode(QTextOption::NoWrap);
      textOption.setAlignment(orientation == Horizontal
                                  ? Qt::AlignTop | Qt::AlignHCenter
                                  : Qt::AlignRight | Qt::AlignVCenter);

      // Draw axis marker text into bounding rectangle
      painter.setPen(color);
      painter.drawText(textRect, text, textOption);
    }

    // Draw grid lines
    painter.setPen(QPen(gridColor, gridStrokeWidth));
    painter.drawLine(pos.toPointF(), (pos + gridSize).toPointF());
  }

  // Draw the label of the axis
  int textDistanceV = 20;
  int textDistanceH = 25;
  int textHeight = 20;
  QRectF labelTextRect(
      (orientation == Horizontal
           ? (position + QVector2D(0, textDistanceV))
           : (QVector2D(-rect.y(), rect.x() - textDistanceH - textHeight)))
          .toPointF(),
      orientation == Horizontal ? QSizeF(rect.width(), textHeight)
                                : QSizeF(-rect.height(), textHeight));
  QTextOption textOption;
  textOption.setWrapMode(QTextOption::NoWrap);
  textOption.setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
  if (orientation == Vertical) {
    painter.rotate(-90);
  }
  painter.setPen(color);
  painter.drawText(labelTextRect, label, textOption);
  if (orientation == Vertical) {
    painter.rotate(90);
  }
}
