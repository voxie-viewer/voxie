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

#include <Voxie/Data/Colorizer.hpp>
#include <VoxieBackend/Data/FloatImage.hpp>

#include <Voxie/Gui/ColorizerGradientWidget.hpp>

#include <QtGui/QImage>
#include <QtGui/QRgb>

#include <QtWidgets/QColorDialog>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QToolButton>

/**
 * @brief The ColorizerWidget class provides a WSYIWYG interface for
 * manipulating a colorizer. The current colorization function is displayed as a
 * gradient.
 * @author Hans Martin Berner
 * @author Adrian Zeyfang
 */
class VOXIECORESHARED_EXPORT ColorizerWidget : public QWidget {
  Q_OBJECT
 public:
  ColorizerWidget(QWidget* parent = 0);
  virtual ~ColorizerWidget();

  void setColorizer(QSharedPointer<vx::Colorizer> colorizer);
  QSharedPointer<vx::Colorizer> getColorizer() const;

  void setHistogramProvider(
      QSharedPointer<vx::HistogramProvider> histogramProvider);
  QSharedPointer<vx::HistogramProvider> getHistogramProvider() const;

  QSize sizeHint() const override;

 private:
  enum class Template {
    Custom,
    CoolToWarm,
    OneHot,
  };

  Template getSelectedTemplate() const;

  void autoFitEntries();

  void updateAll();

  void updateButtonEnableStates();
  void updateCurrentColorMapping();
  void updateColorSelectionButton();
  void updateInterpolatorButton();
  void updateTemplateControls();

  void updateTemplate();

  QColorDialog* colorPicker;
  ColorizerGradientWidget* gradientWidget;

  QToolButton* addEntryButton;
  QToolButton* removeEntryButton;
  QToolButton* prevEntryButton;
  QToolButton* nextEntryButton;
  QDoubleSpinBox* currentValueInput;
  QToolButton* currentColorButton;
  QToolButton* currentInterpolatorButton;

  QToolButton* autoFitEntriesButton;
  QToolButton* autoRescaleToggle;
  QComboBox* templateSelector;
  QLabel* templateParameterLabel;
  QDoubleSpinBox* templateParameterInput;

  bool isUpdatingTemplate = false;
};
