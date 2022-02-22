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

// QDBusConnection should be included as early as possible:
// https://bugreports.qt.io/browse/QTBUG-48351 /
// https://bugreports.qt.io/browse/QTBUG-48377
#include <QtDBus/QDBusConnection>

#include <PluginSegmentation/Gui/ThresholdWidget.hpp>
#include <Voxie/Data/LabelViewModel.hpp>

namespace vx {

/*! ThresholdSelectionWidget UI class
 * Child of ThresholdWidget class with the same UI elements but different
 * usage/functionality specifically adjusted to the Segmentation
 * ThresholdSelectionStep
 */
class ThresholdSelectionWidget : public ThresholdWidget {
  Q_OBJECT

 public:
  ThresholdSelectionWidget(LabelViewModel* labelViewModel);
  void init() override;

  /** @brief Performs color and threshold changes from changes on colorizer */
  void updateCurrentColorMapping() override;

  /**
   * @brief Applies right threshold box value changes to colorizer
   * @param value new box value entry
   */
  void rightValueChange(double value) override;

  /**
   * @brief Applies left threshold box value changes to colorizer
   * @param value new box value entry
   */
  void leftValueChange(double value) override;

 private:
  /**
   * @brief Calculates colorizer index offset for the given thresholdbox to
   * connect the correct colorizer index to the box usage
   * @param box box for which the index shall be calculated (left/right)
   * @return caclulated colorizer index
   */
  int getOffsetFromIndex(QDoubleSpinBox* box);
};
}  // namespace vx
