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

#include <qlabel.h>
#include <qlistview.h>
#include <qlistwidget.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qtableview.h>
#include <qwidget.h>
#include <PluginSegmentation/SegmentationUtils.hpp>
#include <PluginSegmentation/StepManager.hpp>
#include <QRadioButton>
#include <QTimer>
#include <QToolBar>
#include <Voxie/Data/LabelViewModel.hpp>
#include <Voxie/Gui/ColorizerGradientWidget.hpp>
#include <Voxie/Gui/ColorizerWidget.hpp>
#include <Voxie/Gui/LabelTableView.hpp>
#include <Voxie/Vis/HistogramWidget.hpp>
#include "PluginSegmentation/Gui/HistoryViewModel.hpp"

namespace vx {
class SegmentationWidget : public QWidget {
  Q_OBJECT

 public:
  SegmentationWidget(LabelViewModel* labelViewModel,
                     HistoryViewModel* historyViewModel,
                     StepManager* stepManager);
  /**
   * @brief Returns the row indices of the currently selected labels
   */
  QList<int> getSelectedIndices();
  /**
   * @brief Returns the labelIDs of the currently selected labels
   */
  QList<SegmentationType> getSelectedLabelIDs();
  /**
   * @brief Returns all LabelIDs of the labelTable
   */
  QList<SegmentationType> getLabelIDs();
  /**
   * @brief Returns the not selected labelIDs (e.g. diff between AllLabelIDs and
   * SelectedLabelIDs)
   */
  QList<SegmentationType> getNotSelectedLabelIDs();

  /**
   * @brief Uncheck brush and eraser selection buttons
   */
  void uncheckBrushes();
  /**
   * @brief Uncheck lasso selection button
   */
  void uncheckLasso();
  /**
   * @param incremental Flag to indicate if voxelCount is added to the existing
   * value or if the existing value is overwritten
   * @param voxelCount Voxel count of the selection
   * @param totalVoxelCount Voxel count of the whole volume
   */
  void setActiveSelectionInfo(bool incremental, qint64 voxelCount,
                              qint64 totalVoxelCount);
  /**
   * @brief Set the info text of the hovered voxel
   */
  void setHoverInfo(double voxelValue, QString voxelLabel);

  /**
   * @brief Sets histogram variable provider for slices
   */
  void setSliceHistogramProvider(
      QSharedPointer<vx::HistogramProvider> provider);

  /**
   * @brief Gets the segmentation histogram widget colorizer
   */
  ColorizerWidget* getColorizerWidget();

 private Q_SLOTS:
  /**
   * @brief Add a new label and update the GUI
   */
  int addLabel();
  /**
   * @brief Add a new label and update the GUI
   * @param color Color of the new label
   */
  int addLabelByColor(Color color);
  /**
   * @brief Open a color picker and set the selected labels colors to the chosen
   * color
   */
  void colorizeLabels();
  /**
   * @brief Set the selected labels voxels as active selection
   */
  void selectLabelVoxels();
  /**
   * @brief Delete the selected labels
   */
  void deleteLabels();
  /**
   * @brief Add the active selection to the selected label
   */
  void addSelectionToLabel();
  /**
   * @brief Subtract the active selection from the selected labels
   */
  void subtractSelectionFromLabels();
  /**
   * @brief Reset the active selection
   */
  void resetSelection();
  /**
   * @brief Reset the segmentation widgets to their default values
   */
  void resetSegmentationWidgets();

 Q_SIGNALS:
  /**
   * @brief Called when the input volume is set/removed
   */
  void isInputExisting(bool isExisting);
  /**
   * @brief Called when the brush or eraser is toggled using its button
   */
  void toggledBrush(bool isSet);
  /**
   * @brief Called when the radius is changed using its spinbox
   */
  void changedRadius(int radius);
  /**
   * @brief Called when the lasso is toggled using its button
   */
  void toggledLasso(bool isSet);
  /**
   * @brief Called when the active selection is reset
   */
  void resetActiveSelection();
  /**
   * @brief Called, when all UI widgets should be resetted to the default state
   */
  void resetUIWidgets();

 private:
  LabelViewModel* labelViewModel;
  HistoryViewModel* historyViewModel;
  StepManager* stepManager;

  QPushButton* addLabelButton;
  QPushButton* colorLabelButton;
  QPushButton* selectLabelButton;
  QPushButton* deleteLabelButton;
  QTableView* tableView;
  QTabWidget* stepTabs;

  // Colorizer
  ColorizerWidget* segmentationColorizer;

  // Histogram
  HistogramWidget* histogram;
  QRadioButton* sliceRadioButton;
  QRadioButton* volumeRadioButton;
  QSharedPointer<vx::HistogramProvider> sliceHistogramProvider;
  QSharedPointer<vx::HistogramProvider> volumeHistogramProvider;

  QPushButton* addSelectionButton;
  QPushButton* subtractSelectionButton;
  QPushButton* resetSelectionButton;

  /**
   * @brief Indicates if the segmentation fields are beeing reset
   */
  bool isResettingFields = false;

  /**
   * @brief Indicates if the brush is currently in the process of changing. This
   * exists to prevent recursive behaviour.
   */
  bool isChangingBrushState = false;
  QPushButton* brushToggleButton;
  QPushButton* eraserToggleButton;
  QSpinBox* brushRadiusBox;
  QPushButton* lassoToggleButton;

  QLabel* selectedVoxelCount;
  QLabel* selectedVoxelPercentage;
  /**
   * @brief Current voxel count of the active selection (used for incremental
   * changes)
   */
  qint64 activeSelectionVoxelCount = 0;

  QLabel* hoveredVoxelValue;
  QLabel* hoveredVoxelLabel;

  QListView* historyListView;
};
}  // namespace vx
