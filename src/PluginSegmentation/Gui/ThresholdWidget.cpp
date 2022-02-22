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

#include "ThresholdWidget.hpp"
#include <Voxie/Data/Color.hpp>
#include <Voxie/Data/ColorInterpolator.hpp>

using namespace vx;

ThresholdWidget::ThresholdWidget(LabelViewModel* labelViewModel)
    : labelViewModel(labelViewModel) {
  // Multi thresh widget
  layout = new QVBoxLayout;
  colorizerWidget = new ColorizerGradientWidget();
  layout->addWidget(colorizerWidget);
  toolbarTop = new QToolBar;
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

  leftValueBox = new QDoubleSpinBox;
  rightValueBox = new QDoubleSpinBox;
  leftValueBox->setMinimum(-std::numeric_limits<double>::infinity());
  leftValueBox->setMaximum(std::numeric_limits<double>::infinity());
  leftValueBox->setDecimals(3);
  rightValueBox->setMinimum(-std::numeric_limits<double>::infinity());
  rightValueBox->setMaximum(std::numeric_limits<double>::infinity());
  rightValueBox->setDecimals(3);

  currentColorButton = new QToolButton;
  currentColorButton->setToolTip(tr("Select color"));

  colorPicker = new QColorDialog;
  colorPicker->setOption(QColorDialog::NoButtons);
  colorPicker->setOption(QColorDialog::ShowAlphaChannel);

  toolbarTop->addSeparator();

  toolbarTop->addWidget(currentColorButton);

  labelSelector = new QComboBox;
  labelSelector->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  updateLabelList(true);
  labelSelector->setCurrentIndex(labelSelector->findData(-1));
  toolbarTop->addWidget(labelSelector);

  layout->addWidget(toolbarTop);

  // emit the data change signal
  connect(colorizerWidget, &ColorizerGradientWidget::entryAdded, this,
          [this]() { Q_EMIT dataChanged(); });
  connect(colorizerWidget, &ColorizerGradientWidget::entryRemoved, this,
          [this]() { Q_EMIT dataChanged(); });
  connect(colorizerWidget, &ColorizerGradientWidget::entryValueChanged, this,
          [this]() { Q_EMIT dataChanged(); });

  connect(labelSelector,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this,
          &ThresholdWidget::updateThresholdLabelChange);

  connect(addEntryButton, &QToolButton::clicked, this,
          &ThresholdWidget::addEntry);

  connect(removeEntryButton, &QToolButton::clicked, colorizerWidget,
          &ColorizerGradientWidget::removeSelectedEntry);

  connect(nextEntryButton, &QToolButton::clicked, this, [this]() {
    colorizerWidget->nextEntry();
    rightValueChange(rightValueBox->value());
    leftValueChange(leftValueBox->value());
  });

  connect(prevEntryButton, &QToolButton::clicked, this, [this]() {
    colorizerWidget->previousEntry();
    rightValueChange(rightValueBox->value());
    leftValueChange(leftValueBox->value());
  });

  connect(leftValueBox,
          static_cast<void (QDoubleSpinBox::*)(double)>(
              &QDoubleSpinBox::valueChanged),
          this, &ThresholdWidget::leftValueChange);

  connect(rightValueBox,
          static_cast<void (QDoubleSpinBox::*)(double)>(
              &QDoubleSpinBox::valueChanged),
          this, &ThresholdWidget::rightValueChange);

  connect(currentColorButton, &QToolButton::clicked, [this]() {
    int labelId = -1;
    if (colorizerWidget->hasSelectedEntry()) {
      labelId = colorizerLabelMap.value(colorizerWidget->getSelectedIndex());
    }
    if (labelId == -1) colorPicker->show();
  });

  connect(colorizerWidget, &ColorizerGradientWidget::selectedIndexChanged, this,
          [=]() {
            updateButtonEnableStates();
            updateCurrentColorMapping();
            updateLabelSelector();
          });

  connect(colorizerWidget, &ColorizerGradientWidget::entryValueChanged, this,
          [=](int idx, double oldVal, double newVal) {
            Q_UNUSED(idx);
            Q_UNUSED(oldVal);
            Q_UNUSED(newVal);
            updateButtonEnableStates();
            updateCurrentColorMapping();
            updateColorSelectionButton();
          });

  connect(colorizerWidget, &ColorizerGradientWidget::entryRemoved, this,
          [=](int removedIndex) {
            // update old indices in map depending on how the colorizer entries
            // changed
            QMap<int, int> entryBuffer;
            QMap<int, int>::iterator i = colorizerLabelMap.begin();
            while (i != colorizerLabelMap.end()) {
              auto oldIdx = i.key();
              auto labelId = i.value();
              if (oldIdx >= removedIndex) {
                i = colorizerLabelMap.erase(i);
                entryBuffer.insert(oldIdx - 1, labelId);
              } else {
                ++i;
              }
            }
            for (i = entryBuffer.begin(); i != entryBuffer.end(); i++) {
              colorizerLabelMap.insert(i.key(), i.value());
            }

            colorizerWidget->setSelectedIndex(std::max(0, removedIndex - 1));
            Q_EMIT dataChanged();
          });

  connect(colorizerWidget, &ColorizerGradientWidget::entryAdded, this, [=]() {
    if (colorizerWidget->hasSelectedEntry()) {
      colorizerWidget->setSelectedEntryInterpolator(ColorInterpolator(
          ColorInterpolator::Left, AlphaInterpolationType::Left));

      auto selectedIndex = colorizerWidget->getSelectedIndex();

      // update old indices in map depending on how the colorizer entries
      // changed
      QMap<int, int> entryBuffer;
      QMap<int, int>::iterator i = colorizerLabelMap.begin();
      while (i != colorizerLabelMap.end()) {
        auto oldIdx = i.key();
        auto labelId = i.value();
        if (oldIdx >= selectedIndex) {
          i = colorizerLabelMap.erase(i);
          entryBuffer.insert(oldIdx + 1, labelId);
        } else {
          ++i;
        }
      }
      for (i = entryBuffer.begin(); i != entryBuffer.end(); i++) {
        colorizerLabelMap.insert(i.key(), i.value());
      }
      auto entry = colorizerWidget->getSelectedEntry();
      colorizerLabelMap.insert(selectedIndex, -1);

      // trigger color update
      colorizerWidget->setSelectedIndex(selectedIndex);
      colorPicker->setCurrentColor(entry.color().asQColor());
      updateLabelSelector();
      Q_EMIT dataChanged();
    }
  });

  connect(colorizerWidget, &ColorizerGradientWidget::selectedEntryValueChanged,
          this, [=]() {
            updateButtonEnableStates();
            updateCurrentColorMapping();
            updateColorSelectionButton();
          });

  connect(colorPicker, &QColorDialog::currentColorChanged, this,
          [=](QColor color) {
            if (colorizerWidget->hasSelectedEntry()) {
              colorizerWidget->setSelectedEntryColor(color);

              QColor color2 =
                  colorizerWidget->getSelectedEntry().color().asQColor();
              currentColorButton->setStyleSheet(
                  "QWidget { border: 1px solid lightGray; background-color: " +
                  color2.name() + "; }");

              Q_EMIT dataChanged();
            }
          });

  // block deselecting indices in colorizer gradient widget by resetting to last
  // selected index
  connect(colorizerWidget, &ColorizerGradientWidget::selectedIndexChanged, this,
          [=]() {
            if (colorizerWidget->getSelectedIndex() == -1) {
              colorizerWidget->setSelectedIndex(lastSelectedIndex);
            } else {
              lastSelectedIndex = colorizerWidget->getSelectedIndex();
            }
            // clear box borders when changing indices
            leftValueBox->setMinimum(-std::numeric_limits<double>::infinity());
            leftValueBox->setMaximum(std::numeric_limits<double>::infinity());
            rightValueBox->setMinimum(-std::numeric_limits<double>::infinity());
            rightValueBox->setMaximum(std::numeric_limits<double>::infinity());
          });

  QHBoxLayout* thresholdBoxLayout = new QHBoxLayout();
  thresholdBoxLayout->addWidget(leftValueBox);
  thresholdBoxLayout->addWidget(rightValueBox);
  layout->addLayout(thresholdBoxLayout);
  this->setLayout(layout);
  colorizerWidget->setSelectedIndex(0);
  colorizerWidget->setBlockOverlaps(true);
}

void ThresholdWidget::init() {
  auto interpolator =
      ColorInterpolator(ColorInterpolator::Left, AlphaInterpolationType::Left);
  auto color = getNewColor();
  color.setAlpha(1);
  colorizerWidget->blockSignals(true);
  colorizerWidget->addEntry(-std::numeric_limits<double>::infinity(), color,
                            interpolator);
  colorizerWidget->addEntry(std::numeric_limits<double>::infinity(),
                            Color(0, 0, 0, 0), interpolator);
  colorizerWidget->blockSignals(false);
  colorizerLabelMap.insert(0, -1);
  colorizerLabelMap.insert(1, -2);

  colorizerWidget->setEntryCountBounds(2, -1);
  colorizerWidget->setFixedHeight(90);
  this->blockedIndices.append(0);
  this->blockedIndices.append(-1);  // block the last index
  colorizerWidget->setSelectedIndex(0);
  updateButtonEnableStates();
  Q_EMIT dataChanged();
}

void ThresholdWidget::updateButtonEnableStates() {
  auto entryCount = colorizerWidget->getEntryCount();
  auto index = colorizerWidget->getSelectedIndex();

  // set index to -1 if it is the last one to allow for backward indexing in
  // blockIndices
  if (index == entryCount - 1) index = -1;
  bool isIndexBlocked = this->blockedIndices.contains(index);

  bool isNextIndexBlocked = true;
  if (index < entryCount && index >= 0) {
    auto index2 = colorizerWidget->getSelectedIndex() + 1;

    // set index to -1 if it is the last one to allow for backward indexing in
    // blockIndices
    if (index2 == entryCount - 1) index2 = -1;
    isNextIndexBlocked = this->blockedIndices.contains(index2);
  }

  removeEntryButton->setEnabled(
      colorizerWidget->hasSelectedEntry() && !isIndexBlocked &&
      (colorizerWidget->getSelectedIndex() < entryCount - 1) &&
      entryCount > minEntryCount);

  if (maxEntryCount > 0) addEntryButton->setEnabled(entryCount < maxEntryCount);
  prevEntryButton->setEnabled(colorizerWidget->hasSelectedEntry() &&
                              !isIndexBlocked);
  nextEntryButton->setEnabled(colorizerWidget->hasSelectedEntry() &&
                              !isNextIndexBlocked);
  leftValueBox->setEnabled(colorizerWidget->hasSelectedEntry() &&
                           !isIndexBlocked);
  rightValueBox->setEnabled(colorizerWidget->hasSelectedEntry() &&
                            !isNextIndexBlocked);
  currentColorButton->setEnabled(colorizerWidget->hasSelectedEntry());
}

void ThresholdWidget::updateCurrentColorMapping() {
  if (colorizerWidget->hasSelectedEntry()) {
    colorPicker->blockSignals(true);
    colorPicker->setCurrentColor(
        colorizerWidget->getSelectedEntry().color().asQColor());
    colorPicker->blockSignals(false);

    leftValueBox->blockSignals(true);
    leftValueBox->setSingleStep((colorizerWidget->getMaximumValue() -
                                 colorizerWidget->getMinimumValue()) *
                                0.02);
    leftValueBox->setValue(colorizerWidget->getSelectedEntry().value());
    leftValueBox->blockSignals(false);

    rightValueBox->blockSignals(true);
    rightValueBox->setSingleStep((colorizerWidget->getMaximumValue() -
                                  colorizerWidget->getMinimumValue()) *
                                 0.02);

    rightValueBox->setValue(
        colorizerWidget->getEntry(colorizerWidget->getSelectedIndex() + 1)
            .value());
    rightValueBox->blockSignals(false);
  }
}

void ThresholdWidget::updateColorSelectionButton() {
  int labelId = -1;
  if (colorizerWidget->hasSelectedEntry()) {
    labelId = colorizerLabelMap.value(colorizerWidget->getSelectedIndex());
  }
  QColor color = colorizerWidget->getSelectedEntry().color().asQColor();
  int alpha = 255;
  if (colorizerWidget->hasSelectedEntry()) {
    if (labelId != -1) alpha = 80;
    QString style = QString(
                        "QWidget { border: 1px solid lightGray; "
                        "background-color: rgba(%1, %2, %3, %4);}")
                        .arg(color.red())
                        .arg(color.green())
                        .arg(color.blue())
                        .arg(alpha);
    currentColorButton->setStyleSheet(style);
  }
}

Color ThresholdWidget::getNewColor() {
  quint32 value = std::rand() % defaultColorPalette.size();
  auto labelColor = Color(defaultColorPalette.at(value));
  return labelColor;
}

void ThresholdWidget::addEntry() {
  auto labelColor = getNewColor();
  auto mean = (colorizerWidget->getMaximumValue() +
               colorizerWidget->getMinimumValue()) *
              0.5;
  auto interpolator =
      ColorInterpolator(ColorInterpolator::Left, AlphaInterpolationType::Left);
  if (colorizerWidget->getEntryCount() - 2 == 0) {
    colorizerWidget->addEntry(mean, labelColor.asQColor(), interpolator);
  } else if (colorizerWidget->getEntryCount() - 2 == 1) {
    colorizerWidget->addEntry(mean / 2, labelColor.asQColor(), interpolator);
  } else {
    // 2+ entries: create average mapping between selected and next entry
    auto entries = colorizerWidget->getColorizer()->getEntries();
    int index = colorizerWidget->hasSelectedEntry()
                    ? std::min(std::max(colorizerWidget->getSelectedIndex(), 1),
                               colorizerWidget->getEntryCount() - 3)
                    : colorizerWidget->getEntryCount() - 3;

    auto entry1 = entries[index];
    auto entry2 = entries[index + 1];

    double value = (entry1.value() + entry2.value()) * 0.5;
    colorizerWidget->addEntry(value, labelColor.asQColor(), interpolator);
    colorizerWidget->setSelectedIndex(index + 1);
  }
  updateLabelSelector();
}

void ThresholdWidget::printMap() {
  QMap<int, int>::iterator i;
  qDebug() << "------colorizerLabelMap------";
  for (i = colorizerLabelMap.begin(); i != colorizerLabelMap.end(); ++i) {
    int labelId = i.value();
    try {
      auto entry = colorizerWidget->getEntry(i.key());
      qDebug() << "Colorizer Index: " << i.key() << " Val: " << entry.value()
               << "| Id:" << labelId;
    } catch (...) {
      qDebug() << "Colorizer Index: " << i.key() << " Val: "
               << "NO VAL"
               << "| Id:" << labelId;
    }
  }
}

void ThresholdWidget::updateLabelSelector() {
  int labelId = -1;
  if (colorizerWidget->hasSelectedEntry()) {
    labelId = colorizerLabelMap.value(colorizerWidget->getSelectedIndex());
  }

  int boxIdx = labelSelector->findData(labelId);
  labelSelector->setCurrentIndex(boxIdx);
  updateColorSelectionButton();
}

void ThresholdWidget::rightValueChange(double value) {
  auto idx = colorizerWidget->getSelectedIndex() + 1;
  if (idx < colorizerWidget->getEntryCount() - 1 && idx >= 0) {
    colorizerWidget->setEntryValue(idx, value);

    // Make sure right threshold is bigger/equals left threshold
    double lowerThresh = getLeftBox()->value();
    double upperThresh = colorizerWidget->getEntry(idx + 1).value();
    getRightBox()->setMaximum(upperThresh);
    getRightBox()->setMinimum(lowerThresh);
  }
}

void ThresholdWidget::leftValueChange(double value) {
  auto idx = colorizerWidget->getSelectedIndex();
  if (idx < colorizerWidget->getEntryCount() && idx > 0) {
    colorizerWidget->setEntryValue(idx, value);

    // Make sure left threshold is smaller/equals right threshold
    double upperThresh = getRightBox()->value();
    double lowerThresh = colorizerWidget->getEntry(idx - 1).value();
    getLeftBox()->setMaximum(upperThresh);
    getLeftBox()->setMinimum(lowerThresh);
  }
}

void ThresholdWidget::updateThresholdLabelChange(int index) {
  if (colorizerWidget->hasSelectedEntry()) {
    int labelId = labelSelector->itemData(index).toInt();
    colorizerLabelMap[colorizerWidget->getSelectedIndex()] = labelId;
    auto labelColor = Color(1, 1, 1, 0);
    if (labelId >= 0) {
      QList<int> labelIdList;

      // get the color for the labelId by looping all entries
      for (int i = 0; i < labelViewModel->rowCount(); i++) {
        const SegmentationType loopLabelId =
            labelViewModel->getLabelTable()
                ->getRowColumnData(i, "LabelID")
                .toUInt();
        if (loopLabelId == labelId) {
          labelColor = Color(labelViewModel->getLabelTable()
                                 ->getRowColumnData(i, "Color")
                                 .value<TupleVector<double, 4>>());
          break;
        }
      }

    } else if (labelId == -1) {
      labelColor = getNewColor();
    }
    colorizerWidget->setSelectedEntryColor(labelColor);

    // trigger color update
    colorizerWidget->setSelectedIndex(colorizerWidget->getSelectedIndex());
    colorPicker->setCurrentColor(labelColor.asQColor());
    Q_EMIT dataChanged();
  }
}

void ThresholdWidget::updateLabelList(bool status, bool isRemoval) {
  Q_UNUSED(status);
  labelSelector->clear();
  QList<SegmentationType> labelIdList;
  for (int i = 0; i < labelViewModel->rowCount(); i++) {
    SegmentationType labelID = labelViewModel->getLabelTable()
                                   ->getRowColumnData(i, "LabelID")
                                   .toUInt();
    QString labelName =
        labelViewModel->getLabelTable()->getRowColumnData(i, "Name").toString();

    // set corresponding color in colorizer
    auto labelColor = Color(labelViewModel->getLabelTable()
                                ->getRowColumnData(i, "Color")
                                .value<TupleVector<double, 4>>());
    if (colorizerLabelMap.values().contains(labelID))
      colorizerWidget->setEntryColor(colorizerLabelMap.key(labelID),
                                     labelColor);

    labelSelector->addItem(labelName, labelID);
    labelIdList.append((SegmentationType)labelID);
  }

  labelSelector->addItem("New", -1);
  labelSelector->addItem("Ignore", -2);

  // Reset mapping to labels that got deleted to "New"
  QMap<int, int>::iterator i;
  if (isRemoval) {
    for (i = colorizerLabelMap.begin(); i != colorizerLabelMap.end();) {
      if ((!labelIdList.contains(i.value())) && i.value() >= 0) {
        colorizerLabelMap[i.key()] = -1;

      } else {
        ++i;
      }
    }
  }
  updateLabelSelector();
  Q_EMIT dataChanged();
}
