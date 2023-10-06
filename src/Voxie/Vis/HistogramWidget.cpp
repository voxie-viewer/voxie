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

#include "HistogramWidget.hpp"
#include "HistogramVisualizerWidget.hpp"

#include <Voxie/Data/ColorizerEntry.hpp>
#include <VoxieBackend/Data/ImageDataPixel.hpp>
#include <VoxieBackend/Data/ImageDataPixelInst.hpp>

#include <Voxie/Gui/MakeHandButton.hpp>

#include <cmath>

#include <QtCore/QDebug>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QThreadPool>

#include <QtGui/QFont>
#include <QtGui/QMouseEvent>

#include <QtWidgets/QAction>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QVBoxLayout>

using namespace vx;

HistogramWidget::HistogramWidget(QWidget* parent) : QWidget(parent) {
  QString name = "Histogram";
  this->setWindowTitle(name);
  QVBoxLayout* topLayout = new QVBoxLayout(this);

  // set up the HistogramVisualizerWidget; This widget does the heavy lifting
  // for actually drawing the histogram
  this->histogramWidget = new HistogramVisualizerWidget(this);
  this->histogramWidget->setMinimumHeight(200 / 96.0 * this->logicalDpiY());

  //--- Toolbar with settings-button , enable/diable log-view etc. ---
  QToolBar* toolbar = new QToolBar(this);

  //--- Settings Button---
  QAction* settingsButton =
      toolbar->addAction(QIcon(":/icons/gear.png"), "Settings");
  connect(settingsButton, &QAction::triggered, this,
          &HistogramWidget::openSettingsDialog);

  //--- "logarithmic" check Box ---
  this->logCheckBox = new QCheckBox("logarithmic", this);
  this->logCheckBox->setToolTip(
      "Switches between linear mode and logarithmic mode");
  connect(logCheckBox, &QCheckBox::stateChanged, histogramWidget,
          &HistogramVisualizerWidget::setYAxisLogScale);
  toolbar->addWidget(logCheckBox);

  //--- "colorized" check Box ---
  this->colCheckBox = new QCheckBox("colorized", this);
  this->colCheckBox->setToolTip(
      "Switches between standard and multi-color mode.");
  // when the checkbox is (un)checked we have to refresh the
  // HistogramVisualizerWidget's colorizer
  connect(colCheckBox, &QCheckBox::stateChanged, this,
          &HistogramWidget::colorizerChanged);
  toolbar->addWidget(colCheckBox);

  // "hover value snapping" check box
  hoverSnappingCheckbox = new QCheckBox("hover snapping", this);
  hoverSnappingCheckbox->setToolTip(
      "When checked the y value when hovering will snap to the value of the "
      "hovered histogram bar.");
  connect(hoverSnappingCheckbox, &QCheckBox::stateChanged, this, [&]() {
    histogramWidget->setHoverValueSnappingEnabled(
        hoverSnappingCheckbox->isChecked());
  });
  toolbar->addWidget(hoverSnappingCheckbox);

  //--- put all together ---
  topLayout->addWidget(this->histogramWidget);
  topLayout->addWidget(toolbar);

  // checkBoxes for settings dialog are created here so that program doesnt
  // crash  when drawing histogram and settings dialog is not created yet. Also,
  // the checkState of the maxYValueCheckBox is important for the calculation of
  // the histogram`s left edge
  this->maxYValueCheckBox = new QCheckBox("automatic");
  this->maxYValueCheckBox->setToolTip(
      "Enable to automatically assign the upper bound of the Y-Axis");
  this->maxYValueCheckBox->setCheckState(Qt::Checked);
  this->upperBoundCheckBox = new QCheckBox("automatic");
  this->upperBoundCheckBox->setToolTip(
      "Enable to automatically assign the upper bound of the X-Axis");
  this->upperBoundCheckBox->setCheckState(Qt::Checked);
  this->lowerBoundCheckBox = new QCheckBox("automatic");
  this->lowerBoundCheckBox->setToolTip(
      "Enable to automatically assign the lower bound of the X-Axis");
  this->lowerBoundCheckBox->setCheckState(Qt::Checked);
  this->defaultHistoColorCheckbox = new QCheckBox("default");
  this->defaultHistoColorCheckbox->setToolTip(
      "Uncheck to choose a custom foreground color.");
  this->defaultHistoColorCheckbox->setCheckState(Qt::Checked);
  this->defaultBackgroundColorCheckbox = new QCheckBox("default");
  this->defaultBackgroundColorCheckbox->setToolTip(
      "Uncheck to choose a custom background color.");
  this->defaultBackgroundColorCheckbox->setCheckState(Qt::Checked);

  // Make sure maxYValueCheckBox is destroyed even if no settings dialog was
  // created
  QPointer<QCheckBox> maxYValueCheckBoxPtr(maxYValueCheckBox);
  connect(this, &QObject::destroyed, [maxYValueCheckBoxPtr] {
    if (maxYValueCheckBoxPtr) delete maxYValueCheckBoxPtr;
  });
  // Make sure upperBoundCheckBox is destroyed even if no settings dialog was
  // created
  QPointer<QCheckBox> upperBoundCheckBoxPtr(upperBoundCheckBox);
  connect(this, &QObject::destroyed, [upperBoundCheckBoxPtr] {
    if (upperBoundCheckBoxPtr) delete upperBoundCheckBoxPtr;
  });
  // Make sure lowerBoundCheckBox is destroyed even if no settings dialog was
  // created
  QPointer<QCheckBox> lowerBoundCheckBoxPtr(lowerBoundCheckBox);
  connect(this, &QObject::destroyed, [lowerBoundCheckBoxPtr] {
    if (lowerBoundCheckBoxPtr) delete lowerBoundCheckBoxPtr;
  });
}

void HistogramWidget::setColorizer(QSharedPointer<Colorizer> colorizer) {
  colorizer_ = colorizer;

  colorizerChanged();
}

// opens the settings dialog of the widget as seperate window. The settings
// dialog is also defined here
void HistogramWidget::openSettingsDialog() {
  // check if dialog was already created previously, then we can skip this part
  if (this->dialog == nullptr) {
    this->dialog = new QDialog(this);
    this->dialog->setWindowTitle("Histogram settings - Voxie");
    QVBoxLayout* topLayout = new QVBoxLayout();

    // x-axis lower bound controls
    QHBoxLayout* layout1 = new QHBoxLayout();
    QLabel* labelLowBound = new QLabel("Lower bucket bound", this);
    labelLowBound->setToolTip(
        "Change the lowest bucket index that should be displayed on the "
        "histogram; can be used to view a specific part of the histogram");
    labelLowBound->setMinimumWidth(150 / 96.0 * this->logicalDpiX());
    layout1->addWidget(labelLowBound);
    this->spinLowerBound = new QSpinBox();
    spinLowerBound->setEnabled(false);
    this->spinLowerBound->setToolTip("Enter a number, e.g. '5'");
    this->spinLowerBound->setMinimum(0);
    this->spinLowerBound->setMaximum(std::numeric_limits<int>::max());
    this->spinLowerBound->setMinimumWidth(175 / 96.0 * this->logicalDpiX());
    spinLowerBound->setValue(0);
    connect(lowerBoundCheckBox, &QCheckBox::stateChanged, this, [&]() {
      spinLowerBound->setEnabled(!lowerBoundCheckBox->isChecked());
      histogramWidget->setBucketLowerBoundOverrideValue(
          spinLowerBound->value());
      histogramWidget->setAutomaticBucketLowerBound(
          lowerBoundCheckBox->isChecked());
    });
    QObject::connect(
        spinLowerBound,
        static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
        [&](int v) {
          histogramWidget->setBucketLowerBoundOverrideValue(v);
          spinUpperBound->setMinimum(v + 1);
        });
    layout1->addWidget(this->spinLowerBound);
    layout1->addWidget(this->lowerBoundCheckBox, 0, Qt::AlignLeft);
    topLayout->addLayout(layout1);

    // x-axis upper bound controls
    QHBoxLayout* layout2 = new QHBoxLayout();
    QLabel* labelUpperBound = new QLabel("Upper bucket bound", this);
    labelUpperBound->setToolTip(
        "Change the highest bucket index that should be displayed on the "
        "histogram; can be used to view a specific part of the histogram");
    labelUpperBound->setMinimumWidth(150 / 96.0 * this->logicalDpiX());
    layout2->addWidget(labelUpperBound);
    spinUpperBound = new QSpinBox();
    spinUpperBound->setEnabled(false);
    spinUpperBound->setToolTip("Enter a number, e.g. '10'");
    spinUpperBound->setMinimum(1);
    spinUpperBound->setMaximum(std::numeric_limits<int>::max());
    spinUpperBound->setMinimumWidth(175 / 96.0 * this->logicalDpiX());
    spinUpperBound->setValue(
        !histogramProvider().isNull()
            ? histogramProvider()->getData()->buckets.size()
            : 1);
    connect(upperBoundCheckBox, &QCheckBox::stateChanged, this, [&]() {
      spinUpperBound->setEnabled(!upperBoundCheckBox->isChecked());
      histogramWidget->setBucketUpperBoundOverrideValue(
          spinUpperBound->value());
      histogramWidget->setAutomaticBucketUpperBound(
          upperBoundCheckBox->isChecked());
    });
    QObject::connect(
        spinUpperBound,
        static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
        [&](int v) {
          histogramWidget->setBucketUpperBoundOverrideValue(v);
          spinLowerBound->setMaximum(v - 1);
        });
    layout2->addWidget(this->spinUpperBound);
    layout2->addWidget(this->upperBoundCheckBox, 0, Qt::AlignLeft);
    topLayout->addLayout(layout2);

    // y-axis upper bound controls
    QHBoxLayout* layout3 = new QHBoxLayout();
    // checkBox is created in constructor
    this->spinMaxYValue = new QSpinBox();
    spinMaxYValue->setEnabled(false);
    this->spinMaxYValue->setMinimum(1);
    this->spinMaxYValue->setMaximum(std::numeric_limits<int>::max());
    this->spinMaxYValue->setToolTip("Enter a number, e.g. '150'");
    this->spinMaxYValue->setSingleStep(5);
    this->spinMaxYValue->setMinimumWidth(175 / 96.0 * this->logicalDpiX());
    if (!histogramWidget->histogramProvider().isNull()) {
      this->spinMaxYValue->setValue(
          histogramWidget->histogramProvider()->getData()->maximumCount);
    } else {
      spinMaxYValue->setValue(1);
    }
    connect(this->maxYValueCheckBox, &QCheckBox::stateChanged, this, [&]() {
      spinMaxYValue->setEnabled(!maxYValueCheckBox->isChecked());
      histogramWidget->setUpperBoundOverride(spinMaxYValue->value());
      histogramWidget->setAutomaticUpperBound(maxYValueCheckBox->isChecked());
    });
    QObject::connect(
        spinMaxYValue,
        static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
        [&]() {
          histogramWidget->setUpperBoundOverride(spinMaxYValue->value());
        });

    QLabel* labelYUpperBound = new QLabel("Y-Axis: Upper bound", this);
    labelYUpperBound->setToolTip(
        "The Y-Axis of the histogram represents the amount of pixels in the "
        "image matching to specific values.");
    labelYUpperBound->setMinimumWidth(150 / 96.0 * this->logicalDpiX());
    layout3->addWidget(labelYUpperBound);
    layout3->addWidget(this->spinMaxYValue);
    layout3->addWidget(this->maxYValueCheckBox, 0, Qt::AlignLeft);

    topLayout->addLayout(layout3);

    QHBoxLayout* layout4 = new QHBoxLayout();
    // checkBox is created in constructor
    spinResizer = new QSpinBox();
    spinResizer->setEnabled(false);
    spinResizer->setMinimum(150 / 96.0 * this->logicalDpiY());
    spinResizer->setMaximum(500 / 96.0 * this->logicalDpiY());
    // spinResizer->setToolTip("Enter a number between '150' and '500'.");
    spinResizer->setSingleStep(25);
    spinResizer->setMinimumWidth(175 / 96.0 * this->logicalDpiX());
    spinResizer->setValue(this->histogramWidget->height());
    defaultWidgetHeightCheckbox = new QCheckBox("default");
    defaultWidgetHeightCheckbox->setToolTip("Enable to use the default value");
    defaultWidgetHeightCheckbox->setCheckState(Qt::Checked);
    QLabel* resizerLabel = new QLabel("Height: Pixels", this);
    resizerLabel->setToolTip("The height of the histogram in pixels.");
    resizerLabel->setMinimumWidth(150 / 96.0 * this->logicalDpiX());
    layout4->addWidget(resizerLabel);
    layout4->addWidget(spinResizer);
    layout4->addWidget(defaultWidgetHeightCheckbox);
    connect(defaultWidgetHeightCheckbox, &QCheckBox::stateChanged, this, [&]() {
      spinResizer->setEnabled(!defaultWidgetHeightCheckbox->isChecked());
      histogramWidget->setMinimumHeight(
          defaultWidgetHeightCheckbox->isChecked()
              ? 200 / 96.0 * this->logicalDpiY()
              // TODO: High-DPI: Also scale value from input field? Then the
              // minimum / maximum values should not depend on logicalDpiY.
              : spinResizer->value());
    });
    QObject::connect(
        spinResizer,
        static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
        [&]() { histogramWidget->setMinimumHeight(spinResizer->value()); });

    topLayout->addLayout(layout4);

    // color widgets
    this->backgroundColorWidget = new MakeHandButton();
    this->backgroundColorWidget->setToolTip("Choose Color");
    this->backgroundColorWidget->setMinimumWidth(175 / 96.0 *
                                                 this->logicalDpiX());
    this->backgroundColorWidget->setAutoDefault(false);
    this->backgroundColorWidget->setStyleSheet(
        "QWidget { background-color: " +
        histogramWidget->backgroundColor().name() +
        " } QPushButton { border: 1px solid lightGray }");
    this->histoColorWidget = new MakeHandButton();
    this->histoColorWidget->setToolTip("Choose Color");
    this->histoColorWidget->setAutoDefault(false);
    this->histoColorWidget->setMinimumWidth(175 / 96.0 * this->logicalDpiX());
    this->histoColorWidget->setStyleSheet(
        "QWidget { background-color: " +
        histogramWidget->foregroundColor().name() +
        " } QPushButton { border: 1px solid lightGray }");

    // color dialog
    this->colorPicker = new QColorDialog(this->dialog);
    this->colorPicker->setOption(QColorDialog::ShowAlphaChannel);

    connect(this->backgroundColorWidget, &MakeHandButton::clicked,
            [=]() -> void {
              this->colorPicker->setCurrentColor(
                  histogramWidget->backgroundColor());
              this->colorPicker->exec();
              QColor col = this->colorPicker->selectedColor();
              if (col.isValid()) {
                this->defaultBackgroundColorCheckbox->setChecked(false);
                histogramWidget->setBackgroundColor(
                    this->colorPicker->selectedColor());
                this->backgroundColorWidget->setStyleSheet(
                    "QWidget {background: " +
                    histogramWidget->backgroundColor().name() +
                    "} QPushButton {border: 1px solid lightGray}");
              }
            });

    connect(this->histoColorWidget, &MakeHandButton::clicked, [=]() -> void {
      this->colorPicker->setCurrentColor(histogramWidget->foregroundColor());
      this->colorPicker->exec();
      QColor col = this->colorPicker->selectedColor();
      if (col.isValid()) {
        this->defaultHistoColorCheckbox->setChecked(false);
        histogramWidget->setForegroundColor(this->colorPicker->selectedColor());
        this->histoColorWidget->setStyleSheet(
            "QWidget {background: " +
            histogramWidget->foregroundColor().name() +
            "} QPushButton {border: 1px solid lightGray}");
      }
    });

    QHBoxLayout* layout5 = new QHBoxLayout();
    QLabel* backgroundColLabel = new QLabel("Color: Background", this);
    backgroundColLabel->setToolTip("The background color of the histogram.");
    backgroundColLabel->setMinimumWidth(150 / 96.0 * this->logicalDpiX());
    layout5->addWidget(backgroundColLabel);
    layout5->addWidget(this->backgroundColorWidget);
    layout5->addWidget(this->defaultBackgroundColorCheckbox, 0, Qt::AlignLeft);
    connect(defaultBackgroundColorCheckbox, &QCheckBox::stateChanged, this,
            [&]() {
              if (defaultBackgroundColorCheckbox->isChecked())
                histogramWidget->setBackgroundColor(
                    HistogramVisualizerWidget::defaultBackgroundColor());

              backgroundColorWidget->setStyleSheet(
                  "QWidget {background: " +
                  histogramWidget->backgroundColor().name() +
                  "} QPushButton {border: 1px solid lightGray}");
            });

    QHBoxLayout* layout6 = new QHBoxLayout();
    QLabel* foregroundColLabel = new QLabel("Color: Histogram", this);
    foregroundColLabel->setToolTip(
        "The color of the histogram when colorized mode is unchecked.");
    foregroundColLabel->setMinimumWidth(150 / 96.0 * this->logicalDpiX());

    layout6->addWidget(foregroundColLabel);
    layout6->addWidget(this->histoColorWidget);
    layout6->addWidget(this->defaultHistoColorCheckbox, 0, Qt::AlignLeft);
    connect(defaultHistoColorCheckbox, &QCheckBox::stateChanged, this, [&]() {
      if (defaultHistoColorCheckbox->isChecked())
        histogramWidget->setForegroundColor(
            HistogramVisualizerWidget::defaultForegroundColor());

      histoColorWidget->setStyleSheet(
          "QWidget {background: " + histogramWidget->foregroundColor().name() +
          "} QPushButton {border: 1px solid lightGray}");
    });

    topLayout->addLayout(layout6);
    topLayout->addLayout(layout5);

    QPushButton* okButton = new QPushButton("Ok");
    okButton->setAutoDefault(false);
    connect(okButton, &QPushButton::clicked, this, [=]() { dialog->accept(); });
    topLayout->addWidget(okButton);

    this->dialog->setLayout(topLayout);
  }

  dialog->exec();
}

void HistogramWidget::resizeEvent(QResizeEvent* ev) {
  QWidget::resizeEvent(ev);

  // repaint histogram
  // emit this->histogramSettingsChanged();
}

void HistogramWidget::colorizerChanged() {
  if (colCheckBox->isChecked())
    histogramWidget->setColorizer(colorizer_);
  else
    histogramWidget->setColorizer(QSharedPointer<Colorizer>(nullptr));
}
