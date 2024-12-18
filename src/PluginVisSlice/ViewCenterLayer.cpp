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

#include "ViewCenterLayer.hpp"

#include <PluginVisSlice/SliceVisualizer.hpp>

#include <Voxie/Node/ParameterCopy.hpp>

using namespace vx;

ViewCenterLayer::ViewCenterLayer(SliceVisualizer* sv) : sv(sv) {
  // Redraw when the view center is shown / hidden
  QObject::connect(sv->properties, &SliceProperties::showViewCenterChanged,
                   this, &Layer::triggerRedraw);
}

void ViewCenterLayer::render(
    QImage& outputImage, const QSharedPointer<vx::ParameterCopy>& parameters,
    bool isMainImage) {
  Q_UNUSED(isMainImage);

  SlicePropertiesCopy properties(
      parameters->properties()[parameters->mainNodePath()]);

  if (!properties.showViewCenter()) return;

  QPainter painter(&outputImage);

  auto minSize = std::min(outputImage.width(), outputImage.height());
  auto lenHalf = minSize / 12;
  auto midX = outputImage.width() / 2;
  auto midY = outputImage.height() / 2;
  // draw x axis
  QPen pen(QColor(0xcc, 0x80, 0x80));
  painter.setPen(pen);
  painter.drawLine(midX - lenHalf, midY, midX + lenHalf, midY);
  // draw y axis
  pen.setColor(QColor(0x80, 0xcc, 0x80));
  painter.setPen(pen);
  painter.drawLine(midX, midY - lenHalf, midX, midY + lenHalf);
}
