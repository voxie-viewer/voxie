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

#include <qpushbutton.h>
#include <qspinbox.h>
#include <qwidget.h>
#include <PluginSegmentation/SegmentationUtils.hpp>
#include <QColorDialog>
#include <QComboBox>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <Voxie/Data/LabelViewModel.hpp>
#include <Voxie/Gui/ColorizerGradientWidget.hpp>

namespace vx {

/*! ThresholdWidget
 * UI class  allowing a colorizer based threshold to label mapping.
 * QWidget combining Voxies's colorizer gradient widget, toolbar buttons, labels
 * selection ComboBox, 2x Threshold spin boxes
 */
class ThresholdWidget : public QWidget {
  Q_OBJECT

 public:
  ThresholdWidget(LabelViewModel* labelViewModel);
  /** @brief Setter to limit colorizer entry lower and upper counts */
  void setEntryBounds(int min, int max) {
    this->minEntryCount = min;
    this->maxEntryCount = max;
  }

  /** @brief Custom init for widget usage, sets boundaries & initial entries */
  virtual void init();

  /**
   * @brief Getter for colorizer
   * @return Pointer to colorizer of this widget
   */
  ColorizerGradientWidget* getColorizerWidget() {
    return this->colorizerWidget;
  };

  /**
   * @brief Getter for map from colorizer entries to assigned labels
   * @return Map from colorizer vector entries to label ids
   */
  QMap<int, int>& getColorizerLabelMap() { return this->colorizerLabelMap; };

  /**
   * @brief Setter for map from colorizer entries to assigned labels
   * @param map from colorizer vector entries to label ids
   */
  void setColorizerLabelMap(QMap<int, int>& map) {
    this->colorizerLabelMap = map;
  };

  /**
   * @brief Updates label selector box entries when labels are added/removed in
   * segmentation
   * @param status label add/ label remove signal status
   * @param isRemoval signals that labels were removed which clears the
   * colorizerLabelMap
   */
  void updateLabelList(bool status, bool isRemoval = false);

  /**
   * @brief Getter for left threshold value box
   */
  QDoubleSpinBox* getLeftBox() { return leftValueBox; }

  /**
   * @brief Getter for right threshold value box
   */
  QDoubleSpinBox* getRightBox() { return rightValueBox; }

 protected Q_SLOTS:
  /** @brief Perform label changes on the label selector box value */
  void updateLabelSelector();

  /** @brief Performs all button and box enable/disable updates */
  virtual void updateButtonEnableStates();

  /** @brief Performs color and threshold changes from changes on colorizer */
  virtual void updateCurrentColorMapping();

  /**
   * @brief Applies right threshold box value changes to colorizer
   * @param value new box value entry
   */
  virtual void rightValueChange(double value);

  /**
   * @brief Applies left threshold box value changes to colorizer
   * @param value new box value entry
   */
  virtual void leftValueChange(double value);

  /** @brief Applies changes from colorizer to color value button */
  void updateColorSelectionButton();

  /**
   * @brief Handels changes when label to threshold assignment changes
   * @param index of the new selected box entry
   */
  void updateThresholdLabelChange(int index);

  /** @brief Adds entry to colorizer specifically based on existing entries */
  void addEntry();

  /** @brief Prints current label id to colorizer entry map for debug usage */
  void printMap();

 protected:
  /** @brief Returns new color based on segmentation color palette */
  Color getNewColor();

  LabelViewModel* labelViewModel;
  QComboBox* labelSelector;
  ColorizerGradientWidget* colorizerWidget;

  QVBoxLayout* layout;
  QToolBar* toolbarTop;
  QToolButton* addEntryButton;
  QToolButton* removeEntryButton;
  QToolButton* prevEntryButton;
  QToolButton* nextEntryButton;
  QDoubleSpinBox* leftValueBox;
  QDoubleSpinBox* rightValueBox;
  QToolButton* currentColorButton;
  QColorDialog* colorPicker;

  // Map from colorizer entry vector index to assigned label id
  // Needs to be updated when colorizer vector entries change
  QMap<int, int> colorizerLabelMap;

  // colorizer entry count upper and lower borders
  int minEntryCount = 0;
  int maxEntryCount = -1;

  // stores last selected colorizer entry index
  uint lastSelectedIndex = 0;

  // list of colorizer entry indicies which shall be blocked from usage when
  // selected. E.g. -inf and +inf border values
  QList<uint> blockedIndices = QList<uint>();

 Q_SIGNALS:
  // Gets emitted, when the colorizerLabelMap gets changed or some data in
  // colorizerWidget.
  void dataChanged();
};
}  // namespace vx
