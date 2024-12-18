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

#include "Grid.hpp"

#include <Voxie/Node/ParameterCopy.hpp>

#include <PluginVisSlice/SliceVisualizer.hpp>

#include <QPainter>
#include <QPen>
#include <QPoint>

using namespace vx;

Grid::Grid(SliceVisualizer* sv) {
  // Redraw when any of the grid properties changes
  QObject::connect(sv->properties, &SliceProperties::gridShowChanged, this,
                   &Layer::triggerRedraw);
  QObject::connect(sv->properties, &SliceProperties::gridColorChanged, this,
                   &Layer::triggerRedraw);
  QObject::connect(sv->properties,
                   &SliceProperties::gridSpacingAutomaticChanged, this,
                   &Layer::triggerRedraw);
  QObject::connect(sv->properties, &SliceProperties::gridSpacingChanged, this,
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

Grid::~Grid() {}

void Grid::render(QImage& outputImage,
                  const QSharedPointer<vx::ParameterCopy>& parameters,
                  bool isMainImage) {
  Q_UNUSED(isMainImage);

  SlicePropertiesCopy properties(
      parameters->properties()[parameters->mainNodePath()]);

  if (!properties.gridShow()) return;

  // PlaneInfo plane(properties.origin(), properties.orientation());
  // auto planeArea =
  //    SliceVisualizer::getCurrentPlaneArea(&properties, outputImage.size());

  double spacing;
  if (!properties.gridSpacingAutomatic()) {
    spacing = properties.gridSpacing();
  } else {
    double pixelSize =
        SliceVisualizer::getCurrentPixelSize(&properties, outputImage.size());

    // TODO: check this, this is probably max, not min
    double minGridSpacingPixel = 30;  // TODO: Expose as property?
    spacing = std::pow(10, -24);
    for (int i = -24; i <= 24; i++) {
      for (double factor : {1.0, 2.0, 5.0}) {
        double newSpacing = factor * std::pow(10, i);
        if (pixelSize * minGridSpacingPixel >= newSpacing) spacing = newSpacing;
      }
    }
  }
  // qDebug() << "Grid spacing" << spacing;

  if (spacing <= 0) {
    qWarning() << "Attempting to draw grid with" << spacing;
    return;
  }

  // TODO: Change this, should not draw a grid with another grid constant

  int penWidth = 1;
  // int minMeshWidth = penWidth + 10;
  int minMeshWidth = penWidth;

  double pixelSize =
      SliceVisualizer::getCurrentPixelSize(&properties, outputImage.size());
  float imgDistance = (1 / pixelSize) * spacing;

  // Grid mesh width is to small to see each mesh
  if (imgDistance < minMeshWidth) {
    imgDistance = minMeshWidth;
  }

  // TODO: show actual grid size somewhere?

  QPainter painter(&outputImage);

  painter.setRenderHint(QPainter::Antialiasing, true);
  // m_color.setAlpha(m_opacity);
  QPen pen(properties.gridColor().asQColor());
  pen.setWidth(penWidth);
  painter.setPen(pen);

  // Draw vertical lines for grid
  QPoint startPosVertical = QPoint(0, 0);
  QPoint endPosVertical = QPoint(0, outputImage.height());

  for (int x = imgDistance; x < outputImage.width(); x = x + imgDistance) {
    startPosVertical.setX(x);
    endPosVertical.setX(x);
    painter.drawLine(startPosVertical, endPosVertical);
  }

  // Draw horizontal lines for grid
  QPoint startPosHorizontal = QPoint(0, 0);
  QPoint endPosHorizontal = QPoint(outputImage.width(), 0);

  for (int y = imgDistance; y < outputImage.height(); y = y + imgDistance) {
    startPosHorizontal.setY(y);
    endPosHorizontal.setY(y);
    painter.drawLine(startPosHorizontal, endPosHorizontal);
  }
}
