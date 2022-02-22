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

#include "ImageLayer.hpp"

#include <PluginVisSlice/SliceVisualizer.hpp>

#include <Voxie/Data/Colorizer.hpp>
#include <Voxie/Data/Prototypes.hpp>

#include <Voxie/Node/ParameterCopy.hpp>

using namespace vx;

ImageLayer::ImageLayer(SliceVisualizer* sv) {
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
  connect(sv, &SliceVisualizer::volumeDataChangedFinished, this,
          &Layer::triggerRedraw);

  // Redraw when properties of the data change
  connect(sv, &SliceVisualizer::volumeDataRotationChanged, this,
          &Layer::triggerRedraw);
  connect(sv, &SliceVisualizer::volumeDataTranslationChanged, this,
          &Layer::triggerRedraw);

  // Redraw when color mapping changes
  QObject::connect(sv->properties, &SliceProperties::valueColorMappingChanged,
                   this, &Layer::triggerRedraw);

  // Redraw when interpolation changes
  QObject::connect(sv->properties, &SliceProperties::interpolationChanged, this,
                   &Layer::triggerRedraw);

  // Redraw when filter configuration changes
  QObject::connect(sv->properties,
                   &SliceProperties::filter2DConfigurationChanged, this,
                   &Layer::triggerRedraw);
}

void ImageLayer::render(QImage& outputImage,
                        const QSharedPointer<vx::ParameterCopy>& parameters) {
  SlicePropertiesCopy properties(
      parameters->properties()[parameters->mainNodePath()]);

  VolumePropertiesCopy volumeProperties(
      parameters->properties()[properties.volumeRaw()]);

  auto data = qSharedPointerDynamicCast<VolumeData>(
      parameters->getData(properties.volumeRaw()).data());
  if (!data)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error", "data == nullptr");

  auto adjustedRotation = volumeProperties.rotation();
  auto adjustedPosition = volumeProperties.translation();
  PlaneInfo plane;
  plane.origin =
      adjustedRotation.inverted() * (properties.origin() - adjustedPosition);
  plane.rotation = (adjustedRotation.inverted() * properties.orientation());

  auto sliceArea =
      SliceVisualizer::getCurrentPlaneArea(&properties, outputImage.size());

  auto interpolation = SliceVisualizer::getInterpolation(&properties);

  // TODO: try to release memory earlier?
  SliceImage colorizedImage = Slice::generateImage(
      data.data(), plane, sliceArea, outputImage.size(), interpolation);

  // TODO: Check whether this leaks filter objects
  vx::filter::FilterChain2D filterChain;
  filterChain.fromXMLString(properties.filter2DConfiguration());

  // TODO: This function should probably return the filtered image or modify its
  // argument
  filterChain.applyTo(colorizedImage);
  SliceImage filteredImage = filterChain.getOutputSlice();

  vx::Colorizer colorizer;
  colorizer.setEntries(properties.valueColorMapping());
  auto targetImage = colorizer.toQImage(filteredImage);

  QPainter painter(&outputImage);
  painter.drawImage(QPointF(0, 0), targetImage);
}
