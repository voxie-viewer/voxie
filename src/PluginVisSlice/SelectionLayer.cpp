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

#include "SelectionLayer.hpp"
#include <PluginVisSlice/SliceVisualizer.hpp>

#include <Voxie/Data/Colorizer.hpp>
#include <Voxie/Data/Prototypes.hpp>

#include <Voxie/Node/ParameterCopy.hpp>

using namespace vx;

SelectionLayer::SelectionLayer(SliceVisualizer* sv) {
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

  // Redraw when data changes
  connect(sv, &SliceVisualizer::labelContainerChangedFinished, this,
          &Layer::triggerRedraw);
}

void SelectionLayer::render(
    QImage& outputImage, const QSharedPointer<vx::ParameterCopy>& parameters) {
  SlicePropertiesCopy properties(
      parameters->properties()[parameters->mainNodePath()]);

  VolumePropertiesCopy volumeProperties(
      parameters->properties()[properties.volumeRaw()]);

  // Return if no label container is connected
  if (properties.labelContainerRaw() == QDBusObjectPath("/")) return;

  // Data is in myCase not neeeded
  auto data = qSharedPointerDynamicCast<ContainerData>(
      parameters->getData(properties.labelContainerRaw()).data());

  if (!data)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "ContainerData == nullptr");

  auto labelVolume =
      dynamic_cast<VolumeData*>(data->getElement("labelVolume").data());

  DataType dataType = labelVolume->getDataType();

  int selectionBit = 0;
  switch (dataType) {
    case DataType::UInt8:
      selectionBit = 7;
      break;
    case DataType::UInt16:
      selectionBit = 15;
      break;
    case DataType::UInt32:
      selectionBit = 31;
      break;
    case DataType::UInt64:
      selectionBit = 63;
      break;
    default:
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "The LabelValume must be of the type UInt8, UInt16, "
                          "UInt32 or Uint64");
  }
  // TODO: if we have later a Int image compare the bits instead of values
  int selectionValue = pow(2, selectionBit);

  auto adjustedRotation = volumeProperties.rotation();
  auto adjustedPosition = volumeProperties.translation();
  PlaneInfo plane;
  plane.origin =
      adjustedRotation.inverted() * (properties.origin() - adjustedPosition);
  plane.rotation = (adjustedRotation.inverted() * properties.orientation());

  auto sliceArea =
      SliceVisualizer::getCurrentPlaneArea(&properties, outputImage.size());

  // TODO: try to release memory earlier?
  SliceImage colorizedImage =
      Slice::generateImage(labelVolume, plane, sliceArea, outputImage.size(),
                           InterpolationMethod::NearestNeighbor);

  QImage qimage(colorizedImage.getDimension(), QImage::Format_ARGB32);
  QRgb* qimgbuffer = (QRgb*)qimage.bits();
  // TODO maybe replace buffer
  FloatBuffer buffer = colorizedImage.getBufferCopy();
  QRgb colorSelection = QColor(255, 0, 0, 100).rgba();
  QRgb colorNoSelection = QColor(0, 0, 0, 0).rgba();

  for (size_t i = 0; i < buffer.length(); i++) {
    float value = buffer[i];

    if (value >= selectionValue) {
      qimgbuffer[i] = colorSelection;
    } else {
      qimgbuffer[i] = colorNoSelection;
    }
  }

  // Mirror y axis (the y axis of FloatImage goes from bottom to top, the y axis
  // of QImage goes from top to bottom)
  for (std::size_t y = 0; y < (std::size_t)qimage.height() / 2; y++) {
    for (std::size_t x = 0; x < (std::size_t)qimage.width(); x++) {
      std::size_t y2 = qimage.height() - 1 - y;
      std::swap(qimgbuffer[y * qimage.width() + x],
                qimgbuffer[y2 * qimage.width() + x]);
    }
  }

  QPainter painter(&outputImage);

  painter.drawImage(QPointF(0, 0), qimage);
}
