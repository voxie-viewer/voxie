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

#include "HistogramVisualizerWidget.hpp"

#include <VoxieBackend/Data/HistogramProvider.hpp>

#include <Voxie/Data/ColorizerEntry.hpp>

#include <QtCore/QVector>

#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QWidget>

namespace vx {
class ColorizerEntry;
class ImageDataPixel;

class VOXIECORESHARED_EXPORT HistogramWidget : public QWidget {
  Q_OBJECT

  // private attributes
 private:
  QSharedPointer<Colorizer> colorizer_ = QSharedPointer<Colorizer>(nullptr);

  HistogramVisualizerWidget* histogramWidget = nullptr;
  QSpinBox* spinUpperBound = nullptr;
  QSpinBox* spinLowerBound = nullptr;
  QCheckBox* maxYValueCheckBox = nullptr;
  QCheckBox* defaultWidgetHeightCheckbox = nullptr;
  QCheckBox* defaultHistoColorCheckbox = nullptr;
  QCheckBox* defaultBackgroundColorCheckbox = nullptr;
  QCheckBox* upperBoundCheckBox = nullptr;
  QCheckBox* lowerBoundCheckBox = nullptr;
  QSpinBox* spinMaxYValue = nullptr;
  QSpinBox* spinResizer = nullptr;
  QDialog* dialog = nullptr;
  QCheckBox* logCheckBox = nullptr;
  QCheckBox* colCheckBox = nullptr;
  QCheckBox* hoverSnappingCheckbox = nullptr;
  QPushButton* backgroundColorWidget;
  QPushButton* histoColorWidget;
  QColorDialog* colorPicker;

 public:
  HistogramWidget(QWidget* parent = 0);

  /**
   * @return Returns a pointer to the colorizer used by the widget to color the
   * histogram. May return a nullptr when no colorizer is set.
   */
  QSharedPointer<Colorizer> colorizer() { return colorizer_; }
  /**
   * @brief setColorizer Used to set the colorizer which should be used to color
   * the histogram bars if the "colorized" checkbox of the widget is activated.
   * @param colorizer Pointer to the colorizer that should be used. When passing
   * a nullptr the widget will disallow activating the "colorized" checkbox.
   */
  void setColorizer(QSharedPointer<Colorizer> colorizer);

  /**
   * @return Returns the HistogramProvider that this widget gets its histogram
   * data from.
   */
  QSharedPointer<HistogramProvider> histogramProvider() {
    return histogramWidget->histogramProvider();
  }
  /**
   * @brief setHistogramProvider Used to set the histogram provider that this
   * widget gets its histogram data from.
   * @param histogramProvider Pointer to the histogram provider to use.
   */
  void setHistogramProvider(
      QSharedPointer<HistogramProvider> histogramProvider) {
    histogramWidget->setHistogramProvider(histogramProvider);
  }

 protected:
  /**
   * @brief openSettingsDialog
   * creates or opens dialog for changing upper/lower bound and max value limit
   */
  void openSettingsDialog();

 private:
  /**
   * @brief resizeEvent
   * Fuction that gets called when the widget gets resized.
   * @param ev
   */
  void resizeEvent(QResizeEvent* ev) override;

  /**
   * @brief colorizerChanged Used to update the HistogramVisualizerWidget's
   * colorizer when the colorizer or the checkbox for activating colorization
   * change.
   */
  void colorizerChanged();
};
}  // namespace vx
