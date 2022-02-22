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

#include "VolumeGrid.hpp"

#include <PluginVisSlice/SliceVisualizer.hpp>

#include <Voxie/Data/Prototypes.hpp>

#include <Voxie/Node/ParameterCopy.hpp>

#include <QPainter>
#include <QPen>
#include <QPicture>
#include <QPoint>

using namespace vx;

VolumeGrid::VolumeGrid(SliceVisualizer* sv) {
  // Redraw when any of the volume grid properties changes
  QObject::connect(sv->properties, &SliceProperties::volumeGridShowChanged,
                   this, &Layer::triggerRedraw);
  QObject::connect(sv->properties, &SliceProperties::volumeGridColorChanged,
                   this, &Layer::triggerRedraw);

  // Redraw when the slice changes
  QObject::connect(sv->properties, &SliceProperties::orientationChanged, this,
                   &Layer::triggerRedraw);
  QObject::connect(sv->properties, &SliceProperties::originChanged, this,
                   &Layer::triggerRedraw);

  // Redraw when shown part of slice changes
  connect(sv->properties, &SliceProperties::centerPointChanged, this,
          &Layer::triggerRedraw);
  connect(sv->properties, &SliceProperties::verticalSizeChanged, this,
          &Layer::triggerRedraw);

  // Redraw when the volume data changes
  connect(sv, &SliceVisualizer::volumeDataChangedFinished, this,
          &Layer::triggerRedraw);

  // Redraw when properties of the data change
  connect(sv, &SliceVisualizer::volumeDataRotationChanged, this,
          &Layer::triggerRedraw);
  connect(sv, &SliceVisualizer::volumeDataTranslationChanged, this,
          &Layer::triggerRedraw);
}

VolumeGrid::~VolumeGrid() {}

void VolumeGrid::render(QImage& outputImage,
                        const QSharedPointer<vx::ParameterCopy>& parameters) {
  SlicePropertiesCopy properties(
      parameters->properties()[parameters->mainNodePath()]);

  if (!properties.volumeGridShow()) return;

  PlaneInfo plane(properties.origin(), properties.orientation());
  auto planeArea =
      SliceVisualizer::getCurrentPlaneArea(&properties, outputImage.size());

  auto volumePath = properties.volumeRaw();
  if (volumePath == QDBusObjectPath("/")) return;
  VolumePropertiesCopy volumeProperties(parameters->properties()[volumePath]);

  auto data = qSharedPointerDynamicCast<VolumeData>(
      parameters->getData(volumePath).data());
  if (!data) return;

  try {
    // get the necassary data to draw a grid
    auto adjustedRotation = volumeProperties.rotation();
    auto adjustedPosition = volumeProperties.translation();
    plane.origin =
        adjustedRotation.inverted() * (plane.origin - adjustedPosition);
    plane.rotation = (adjustedRotation.inverted() * plane.rotation);

    QVector3D origin = plane.origin;
    origin += plane.tangent() * planeArea.x();
    origin += plane.cotangent() * planeArea.y();

    // draw on outputImage
    data->extractGrid(origin, plane.rotation, outputImage.size(),
                      planeArea.width() / outputImage.width(),
                      planeArea.height() / outputImage.height(), outputImage,
                      properties.volumeGridColor().asQColor().rgba());
  } catch (std::exception& e) {
    qDebug() << e.what();
  }
}
