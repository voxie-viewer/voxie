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

#include <Voxie/Gui/ColorizerWidget.hpp>

#include <Voxie/Data/ColorizerEntry.hpp>
#include <VoxieBackend/Data/VolumeDataVoxel.hpp>
#include <VoxieBackend/Data/VolumeDataVoxelInst.hpp>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QListWidgetItem>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include <cmath>

// TODO: This (or possible some other code) currently will always drop the NaN
// entry, this should be fixed

using namespace vx;

static std::map<vx::ColorInterpolator::InterpolationType, QString>
    interpolators{
        {vx::ColorInterpolator::RGB, "RGB"},
        {vx::ColorInterpolator::LAB, "LAB"},
        {vx::ColorInterpolator::MSH, "MSH"},
        {vx::ColorInterpolator::MSHDiverging, "MSH*"},
        {vx::ColorInterpolator::Right, "Right"},
        {vx::ColorInterpolator::Left, "Left"},
    };

ColorizerWidget::ColorizerWidget(QWidget* parent) : QWidget(parent) {
  this->setWindowTitle("Colorizer");

  auto layout = new QVBoxLayout;
  this->setLayout(layout);

  gradientWidget = new ColorizerGradientWidget;
  colorPicker = new QColorDialog;
  colorPicker->setOption(QColorDialog::NoButtons);
  colorPicker->setOption(QColorDialog::ShowAlphaChannel);

  auto toolbarTop = new QToolBar;
  auto toolbarBottom = new QToolBar;

  auto getLockUnlockIcon = [](bool locked) {
    return QIcon(locked ? ":/icons/lock.png" : ":/icons/lock-unlock.png");
  };

  layout->addWidget(gradientWidget);
  layout->addWidget(toolbarTop);
  layout->addWidget(toolbarBottom);

  addEntryButton = new QToolButton;
  addEntryButton->setIcon(QIcon(":/icons/plus.png"));
  addEntryButton->setToolTip(tr("Add mapping entry"));
  toolbarTop->addWidget(addEntryButton);

  removeEntryButton = new QToolButton;
  removeEntryButton->setIcon(QIcon(":/icons/minus.png"));
  removeEntryButton->setToolTip(tr("Remove mapping entry"));
  toolbarTop->addWidget(removeEntryButton);

  prevEntryButton = new QToolButton;
  prevEntryButton->setIcon(QIcon(":/icons/arrow-180.png"));
  prevEntryButton->setToolTip(tr("Previous mapping entry"));
  toolbarTop->addWidget(prevEntryButton);

  nextEntryButton = new QToolButton;
  nextEntryButton->setIcon(QIcon(":/icons/arrow.png"));
  nextEntryButton->setToolTip(tr("Next mapping entry"));
  toolbarTop->addWidget(nextEntryButton);

  toolbarTop->addSeparator();

  currentValueInput = new QDoubleSpinBox;
  currentValueInput->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  currentValueInput->setMinimum(-std::numeric_limits<double>::infinity());
  currentValueInput->setMaximum(std::numeric_limits<double>::infinity());
  // TODO: Use UnitSpinBox here?
  currentValueInput->setDecimals(6);
  toolbarTop->addWidget(currentValueInput);

  toolbarTop->addSeparator();

  currentColorButton = new QToolButton;
  currentColorButton->setToolTip(tr("Select color"));
  toolbarTop->addWidget(currentColorButton);

  toolbarTop->addSeparator();

  auto interpolatorMenu = new QMenu;
  for (auto interpolator : interpolators) {
    connect(interpolatorMenu->addAction(interpolator.second),
            &QAction::triggered, this, [this, interpolator]() {
              gradientWidget->setSelectedEntryInterpolator(interpolator.first);
              updateInterpolatorButton();
            });
  }

  currentInterpolatorButton = new QToolButton;
  currentInterpolatorButton->setText(tr("MSH*"));
  currentInterpolatorButton->setToolTip(tr("Select interpolation method"));
  toolbarTop->addWidget(currentInterpolatorButton);

  autoFitEntriesButton = new QToolButton;
  autoFitEntriesButton->setIcon(
      QIcon(":/icons/edit-alignment-justify-distribute.png"));
  autoFitEntriesButton->setToolTip(tr("Fit color map to data"));
  toolbarBottom->addWidget(autoFitEntriesButton);

  templateSelector = new QComboBox;
  templateSelector->addItem(tr("Custom"), int(Template::Custom));
  templateSelector->addItem(tr("Cool to warm"), int(Template::CoolToWarm));
  templateSelector->addItem(tr("One-hot"), int(Template::OneHot));
  toolbarBottom->addWidget(templateSelector);

  templateParameterLabel = new QLabel;
  toolbarBottom->addWidget(templateParameterLabel);

  templateParameterInput = new QDoubleSpinBox;
  templateParameterInput->setSizePolicy(QSizePolicy::Expanding,
                                        QSizePolicy::Fixed);
  templateParameterInput->setMinimum(-std::numeric_limits<double>::infinity());
  templateParameterInput->setMaximum(std::numeric_limits<double>::infinity());
  templateParameterInput->setValue(100.0);
  toolbarBottom->addWidget(templateParameterInput);

  autoRescaleToggle = new QToolButton;
  autoRescaleToggle->setCheckable(true);
  autoRescaleToggle->setIcon(getLockUnlockIcon(false));
  autoRescaleToggle->setToolTip(tr("Automatically rescale color map to data"));
  toolbarBottom->addWidget(autoRescaleToggle);

  connect(addEntryButton, &QToolButton::clicked, this,
          [this]() { gradientWidget->addEntry(); });

  connect(removeEntryButton, &QToolButton::clicked, gradientWidget,
          &ColorizerGradientWidget::removeSelectedEntry);

  connect(nextEntryButton, &QToolButton::clicked, gradientWidget,
          &ColorizerGradientWidget::nextEntry);

  connect(prevEntryButton, &QToolButton::clicked, gradientWidget,
          &ColorizerGradientWidget::previousEntry);

  connect(autoRescaleToggle, &QToolButton::toggled, this, [=](bool checked) {
    gradientWidget->setAutoRescaling(checked);
    autoRescaleToggle->setIcon(getLockUnlockIcon(checked));
  });

  connect(autoFitEntriesButton, &QToolButton::clicked, this,
          &ColorizerWidget::autoFitEntries);

  connect(currentValueInput,
          static_cast<void (QDoubleSpinBox::*)(double)>(
              &QDoubleSpinBox::valueChanged),
          gradientWidget, &ColorizerGradientWidget::setSelectedEntryValue);

  connect(currentColorButton, &QToolButton::clicked, colorPicker,
          &QColorDialog::show);

  connect(colorPicker, &QColorDialog::currentColorChanged, this,
          [=](QColor color) {
            gradientWidget->setSelectedEntryColor(color);
            updateColorSelectionButton();
          });

  connect(currentInterpolatorButton, &QToolButton::clicked, this, [=] {
    interpolatorMenu->popup(currentInterpolatorButton->mapToGlobal(
        QPoint(0, currentInterpolatorButton->height())));
  });

  connect(
      templateSelector,
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this, [this] {
        updateTemplateControls();
        updateTemplate();
      });

  connect(templateParameterInput,
          static_cast<void (QDoubleSpinBox::*)(double)>(
              &QDoubleSpinBox::valueChanged),
          this, &ColorizerWidget::updateTemplate);

  connect(gradientWidget, &ColorizerGradientWidget::selectedIndexChanged, this,
          &ColorizerWidget::updateAll);

  connect(gradientWidget, &ColorizerGradientWidget::selectedEntryValueChanged,
          this, &ColorizerWidget::updateAll);

  connect(gradientWidget, &ColorizerGradientWidget::boundsChanged, this,
          [=] { updateCurrentColorMapping(); });

  connect(gradientWidget, &ColorizerGradientWidget::entryDoubleClicked,
          colorPicker, &QColorDialog::show);
}

ColorizerWidget::~ColorizerWidget() {}

void ColorizerWidget::setColorizer(QSharedPointer<vx::Colorizer> colorizer) {
  if (getColorizer()) {
    disconnect(getColorizer().data(), nullptr, this, nullptr);
  }

  gradientWidget->setColorizer(colorizer);

  if (getColorizer()) {
    connect(getColorizer().data(), &Colorizer::mappingChanged, this, [=] {
      if (!isUpdatingTemplate) {
        templateSelector->setCurrentIndex(
            templateSelector->findData(int(Template::Custom)));
      }
    });
  }

  updateAll();
}

QSharedPointer<vx::Colorizer> ColorizerWidget::getColorizer() const {
  return gradientWidget->getColorizer();
}

void ColorizerWidget::setHistogramProvider(
    QSharedPointer<HistogramProvider> histogramProvider) {
  gradientWidget->setHistogramProvider(histogramProvider);
}

QSharedPointer<HistogramProvider> ColorizerWidget::getHistogramProvider()
    const {
  return gradientWidget->getHistogramProvider();
}

QSize ColorizerWidget::sizeHint() const { return QSize(100, 40); }

ColorizerWidget::Template ColorizerWidget::getSelectedTemplate() const {
  return Template(templateSelector->currentIndex());
}

void ColorizerWidget::autoFitEntries() {
  if (getColorizer()->getEntryCount() > 1) {
    getColorizer()->rescale(getColorizer()->getEntries().front().value(),
                            getColorizer()->getEntries().back().value(),
                            gradientWidget->getMinimumValue(),
                            gradientWidget->getMaximumValue());
    gradientWidget->update();
  }
}

void ColorizerWidget::updateAll() {
  updateButtonEnableStates();
  updateCurrentColorMapping();
  updateColorSelectionButton();
  updateInterpolatorButton();
  updateTemplateControls();
}

void ColorizerWidget::updateButtonEnableStates() {
  removeEntryButton->setEnabled(gradientWidget->hasSelectedEntry());
  prevEntryButton->setEnabled(!gradientWidget->hasSelectedEntry() ||
                              gradientWidget->getSelectedIndex() > 0);
  nextEntryButton->setEnabled(!gradientWidget->hasSelectedEntry() ||
                              gradientWidget->getSelectedIndex() <
                                  gradientWidget->getEntryCount() - 1);

  currentValueInput->setEnabled(gradientWidget->hasSelectedEntry());
  currentColorButton->setEnabled(gradientWidget->hasSelectedEntry());
  currentInterpolatorButton->setEnabled(
      gradientWidget->hasSelectedEntry() &&
      gradientWidget->getSelectedIndex() < gradientWidget->getEntryCount() - 1);
}

void ColorizerWidget::updateCurrentColorMapping() {
  if (gradientWidget->hasSelectedEntry()) {
    colorPicker->blockSignals(true);
    colorPicker->setCurrentColor(
        gradientWidget->getSelectedEntry().color().asQColor());
    colorPicker->blockSignals(false);

    currentValueInput->blockSignals(true);
    currentValueInput->setSingleStep((gradientWidget->getMaximumValue() -
                                      gradientWidget->getMinimumValue()) *
                                     0.02);
    currentValueInput->setValue(gradientWidget->getSelectedEntry().value());
    currentValueInput->blockSignals(false);
  }
}

void ColorizerWidget::updateColorSelectionButton() {
  if (gradientWidget->hasSelectedEntry()) {
    QColor color = gradientWidget->getSelectedEntry().color().asQColor();
    currentColorButton->setStyleSheet(
        "QWidget { border: 1px solid lightGray; background-color: " +
        color.name() + "; }");
  }
}

void ColorizerWidget::updateInterpolatorButton() {
  auto interpolator = interpolators.find(
      gradientWidget->getSelectedEntry().interpolator().getType());
  if (interpolator != interpolators.end()) {
    currentInterpolatorButton->setText(interpolator->second);
  }
}

void ColorizerWidget::updateTemplateControls() {
  Template selectedTemplate = getSelectedTemplate();
  templateParameterLabel->setEnabled(selectedTemplate != Template::Custom);
  templateParameterInput->setEnabled(selectedTemplate != Template::Custom);

  switch (selectedTemplate) {
    case Template::Custom:
    default:
      templateParameterLabel->setText("");
      break;

    case Template::CoolToWarm:
      templateParameterLabel->setText(tr("Percentile: "));
      templateParameterInput->setMinimum(50.0);
      templateParameterInput->setMaximum(100.0);
      templateParameterInput->setSingleStep(1.0);
      templateParameterInput->setDecimals(2);
      break;

    case Template::OneHot:
      templateParameterLabel->setText(tr("Value: "));

      if (auto histogram = getHistogramProvider()
                               ? getHistogramProvider()->getData()
                               : HistogramProvider::DataPtr()) {
        templateParameterInput->setMinimum(histogram->minimumValue);
        templateParameterInput->setMaximum(histogram->maximumValue);
        templateParameterInput->setSingleStep(1);
        templateParameterInput->setDecimals(0);
      }

      break;
  }
}

void ColorizerWidget::updateTemplate() {
  static const Color coolColor(0.230, 0.299, 0.754);  // Pretty cool
  static const Color warmColor(0.706, 0.016, 0.150);
  static const Color transparentColor(0.230, 0.299, 0.754, 0.1);

  Template selectedTemplate = getSelectedTemplate();

  // Suppress colorizer change events causing the combobox to revert to "Custom"
  isUpdatingTemplate = true;

  switch (selectedTemplate) {
    case Template::Custom:
    default:
      break;

    case Template::CoolToWarm: {
      std::vector<ColorizerEntry> entries;
      auto lerp = ColorInterpolator::MSHDiverging;

      auto histogram = getHistogramProvider()
                           ? getHistogramProvider()->getData()
                           : HistogramProvider::DataPtr();

      double percentile = templateParameterInput->value() / 100.0;
      double min = histogram ? histogram->findPercentile(1 - percentile) : 0.0;
      double max = histogram ? histogram->findPercentile(percentile) : 0.0;
      entries.emplace_back(min, coolColor, lerp);
      entries.emplace_back(max, warmColor, lerp);
      getColorizer()->setEntries(std::move(entries));
      break;
    }

    case Template::OneHot: {
      double value = templateParameterInput->value();
      double prev = std::nextafter(value, std::numeric_limits<double>::min());
      double next = std::nextafter(value, std::numeric_limits<double>::max());

      std::vector<ColorizerEntry> entries;
      entries.emplace_back(prev, transparentColor, ColorInterpolator::RGB);
      entries.emplace_back(value, warmColor, ColorInterpolator::RGB);
      entries.emplace_back(next, transparentColor, ColorInterpolator::RGB);
      getColorizer()->setEntries(std::move(entries));
      break;
    }
  }

  isUpdatingTemplate = false;
}
