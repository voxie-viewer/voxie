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

#include "ThresholdSelectionWidget.hpp"
#include <Voxie/Data/Color.hpp>
#include <Voxie/Data/ColorInterpolator.hpp>

using namespace vx;

ThresholdSelectionWidget::ThresholdSelectionWidget(
    LabelViewModel* labelViewModel)
    : ThresholdWidget(labelViewModel) {}

void ThresholdSelectionWidget::init() {
  toolbarTop->setVisible(false);
  this->minEntryCount = 4;
  this->maxEntryCount = 4;
  colorizerWidget->setEntryCountBounds(4, 4);
  colorizerWidget->setFixedHeight(90);

  connect(
      colorizerWidget, &ColorizerGradientWidget::boundsChanged, this, [=]() {
        // remove old entries before inserting new ones
        colorizerWidget->blockSignals(true);
        colorizerWidget->clearAllEntries();
        colorizerLabelMap.clear();

        auto range = (colorizerWidget->getMaximumValue() +
                      colorizerWidget->getMinimumValue());
        auto interpolatorLeft = ColorInterpolator(ColorInterpolator::Left,
                                                  AlphaInterpolationType::Left);

        auto borderColor = Color(0.25, 0.25, 0.25, 0.85);
        colorizerWidget->addEntry(-std::numeric_limits<double>::infinity(),
                                  borderColor, interpolatorLeft);
        colorizerWidget->addEntry(range * 0.25, Color(1, 0.4, 0.4, 1),
                                  interpolatorLeft);
        colorizerWidget->addEntry(range * 0.75, borderColor, interpolatorLeft);
        colorizerWidget->addEntry(std::numeric_limits<double>::infinity(),
                                  borderColor, interpolatorLeft);
        colorizerWidget->blockSignals(false);
        colorizerLabelMap.insert(0, -2);
        colorizerLabelMap.insert(1, -1);
        colorizerLabelMap.insert(2, -1);
        colorizerLabelMap.insert(3, -2);

        colorizerWidget->setSelectedIndex(1);

        updateButtonEnableStates();
      });
}

int ThresholdSelectionWidget::getOffsetFromIndex(QDoubleSpinBox* box) {
  int index = colorizerWidget->getSelectedIndex();
  int leftIndex = index;
  int rightIndex = index + 1;
  if (index == colorizerWidget->getEntryCount() - 2) {
    leftIndex = index - 1;
    rightIndex = index;
  }

  if (box == leftValueBox) {
    return leftIndex;
  } else if (box == rightValueBox) {
    return rightIndex;
  }
  return -1;
}

void ThresholdSelectionWidget::rightValueChange(double value) {
  auto idx = getOffsetFromIndex(rightValueBox);
  if (idx < colorizerWidget->getEntryCount()) {
    colorizerWidget->setEntryValue(idx, value);
  }
}

void ThresholdSelectionWidget::leftValueChange(double value) {
  auto idx = getOffsetFromIndex(leftValueBox);
  if (idx < colorizerWidget->getEntryCount()) {
    colorizerWidget->setEntryValue(idx, value);
  }
}

void ThresholdSelectionWidget::updateCurrentColorMapping() {
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
    leftValueBox->setValue(
        colorizerWidget->getEntry(getOffsetFromIndex(leftValueBox)).value());
    leftValueBox->blockSignals(false);
    rightValueBox->blockSignals(true);

    rightValueBox->setSingleStep((colorizerWidget->getMaximumValue() -
                                  colorizerWidget->getMinimumValue()) *
                                 0.02);

    rightValueBox->setValue(
        colorizerWidget->getEntry(getOffsetFromIndex(rightValueBox)).value());

    rightValueBox->blockSignals(false);
  }
}
