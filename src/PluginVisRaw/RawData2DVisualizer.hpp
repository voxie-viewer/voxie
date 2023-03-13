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

#include <VoxieBackend/Data/InterpolationMethod.hpp>

#include <PluginVisRaw/Prototypes.forward.hpp>
#include <PluginVisRaw/RawImageCache.hpp>

#include <Voxie/Vis/HistogramWidget.hpp>
#include <Voxie/Vis/VisualizerNode.hpp>
#include <Voxie/Vis/VisualizerView.hpp>

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QVarLengthArray>

#include <QtGui/QPaintEvent>

#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>

class InfoWidget;
class ImageSelectionWidget;

namespace vx {
class InitializeColorizeWorker;
class ColorizerEntry;
}  // namespace vx

/**
 * @brief The RawVisualizer class is a godclass mainly responsible for
 * initializing and handling the data flow between all slice view operations.
 */

class RawVisualizer : public vx::visualization::SimpleVisualizer {
  Q_OBJECT

 public:
  vx::visualizer_prop::TomographyRawDataProperties* properties;

 private:
  bool setupFinishedCalled = false;

  QComboBox* box;

  // sidebar widgets
  InfoWidget* info;
  vx::HistogramWidget* _histogramWidget;
  ImageSelectionWidget* imageSelectionWidget;
  QPushButton* showPerImageMetadata;
  QPushButton* showImageKind;

  void updateCachedImage();

  static std::tuple<QString, quint64, QJsonObject> getStreamAndImage(
      const QSharedPointer<vx::TomographyRawData2DAccessor>& data,
      const std::tuple<QString, QJsonObject>& imageList, quint64 listPos);
  std::tuple<QString, quint64, QJsonObject> getStreamAndImage(
      const std::tuple<QString, QJsonObject>& imageList, quint64 listPos);
  std::tuple<QString, quint64, QJsonObject> getCurrentStreamAndImage();

  QSharedPointer<vx::HistogramProvider> histogramProvider;

  QMap<int, QImage> _drawStack;

  // for window elements
  QVBoxLayout* hobox;

  /**
   * Handles resizes. We don't want to regenerate an image on each resize event
   * as this would need unnecessary amount of resources
   */
  QTimer _resizeTimer;

 public:  // TODO
  QSharedPointer<RawImageCache> cache;

 private:
  QSharedPointer<vx::ImageDataPixel> cachedImage;

  vx::InitializeColorizeWorker* initializeWorker;

  // For mouse actions
  QPoint dragStart;
  bool dragStartValid = false;

 public:
  RawVisualizer();
  //~RawVisualizer();

  FACTORY_VISUALIZERMODULE_HPP(Raw)

  vx::SharedFunPtr<RenderFunction> getRenderFunction() override;

  void setupFinished() override;

  /**
   * @return the current plane area describing the current position and size of
   * the sliceimage inside the visualizer
   */
  static QRectF getCurrentPlaneArea(
      vx::visualizer_prop::TomographyRawDataPropertiesBase* properties,
      const QSize& canvasSize);

  /**
   * @param multiplier zooms the plane area by a given delta. Multiplier is
   * multiplied with current zoom. e.g.
   */
  void zoomPlaneArea(qreal multiplier);

  /**
   * Moves the current plane (representing the position of the image inside the
   * canvas) by the given values
   * @param pixelDeltaX delta x
   * @param pixelDeltaX delta y
   */
  void moveArea(  // vx::TomographyRawDataPropertiesBase* properties,const
                  // QSize& canvasSize,
      qreal pixelDeltaX, qreal pixelDeltaY);

  /**
   * Return the current data or nullptr if no object is connected or the data
   * object does not contain any data.
   */
  QSharedPointer<vx::TomographyRawData2DAccessor> data();

  /**
   * Exposes the histogram provider to the Colorizer widget
   */
  QSharedPointer<QObject> getPropertyUIData(QString propertyName) override;

  bool isUpToDate();

  quint64 getCurrentImageListCount();

 protected:
  QWidget* getCustomPropertySectionContent(const QString& name) override;

 Q_SIGNALS:
  void signalRequestHistogram(const QSharedPointer<vx::ImageDataPixel>& image);

  // Signals forwarded from RawDataObject
  void rawDataChangedFinished();
  void rawDataDisplayNameChanged();

 public:  // TODO
  static QRectF getProjectionArea(
      const QSharedPointer<vx::TomographyRawData2DAccessor>& data,
      const QString& stream, qint64 imageId,
      const QJsonObject& projectionGeometry);

 private:
  void selectInitialImageKindAndList();
};
