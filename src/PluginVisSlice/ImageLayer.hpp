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

#include <PluginVisSlice/Layer.hpp>

#include <PluginVisSlice/MultivariateDataWidget/multivariateDataWidget.hpp>
#include <PluginVisSlice/SliceVisualizer.hpp>

#include <Voxie/Data/Colorizer.hpp>
#include <Voxie/Data/Prototypes.hpp>
#include <Voxie/Data/Slice.hpp>
#include <Voxie/Data/Spectrum.hpp>

#include <Voxie/Node/ParameterCopy.hpp>

#include <VoxieBackend/Data/DataProperty.hpp>
#include <VoxieBackend/Data/VolumeSeriesData.hpp>

using namespace vx;

class SliceVisualizer;

class ImageLayer : public Layer {
  Q_OBJECT
  VX_REFCOUNTEDOBJECT

  // TODO: This is a hack, remove?
  // Note: This may only be used when isMainImage is true
  SliceVisualizer* sv;

 private:
  /**
   * Sum up colors at all pixel positions of a given images in a image list.
   * This function implements a post classification color mapping.
   */
  void sumUpColorsOverImages(QList<QPair<QImage, float>>* imageList,
                             QImage* outputImage);
  QPair<double, double> calcMean_StandardDeviation(QList<float>* data);
  QList<ColorizerEntry> createColorEntries(QColor channelColor,
                                           int channelMappingValue);

 public:
  ImageLayer(SliceVisualizer* sv);

  QString getName() override {
    return "de.uni_stuttgart.Voxie.SliceVisualizer.Layer.ImageLayer";
  }

  void render(QImage& outputImage,
              const QSharedPointer<vx::ParameterCopy>& parameters,
              bool isMainImage) override;
};
