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

#include "SliceCenterLayer.hpp"

#include <PluginVisSlice/SliceVisualizer.hpp>

#include <Voxie/Node/ParameterCopy.hpp>

using namespace vx;

SliceCenterLayer::SliceCenterLayer(SliceVisualizer* sv) : sv(sv) {
  // Redraw when the slice center is shown / hidden
  QObject::connect(sv->properties, &SliceProperties::showSliceCenterChanged,
                   this, &Layer::triggerRedraw);

  // Ignore changes to the slice, they don't affect the lines drawn here
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

void SliceCenterLayer::render(
    QImage& outputImage, const QSharedPointer<vx::ParameterCopy>& parameters,
    bool isMainImage) {
  Q_UNUSED(isMainImage);

  SlicePropertiesCopy properties(
      parameters->properties()[parameters->mainNodePath()]);

  if (!properties.showSliceCenter()) return;

  QPainter painter(&outputImage);

  QRectF area =
      SliceVisualizer::getCurrentPlaneArea(&properties, outputImage.size());
  qreal origX = -area.left();
  // qreal origY = -area.top();
  qreal origY = area.bottom();
  // normalize x & y
  origX /= area.width();
  origY /= area.height();
  // stretch x & y to canvas
  origX *= outputImage.width();
  origY *= outputImage.height();

  // draw x axis
  QPen pen(QColor(0x00, 0xff, 0x00));
  painter.setPen(pen);
  painter.drawLine(0, (int)origY, outputImage.width(), (int)origY);
  // draw y axis
  pen.setColor(QColor(0x00, 0x00, 0xff));
  painter.setPen(pen);
  painter.drawLine((int)origX, 0, (int)origX, outputImage.height());
}
