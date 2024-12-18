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

ImageLayer::ImageLayer(SliceVisualizer* sv) : sv(sv) {
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

  // Bridge from MultivariateDataWidget to SliceVisualizer to Layer
  connect(this->sv, &SliceVisualizer::multivariateDataPropertiesChangedOut,
          this, &Layer::triggerRedraw);
}

void ImageLayer::render(QImage& outputImage,
                        const QSharedPointer<vx::ParameterCopy>& parameters,
                        bool isMainImage) {
  // qDebug() << "start Rendering";
  SlicePropertiesCopy properties(
      parameters->properties()[parameters->mainNodePath()]);

  VolumePropertiesCopy volumeProperties(
      parameters->properties()[properties.volumeRaw()]);

  QDBusObjectPath volumeRaw = properties.volumeRaw();
  if (volumeRaw == QDBusObjectPath("/")) {
    qDebug() << "No volume connected, nothing to render";
    return;
  }

  auto data = qSharedPointerDynamicCast<VolumeData>(
      parameters->getData(volumeRaw).data());
  QQuaternion adjustedRotation = volumeProperties.rotation();
  QVector3D adjustedPosition = volumeProperties.translation();

  PlaneInfo plane;
  plane.origin =
      adjustedRotation.inverted() * (properties.origin() - adjustedPosition);
  plane.rotation = (adjustedRotation.inverted() * properties.orientation());

  QRectF sliceArea =
      SliceVisualizer::getCurrentPlaneArea(&properties, outputImage.size());

  vx::InterpolationMethod interpolation =
      SliceVisualizer::getInterpolation(&properties);

  // TODO: Check whether this leaks filter objects
  vx::filter::FilterChain2D filterChain;
  filterChain.fromXMLString(properties.filter2DConfiguration());

  if (data) {
    // normal volume data

    // TODO: try to release memory earlier?
    SliceImage sliceImage = generateSliceImage(
        data.data(), plane, sliceArea, outputImage.size(), interpolation);

    // TODO: This function should probably return the filtered image or modify
    // its argument
    filterChain.applyTo(sliceImage);
    SliceImage filteredImage = filterChain.getOutputSlice();

    if (isMainImage) {
      // TODO: This is kind of a hack to update the slice histogram
      // here. Remove the slice histogram?
      Q_EMIT this->sv->signalRequestHistogram(filteredImage);
    }

    vx::Colorizer colorizer;
    colorizer.setEntries(properties.valueColorMapping());
    QImage targetImage = colorizer.toQImage(filteredImage);

    QPainter painter(&outputImage);
    painter.drawImage(QPointF(0, 0), targetImage);

  } else {
    // invalid data or multivariate data
    auto dataSeries = qSharedPointerDynamicCast<VolumeSeriesData>(
        parameters->getData(volumeRaw).data());
    if (dataSeries) {
      if (dataSeries->dimensionCount() != 1) {
        return;
      }

      auto dimension = dataSeries->dimensions()[0];

      MultivariateDataWidget* widget = this->sv->getMultivariateDataWidget();
      ActiveMode activeMode = widget->getActiveMode();
      if (dimension->property()->name().contains(
              "de.uni_stuttgart.Voxie.SeriesDimension.")) {  // check generic
                                                             // data types

        if (activeMode == ActiveMode::none) {
          // qDebug() << "Activemode == None. Nothing to render";
          return;
        } else if (activeMode == ActiveMode::details) {
          QList<QPair<QImage, float>> outputImageList;
          QList<channelMetaData> actChan = widget->getActiveChannels();
          for (channelMetaData channel : actChan) {
            const QMap<SeriesData::EntryKeyList, QSharedPointer<Data>>
                channelDataList = dataSeries->entries();

            if (channel.isCustomChannel) {
              QList<vx::SpectrumResult> channelIndicators =
                  vx::findMatchingSpectrums(dimension,
                                            (double)channel.channelMin,
                                            (double)channel.channelMax);

              for (vx::SpectrumResult indicator : channelIndicators) {
                QList<SeriesDimension::EntryKey> keyList;
                keyList.append(indicator.key);
                QSharedPointer<vx::Data> channelData =
                    dataSeries->lookupEntry(keyList);

                QSharedPointer<VolumeData> channelDataVolume =
                    qSharedPointerDynamicCast<VolumeData>(channelData);

                // # create corresponding slice image
                SliceImage sliceImage = generateSliceImage(
                    channelDataVolume.data(), plane, sliceArea,
                    outputImage.size(), interpolation);

                filterChain.applyTo(sliceImage);
                SliceImage filteredImage = filterChain.getOutputSlice();

                // # prepare colorizer
                vx::Colorizer colorizer;
                colorizer.setEntries(
                    createColorEntries(channel.color, channel.mappingValue));

                // # colorize corresponding slice image
                QImage currentImage = colorizer.toQImage(filteredImage);
                QPair<QImage, float> sliceWithWeight =
                    QPair<QImage, float>(currentImage, indicator.weight);
                outputImageList.append(sliceWithWeight);
              }
            } else {
              // EntryKeyList = QList<SeriesDimension::EntryKey>
              QList<SeriesDimension::EntryKey> keyList;
              keyList.append(channel.entryIndex);
              QSharedPointer<vx::Data> channelData =
                  dataSeries->lookupEntry(keyList);

              QSharedPointer<VolumeData> channelDataVolume =
                  qSharedPointerDynamicCast<VolumeData>(channelData);

              // # create corresponding slice image
              SliceImage sliceImage =
                  generateSliceImage(channelDataVolume.data(), plane, sliceArea,
                                     outputImage.size(), interpolation);

              filterChain.applyTo(sliceImage);
              SliceImage filteredImage = filterChain.getOutputSlice();

              // # prepare colorizer
              vx::Colorizer colorizer;
              colorizer.setEntries(
                  createColorEntries(channel.color, channel.mappingValue));

              // # colorize corresponding slice image
              QImage currentImage = colorizer.toQImage(filteredImage);

              QPair<QImage, float> sliceWithWeight =
                  QPair<QImage, float>(currentImage, 1.0);
              outputImageList.append(sliceWithWeight);
            }
          }

          this->sumUpColorsOverImages(&outputImageList, &outputImage);
        } else if (activeMode == ActiveMode::overview) {
          // # extract slices from volumes
          QList<SliceImage> sliceList;
          for (QSharedPointer<Data> channelData : dataSeries->entries()) {
            QSharedPointer<VolumeData> channelDataVolume =
                qSharedPointerDynamicCast<VolumeData>(channelData);
            SliceImage sliceImage =
                generateSliceImage(channelDataVolume.data(), plane, sliceArea,
                                   outputImage.size(), interpolation);

            filterChain.applyTo(sliceImage);
            SliceImage filteredImage = filterChain.getOutputSlice();
            sliceList.append(filteredImage);
          }

          for (int w = 0; w < outputImage.width(); w++) {
            for (int h = 0; h < outputImage.height(); h++) {
              // # get all values on position (w,h)
              QList<float> pixelList;

              for (int i = 0; i < sliceList.size(); i++) {
                const float value =
                    ((SliceImage)sliceList.at(i)).getPixel(w, h);

                if (!std::isnan(value)) {
                  pixelList.append(value);
                }
              }

              QPair<double, double> erg =
                  calcMean_StandardDeviation(&pixelList);

              const double mean = std::max(erg.first, 0.0);
              double standartDeviation = std::max(erg.second, 0.0);
              standartDeviation = standartDeviation / mean;

              const float avgMin = widget->getOverviewAvgMin();
              const float avgMax = widget->getOverviewAvgMax();

              const float stdDevMin = widget->getOverviewStdDevMin();
              const float stdDevMax = widget->getOverviewStdDevMax();

              OverviewStrategyOption overviewStrategy =
                  widget->getOverviewStrategy();

              const double min = 0.0;

              // hue: valuerange(int): 0° - 360°, in practice range -30° to
              // avoid confusion between very high and very low values
              double hue = 0.0;
              const double hueMax = 330.0;

              // saturation: valuerange(int): 0 - 255
              double sat = 0.0;
              const double satMax = 255.0;

              // value: valuerange(int): 0 - 255
              double value = 0.0;
              const double valueMax = 255.0;

              if (overviewStrategy ==
                  OverviewStrategyOption::Avg2Hue_StdDev2Sat) {
                hue = mapValueFromSourceScaleToTargetScale(avgMin, avgMax, min,
                                                           hueMax, mean);
                sat = mapValueFromSourceScaleToTargetScale(
                    stdDevMin, stdDevMax, min, satMax, standartDeviation);
                value = valueMax;
              }

              if (overviewStrategy ==
                  OverviewStrategyOption::StdDev2Hue_Avg2Sat) {
                hue = mapValueFromSourceScaleToTargetScale(
                    stdDevMin, stdDevMax, min, hueMax, standartDeviation);
                sat = mapValueFromSourceScaleToTargetScale(avgMin, avgMax, min,
                                                           satMax, mean);
                value = valueMax;
              }

              if (overviewStrategy ==
                  OverviewStrategyOption::Avg2Value_StdDev2Sat) {
                hue = min;
                sat = mapValueFromSourceScaleToTargetScale(
                    stdDevMin, stdDevMax, min, satMax, standartDeviation);
                value = mapValueFromSourceScaleToTargetScale(
                    avgMin, avgMax, min, valueMax, mean);
              }

              // # Ensure color components are in their boundries
              hue = std::max(std::min(hue, hueMax), min);
              sat = std::max(std::min(sat, satMax), min);
              value = std::max(std::min(value, valueMax), min);

              // # Convert to int
              int intHue = round(hue);
              int intSat = round(sat);
              int intValue = round(value);

              QColor color;
              // # Ensure color components are not nan (not a number)
              if (!std::isnan(hue) && !std::isnan(sat) && !std::isnan(value)) {
                color = QColor::fromHsv(intHue, intSat, intValue);
              }

              if (color.isValid()) {
                // # Assign Color to output pixel
                outputImage.setPixel(w, h, color.rgba());
              }
            }
          }
        } else {
          throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                              "Active mode error");
          return;
        }

      } else if (
          dimension->property()->name() ==
          "de.uni_stuttgart.Voxie.DataProperty.ValueQuantity") {  // check
                                                                  // special
                                                                  // data types
        if (activeMode == ActiveMode::effZ) {
          // # extract slices from volumes
          QList<SliceImage> sliceList;
          QList<QSharedPointer<Data>> dataSeriesEntries =
              dataSeries->entries().values();
          for (int i = 0; i < dataSeriesEntries.size(); i++) {
            QSharedPointer<Data> channelData = dataSeriesEntries[i];
            QSharedPointer<VolumeData> channelDataVolume =
                qSharedPointerDynamicCast<VolumeData>(channelData);
            SliceImage sliceImage =
                generateSliceImage(channelDataVolume.data(), plane, sliceArea,
                                   outputImage.size(), interpolation);

            filterChain.applyTo(sliceImage);
            SliceImage filteredImage = filterChain.getOutputSlice();
            if (channelDataVolume) {
              // insert eff.Z channel at 0 & density at 1
              QString channelType = dimension->getEntryValue(i).toString();
              if (channelType ==
                  "de.uni_stuttgart.Voxie.Quantity.EffectiveAtomicNumber") {
                sliceList.insert(0, filteredImage);
              } else if (channelType ==
                         "de.uni_stuttgart.Voxie.Quantity.Density") {
                sliceList.insert(1, filteredImage);
              }
            }
          }

          for (int w = 0; w < outputImage.width(); w++) {
            for (int h = 0; h < outputImage.height(); h++) {
              // # get all values on position (w,h)

              const float actualEffZ =
                  ((SliceImage)sliceList.at(0)).getPixel(w, h);

              const float actualDensity =
                  ((SliceImage)sliceList.at(1)).getPixel(w, h);

              if (std::isnan(actualDensity) || std::isnan(actualEffZ)) {
                continue;
              }

              const float effZMin = widget->getEffZ_EffZMin();
              const float effZMax = widget->getEffZ_EffZMax();

              const float densityMin = widget->getEffZ_DensityMin();
              const float densityMax = widget->getEffZ_DensityMax();

              EffZStrategyOption effZStrat = widget->getEffZStrategy();

              const double min = 0.0;

              // hue: valuerange(int): 0° - 360°, in practice range -30° to
              // avoid confusion between very high and very low values
              double hue = 0.0;
              const double hueMax = 330.0;

              // saturation: valuerange(int): 0 - 255
              double sat = 0.0;
              double satMax = 255.0;

              // value: valuerange(int): 0 - 255
              double value = 0.0;
              const double valueMax = 255.0;

              if (effZStrat == EffZStrategyOption::EffZ2Hue_Density2Value) {
                hue = mapValueFromSourceScaleToTargetScale(
                    effZMin, effZMax, min, hueMax, actualEffZ);
                sat = satMax;
                value = mapValueFromSourceScaleToTargetScale(
                    densityMin, densityMax, min, valueMax, actualDensity);
              }

              if (effZStrat == EffZStrategyOption::EffZ2Hue_Density2Sat) {
                hue = mapValueFromSourceScaleToTargetScale(
                    effZMin, effZMax, min, hueMax, actualEffZ);
                sat = mapValueFromSourceScaleToTargetScale(
                    densityMin, densityMax, min, satMax, actualDensity);
                value = valueMax;
              }

              if (effZStrat == EffZStrategyOption::Density2Hue_EffZ2Sat) {
                hue = mapValueFromSourceScaleToTargetScale(
                    densityMin, densityMax, min, hueMax, actualDensity);
                sat = mapValueFromSourceScaleToTargetScale(
                    effZMin, effZMax, min, satMax, actualEffZ);
                value = valueMax;
              }

              if (effZStrat == EffZStrategyOption::EffZ2Value_Density2Sat) {
                hue = hueMax;
                sat = mapValueFromSourceScaleToTargetScale(
                    densityMin, densityMax, min, satMax, actualDensity);
                value = mapValueFromSourceScaleToTargetScale(
                    effZMin, effZMax, min, valueMax, actualEffZ);
              }

              // # Ensure color components are in their boundries
              hue = std::max(std::min(hue, hueMax), min);
              sat = std::max(std::min(sat, satMax), min);
              value = std::max(std::min(value, valueMax), min);

              const QColor color = QColor::fromHsv(
                  static_cast<int>(round(hue)), static_cast<int>(round(sat)),
                  static_cast<int>(round(value)));

              // # Assign Color to output pixel
              outputImage.setPixel(w, h, color.rgba());
            }
          }
        }
      }

    } else {
      // invalid data
      throw vx::Exception("de.uni_stuttgart.Voxie.Error", "data == nullptr");
    }
  }
}

void ImageLayer::sumUpColorsOverImages(QList<QPair<QImage, float>>* imageList,
                                       QImage* outputImage) {
  if (imageList->size() <= 0) {
    return;
  }

  if (imageList->size() == 1) {
    *outputImage = imageList->at(0).first;
    return;
  }

  // Per Pixel (w,h)
  for (int w = 0; w < outputImage->width(); w++) {
    for (int h = 0; h < outputImage->height(); h++) {
      vx::Vector<double, 4> result = {0.0, 0.0, 0.0, 0.0};

      for (QPair<QImage, float> slice : *imageList) {
        float sliceWeight = slice.second;

        vx::Color color = vx::Color::fromRgba(slice.first.pixel(w, h));

        // weight RGB components by alpha and channel slice weight
        result += color.premultiplied() * sliceWeight;
      }

      // de-weight RGB components by alpha
      for (size_t i = 0; i < 3; i++) {
        result[i] /= result[3];
      }

      const double min = 0.0;
      const double max = 1.0;

      // Clamp each color component
      for (size_t i = 0; i < 4; i++) {
        result[i] = std::max(std::min(result[i], max), min);
      }
      outputImage->setPixel(
          w, h,
          QColor::fromRgbF(result[0], result[1], result[2], result[3]).rgba());
    }
  }
};

QPair<double, double> ImageLayer::calcMean_StandardDeviation(
    QList<float>* data) {
  double sum = 0.0, mean = 0.0, variance = 0.0, standardDeviation = 0.0;
  const int size = data->size();
  double sumCount = 0.0;

  if (size > 0) {
    for (int i = 0; i < size; ++i) {
      if (!std::isnan(data->at(i))) {
        sum += data->at(i);
        sumCount++;
      }
    }

    mean = sumCount != 0.0 ? sum / sumCount : 0.0;
    double varCount = 0.0;

    for (int j = 0; j < size; ++j) {
      if (!std::isnan(data->at(j))) {
        variance += std::pow(data->at(j) - mean, 2);
        varCount++;
      }
    }
    standardDeviation = varCount != 0.0 ? std::sqrt(variance / varCount) : 0.0;

    return std::isnan(mean) || std::isnan(standardDeviation)
               ? QPair<float, float>(0.0, 0.0)
               : QPair<float, float>(mean, standardDeviation);

  } else {
    return QPair<float, float>(0.0, 0.0);
  }
}

QList<ColorizerEntry> ImageLayer::createColorEntries(QColor channelColor,
                                                     int channelMappingValue) {
  // Mapping to saturation
  QColor colorLow = QColor(0, 0, 0, 0);
  QColor colorHigh = channelColor;

  QList<ColorizerEntry> entryList;
  // Create color entries
  ColorizerEntry entryLow = ColorizerEntry(
      0.0, colorLow,
      ColorInterpolator(vx::ColorInterpolator::InterpolationType::RGB));
  entryList.append(entryLow);

  ColorizerEntry entryHigh = ColorizerEntry(
      channelMappingValue, colorHigh,
      ColorInterpolator(vx::ColorInterpolator::InterpolationType::RGB));
  entryList.append(entryHigh);
  return entryList;
}
