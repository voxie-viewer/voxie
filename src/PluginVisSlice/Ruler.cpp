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

#include "Ruler.hpp"

#include <VoxieBackend/Data/VolumeDataVoxel.hpp>

#include <Voxie/Node/ParameterCopy.hpp>

#include <QPainter>
#include <QPen>
#include <QPicture>
#include <QPoint>

#include <qmath.h>

using namespace vx;

// TODO: copied from UnitSpinBox.cpp
struct PrefixInfo {
  const char* prefix;
  double value;
};
static const PrefixInfo prefixes[]{
    // https://en.wikipedia.org/wiki/Metric_prefix#List_of_SI_prefixes
    // https://en.wikipedia.org/w/index.php?title=Metric_prefix&oldid=925982941#List_of_SI_prefixes
    {"Y", 1e+24},
    {"Z", 1e+21},
    {"E", 1e+18},
    {"P", 1e+15},
    {"T", 1e+12},
    {"G", 1e+09},
    {"M", 1e+06},
    {"k", 1e+03},
    //{ "h", 1e+02 },
    //{ "da", 1e+01 },
    {"", 1e+00},
    //{"d", 1e-01},
    //{"c", 1e-02},
    {"m", 1e-03},
    {"µ", 1e-06},
    {"n", 1e-09},
    {"p", 1e-12},
    {"f", 1e-15},
    {"a", 1e-18},
    {"z", 1e-21},
    {"y", 1e-24},
};

Ruler::Ruler(SliceVisualizer* sv) {
  // Redraw when any of the ruler properties changes
  QObject::connect(sv->properties, &SliceProperties::rulerShowChanged, this,
                   &Layer::triggerRedraw);
  QObject::connect(sv->properties, &SliceProperties::rulerColorChanged, this,
                   &Layer::triggerRedraw);
  QObject::connect(sv->properties,
                   &SliceProperties::rulerSpacingAutomaticChanged, this,
                   &Layer::triggerRedraw);
  QObject::connect(sv->properties, &SliceProperties::rulerSpacingChanged, this,
                   &Layer::triggerRedraw);

  // Redraw when the slice changes
  // TODO: currently this layer does not depend on the slice, should it?
  /*
  QObject::connect(sv->properties,
                   &SliceProperties::orientationChanged, this,
                   &Layer::triggerRedraw);
  QObject::connect(sv->properties, &SliceProperties::originChanged,
                   this, &Layer::triggerRedraw);
  */

  // Redraw when shown part of slice changes
  connect(sv->properties, &SliceProperties::centerPointChanged, this,
          &Layer::triggerRedraw);
  connect(sv->properties, &SliceProperties::verticalSizeChanged, this,
          &Layer::triggerRedraw);
}

Ruler::~Ruler() {}

void Ruler::render(QImage& outputImage,
                   const QSharedPointer<vx::ParameterCopy>& parameters) {
  SlicePropertiesCopy properties(
      parameters->properties()[parameters->mainNodePath()]);

  if (!properties.rulerShow()) return;

  double pixelSize =
      SliceVisualizer::getCurrentPixelSize(&properties, outputImage.size());

  double spacing;
  if (!properties.rulerSpacingAutomatic()) {
    spacing = properties.rulerSpacing();
  } else {
    // TODO: check this, this is probably max, not min
    double minRulerSpacingPixel = 30;  // TODO: Expose as property?
    spacing = std::pow(10, -24);
    for (int i = -24; i <= 24; i++) {
      for (double factor : {1.0, 2.0, 5.0}) {
        double newSpacing = factor * std::pow(10, i);
        if (pixelSize * minRulerSpacingPixel >= newSpacing)
          spacing = newSpacing;
      }
    }
  }

  // TODO: default values? Should matter only for extremly large values
  double ufactor = 1;
  QString unitString = "m";
  for (const auto& prefix : prefixes) {
    if (spacing / prefix.value <= 10) {
      ufactor = spacing / prefix.value;
      unitString = QString() + prefix.prefix + "m";
    }
  }

  // qDebug() << "Ruler spacing" << spacing << ufactor << unitString;

  // TODO: rework the algorithm when the 5/10-text is shown (when ufactor is not
  // a power of 10, this is does not match 5/10)

  float imgDistance = (1 / pixelSize) * spacing;

  if (spacing <= 0) {
    qWarning() << "Attempting to draw ruler with" << spacing;
    return;
  }

  int penWidth = 1;
  if (imgDistance < penWidth) {
    // TODO: This is broken (would not match anymore)
    // imgDistance = penWidth + 1;
    qWarning() << "Not drawing ruler, spacing too small" << imgDistance
               << penWidth;
    return;  // don't paint anything
  }

  QPainter painter(&outputImage);
  painter.setRenderHint(QPainter::Antialiasing, true);

  const int lineLength = 5;
  const int factor5LineLength = lineLength + 5;
  const int factor10LineLength = factor5LineLength + 5;
  int offset = factor10LineLength + 2;

  // ToDo Draw Ruler Background

  auto contrastColor = properties.rulerColor();
  contrastColor.setRed(1 - contrastColor.red());
  contrastColor.setGreen(1 - contrastColor.green());
  contrastColor.setBlue(1 - contrastColor.blue());
  QPen pen1(contrastColor.asQColor());
  pen1.setWidth(1);

  QRect recVertical;
  QRect recHorizontal;
  recVertical.setCoords(0, 0, factor10LineLength + 2, outputImage.height());
  recHorizontal.setCoords(0, 0, outputImage.width(), factor10LineLength + 2);
  painter.setPen(pen1);
  painter.setBrush(contrastColor.asQColor());
  painter.drawRect(recHorizontal);
  painter.drawRect(recVertical);

  // ToDo Draw Ruler Background END
  QPen pen(properties.rulerColor().asQColor());
  pen.setWidth(penWidth);
  painter.setPen(pen);
  painter.drawText(2, offset / 2, unitString);

  // Draw horizontal lines for Ruler

  QPoint startPosHorizontal = QPoint(offset, offset - lineLength);
  QPoint endPosHorizontal = QPoint(offset, offset);

  int indexHorizontal = 0;
  for (int x = offset; x < outputImage.width(); x = x + imgDistance) {
    if (indexHorizontal % 10 == 0) {
      startPosHorizontal.setY(offset - factor10LineLength);
      painter.drawText(x + 2, (offset / 2) + 1,
                       QString::number(indexHorizontal * ufactor));
    } else {
      if (indexHorizontal % 5 == 0) {
        startPosHorizontal.setY(offset - factor5LineLength);
        painter.drawText(x + 1, (offset / 2) + 1,
                         QString::number(indexHorizontal * ufactor));
      } else {
        startPosHorizontal.setY(offset - lineLength);
      }
    }
    startPosHorizontal.setX(x);
    endPosHorizontal.setX(x);

    painter.drawLine(startPosHorizontal, endPosHorizontal);
    indexHorizontal++;
  }

  // Draw vertical lines for ruler

  QPoint startPosVertical = QPoint(offset - lineLength, offset);
  QPoint endPosVertical = QPoint(offset, offset);

  int indexVertical = 0;
  for (int y = offset; y < outputImage.height(); y = y + imgDistance) {
    if (indexVertical % 10 == 0) {
      startPosVertical.setX(offset - factor10LineLength);
      painter.drawText(1, y + 10, QString::number(indexVertical * ufactor));
    } else {
      if (indexVertical % 5 == 0) {
        startPosVertical.setX(offset - factor5LineLength);
        painter.drawText(1, y + 10, QString::number(indexVertical * ufactor));
      } else {
        startPosVertical.setX(offset - lineLength);
      }
    }
    startPosVertical.setY(y);
    endPosVertical.setY(y);

    painter.drawLine(startPosVertical, endPosVertical);
    indexVertical++;
  }
}
