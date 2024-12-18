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

#include <qboxlayout.h>
#include <qcolordialog.h>
#include <qformlayout.h>
#include <qheaderview.h>
#include <PluginSegmentation/Gui/SegmentationWidget.hpp>
#include <PluginSegmentation/Steps/MetaStep.hpp>
#include <Voxie/Data/Color.hpp>
#include <Voxie/Data/ColorInterpolator.hpp>
#include <Voxie/IVoxie.hpp>
#include <Voxie/Interfaces/SliceVisualizerI.hpp>
#include <Voxie/Vis/VisualizerNode.hpp>

#include "Voxie/Data/ContainerData.hpp"

#include <VoxieClient/Format.hpp>

using namespace vx;

SegmentationWidget::SegmentationWidget(LabelViewModel* labelViewModel,
                                       HistoryViewModel* historyViewModel,
                                       StepManager* stepManager)
    : labelViewModel(labelViewModel),
      historyViewModel(historyViewModel),
      stepManager(stepManager) {
  QVBoxLayout* mainVBox = new QVBoxLayout();
  setLayout(mainVBox);
  setWindowTitle("Segmentation");

  QHBoxLayout* labelActions = new QHBoxLayout();
  QLabel* labelLabel = new QLabel("Labels");
  labelLabel->setStyleSheet("font-weight: bold");
  addLabelButton = new QPushButton("+");
  addLabelButton->setFixedWidth(40);
  addLabelButton->setEnabled(false);
  addLabelButton->setToolTip(QString("Add a new label"));
  connect(addLabelButton, &QPushButton::clicked, this,
          &SegmentationWidget::addLabel);
  connect(this, &SegmentationWidget::isInputExisting, addLabelButton,
          [this](bool isExisting) {
            this->addLabelButton->setEnabled(
                this->labelViewModel->getLabelIdCounter() <=
                    (std::numeric_limits<SegmentationType>::max() >> 1) &&
                isExisting);
          });

  colorLabelButton = new QPushButton("Color");
  colorLabelButton->setFixedWidth(60);
  colorLabelButton->setEnabled(false);
  colorLabelButton->setToolTip(QString("Set color of labels"));
  connect(colorLabelButton, &QPushButton::clicked, this,
          &SegmentationWidget::colorizeLabels);

  selectLabelButton = new QPushButton("Select");
  selectLabelButton->setFixedWidth(60);
  selectLabelButton->setEnabled(false);
  selectLabelButton->setToolTip(QString("Select the voxels of labels"));
  deleteLabelButton = new QPushButton("-");
  deleteLabelButton->setFixedWidth(40);
  deleteLabelButton->setEnabled(false);
  deleteLabelButton->setToolTip(QString("Delete labels"));
  connect(selectLabelButton, &QPushButton::clicked, this,
          &SegmentationWidget::selectLabelVoxels);
  connect(stepManager, &StepManager::voxelOpIsFinished, selectLabelButton,
          [=](bool voxelOpFinished) {
            bool isSelectionEmpty =
                tableView->selectionModel()->selectedRows().empty();
            this->selectLabelButton->setEnabled(!isSelectionEmpty &&
                                                voxelOpFinished);
          });
  connect(deleteLabelButton, &QPushButton::clicked, this,
          &SegmentationWidget::deleteLabels);
  connect(stepManager, &StepManager::voxelOpIsFinished, deleteLabelButton,
          [=](bool status) {
            bool isSelectionEmpty =
                tableView->selectionModel()->selectedRows().empty();
            this->deleteLabelButton->setEnabled(!isSelectionEmpty && status);
          });

  labelActions->addWidget(labelLabel);
  labelActions->addStretch();
  labelActions->addWidget(colorLabelButton);
  labelActions->addWidget(selectLabelButton);
  labelActions->addWidget(addLabelButton);
  labelActions->addWidget(deleteLabelButton);
  mainVBox->addLayout(labelActions);

  tableView = setLabelTableView(new QTableView(), labelViewModel);
  tableView->setColumnHidden(
      labelViewModel->getLabelTable()->getColumnIndexByName("Export"), true);
  mainVBox->addWidget(tableView);
  mainVBox->addSpacing(10);
  connect(labelViewModel, &QAbstractListModel::dataChanged, historyViewModel,
          &HistoryViewModel::updateViewSlot);

  connect(
      tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this,
      [=](const QItemSelection& selected, const QItemSelection& deselected) {
        Q_UNUSED(selected);
        Q_UNUSED(deselected);
        bool isSelectionEmpty =
            tableView->selectionModel()->selectedRows().empty();
        colorLabelButton->setEnabled(!isSelectionEmpty);
        selectLabelButton->setEnabled(!isSelectionEmpty);
        deleteLabelButton->setEnabled(!isSelectionEmpty);
      });

  QHBoxLayout* selectionActions = new QHBoxLayout();
  addSelectionButton = new QPushButton("+");
  addSelectionButton->setFixedWidth(40);
  addSelectionButton->setEnabled(false);
  addSelectionButton->setToolTip(QString("Add selection to selected label"));
  connect(addSelectionButton, &QPushButton::clicked, this,
          &SegmentationWidget::addSelectionToLabel);
  connect(stepManager, &StepManager::canApplyActiveSelection,
          addSelectionButton, &QPushButton::setEnabled);

  subtractSelectionButton = new QPushButton("-");
  subtractSelectionButton->setFixedWidth(40);
  subtractSelectionButton->setEnabled(false);
  subtractSelectionButton->setToolTip(
      QString("Subtract selection from selected label"));
  connect(subtractSelectionButton, &QPushButton::clicked, this,
          &SegmentationWidget::subtractSelectionFromLabels);
  connect(stepManager, &StepManager::canApplyActiveSelection,
          subtractSelectionButton, &QPushButton::setEnabled);

  resetSelectionButton = new QPushButton("X");
  resetSelectionButton->setFixedWidth(40);
  resetSelectionButton->setEnabled(false);
  resetSelectionButton->setToolTip(QString("Reset selection"));
  connect(resetSelectionButton, &QPushButton::clicked, this,
          &SegmentationWidget::resetSelection);
  connect(stepManager, &StepManager::canApplyActiveSelection,
          resetSelectionButton, &QPushButton::setEnabled);

  QLabel* selectionLabel = new QLabel("Active Selection");
  selectionLabel->setStyleSheet("font-weight: bold");
  selectionActions->addWidget(selectionLabel);
  selectionActions->addStretch();
  selectionActions->addWidget(addSelectionButton);
  selectionActions->addWidget(subtractSelectionButton);
  selectionActions->addWidget(resetSelectionButton);
  mainVBox->addLayout(selectionActions);

  // Brush
  QWidget* brushWidget = new QWidget();
  QVBoxLayout* brushOuterLayout = new QVBoxLayout();
  QHBoxLayout* brushLayout = new QHBoxLayout();
  brushToggleButton = new QPushButton("Brush");
  brushToggleButton->setCheckable(true);
  brushLayout->addWidget(brushToggleButton);
  eraserToggleButton = new QPushButton("Eraser");
  eraserToggleButton->setCheckable(true);
  brushLayout->addWidget(eraserToggleButton);
  QFormLayout* brushForm = new QFormLayout();
  brushRadiusBox = new QSpinBox();
  brushRadiusBox->setValue(10);
  brushRadiusBox->setMinimum(0);
  brushRadiusBox->setMaximum(std::numeric_limits<int>::max());
  brushForm->addRow(QString("Radius"), brushRadiusBox);
  brushOuterLayout->addLayout(brushLayout);
  brushOuterLayout->addLayout(brushForm);

  brushWidget->setLayout(brushOuterLayout);

  connect(brushToggleButton, &QPushButton::toggled, this, [this](bool isSet) {
    if (!isChangingBrushState) {
      isChangingBrushState = true;
      if (this->eraserToggleButton->isChecked()) {
        this->eraserToggleButton->setChecked(false);
      } else {
        Q_EMIT(this->toggledBrush(isSet));
      }
      this->stepManager->setBrushMode(brushModes::select);
      isChangingBrushState = false;
    }
  });
  connect(eraserToggleButton, &QPushButton::toggled, this, [this](bool isSet) {
    if (!isChangingBrushState) {
      isChangingBrushState = true;
      if (this->brushToggleButton->isChecked()) {
        this->brushToggleButton->setChecked(false);
      } else {
        Q_EMIT(this->toggledBrush(isSet));
      }
      this->stepManager->setBrushMode(brushModes::erase);
      isChangingBrushState = false;
    }
  });
  connect(brushRadiusBox,
          static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
          [this](int radius) { Q_EMIT(this->changedRadius(radius)); });
  Q_EMIT(this->changedRadius(this->brushRadiusBox->value()));

  // Lasso
  QWidget* lassoWidget = new QWidget();
  QVBoxLayout* lassoLayout = new QVBoxLayout();
  lassoToggleButton = new QPushButton("Lasso");
  lassoToggleButton->setCheckable(true);
  lassoLayout->addWidget(lassoToggleButton);
  lassoLayout->setAlignment(Qt::AlignmentFlag::AlignTop);
  lassoWidget->setLayout(lassoLayout);
  connect(lassoToggleButton, &QPushButton::toggled, this,
          [this](bool isSet) { Q_EMIT(this->toggledLasso(isSet)); });

  // Active Selection Tabs
  this->stepTabs = new QTabWidget();
  stepTabs->addTab(brushWidget, QIcon(":/icons/paint-brush.png"), "");
  stepTabs->setTabToolTip(stepTabs->count() - 1, "Brush Selection");
  stepTabs->addTab(lassoWidget, QIcon(":/icons/layer-shape-polygon.png"), "");
  stepTabs->setTabToolTip(stepTabs->count() - 1, "Lasso Selection");
  stepTabs->setIconSize(QSize(20, 20));

  // Add all default UI-StepWidgets
  auto nonSpecialSteps = this->stepManager->spawnDefaultSteps(
      this->labelViewModel, this->tableView->selectionModel());

  for (auto step : nonSpecialSteps) {
    QWidget* defaultUIWidget = new QWidget();
    QVBoxLayout* l = new QVBoxLayout();
    l->addWidget(step->getPropertySection());
    l->addStretch();
    defaultUIWidget->setLayout(l);
    stepTabs->addTab(defaultUIWidget, QIcon(step->prototype()->icon()),
                     step->prototype()->displayName());
    stepTabs->setTabToolTip(stepTabs->count() - 1, step->getStepTip());
  }

  connect(stepTabs, &QTabWidget::currentChanged, this, [this]() {
    Q_EMIT(this->toggledBrush(false));
    this->resetSegmentationWidgets();
  });

  mainVBox->addWidget(stepTabs);

  // Active selection info
  QGridLayout* selectionInfo = new QGridLayout();
  selectionInfo->setColumnStretch(0, 1);
  selectionInfo->setColumnStretch(1, 1);
  selectedVoxelCount = new QLabel("Voxels: ");
  selectedVoxelPercentage = new QLabel("Percentage: ");
  selectionInfo->addWidget(selectedVoxelCount);
  selectionInfo->addWidget(selectedVoxelPercentage);
  mainVBox->addLayout(selectionInfo);
  mainVBox->addSpacing(10);

  // Visualizer info
  QLabel* visualizerLabel = new QLabel("Visualizer");
  visualizerLabel->setStyleSheet("font-weight: bold;");
  mainVBox->addWidget(visualizerLabel);

  QGridLayout* hoverInfo = new QGridLayout();
  hoverInfo->setColumnStretch(0, 1);
  hoverInfo->setColumnStretch(1, 1);
  hoveredVoxelValue = new QLabel("Voxel value: ");
  hoveredVoxelLabel = new QLabel("Voxel label: ");
  hoveredSelectedLabel = new QLabel("Selected: ");
  hoverInfo->addWidget(hoveredVoxelValue);
  hoverInfo->addWidget(hoveredVoxelLabel);
  hoverInfo->addWidget(hoveredSelectedLabel);
  mainVBox->addLayout(hoverInfo);
  mainVBox->addSpacing(10);

  // Colorizer
  QPushButton* colorizerButton = new QPushButton("Show");
  QHBoxLayout* colorizerLayout = new QHBoxLayout();
  QLabel* colorizerLabel = new QLabel("Colorizer");
  colorizerLabel->setStyleSheet("font-weight: bold;");
  colorizerLayout->addWidget(colorizerLabel);
  colorizerLayout->addStretch();
  colorizerLayout->addWidget(colorizerButton);
  mainVBox->addLayout(colorizerLayout);
  segmentationColorizer = new ColorizerWidget();
  segmentationColorizer->setVisible(false);
  segmentationColorizer->getColorizer()->initDefaultMapping();
  mainVBox->addWidget(segmentationColorizer);

  connect(colorizerButton, &QPushButton::clicked, this,
          [colorizerButton, this]() -> void {
            this->segmentationColorizer->setVisible(
                !this->segmentationColorizer->isVisible());
            if (this->segmentationColorizer->isVisible()) {
              colorizerButton->setText("Hide");
            } else {
              colorizerButton->setText("Show");
            }
          });

  // Histogram
  histogram = new HistogramWidget();
  QPushButton* histogramButton = new QPushButton("Hide");
  QHBoxLayout* histogramLayout = new QHBoxLayout();
  QLabel* histogramLabel = new QLabel("Histogram");
  QWidget* radioWidget = new QWidget();
  QHBoxLayout* radioButtonLayout = new QHBoxLayout(radioWidget);
  volumeRadioButton = new QRadioButton("Volume");
  sliceRadioButton = new QRadioButton("Slice");
  radioButtonLayout->addWidget(volumeRadioButton);
  radioButtonLayout->addWidget(sliceRadioButton);
  volumeRadioButton->setChecked(true);
  connect(volumeRadioButton, &QRadioButton::toggled, [this](bool checked) {
    if (checked) {
      histogram->setHistogramProvider(
          this->stepManager->getHistogramProvider());

    } else
      histogram->setHistogramProvider(sliceHistogramProvider);
  });

  histogramLabel->setStyleSheet("font-weight: bold;");
  histogramLayout->addWidget(histogramLabel);
  histogramLayout->addStretch();
  histogramLayout->addWidget(histogramButton);
  mainVBox->addLayout(histogramLayout);

  mainVBox->addWidget(radioWidget);
  mainVBox->addWidget(histogram);

  connect(stepManager, &StepManager::inputVolChanged, this, [=](Node* node) {
    Q_UNUSED(node);
    if (stepManager->getHistogramProvider()) {
      volumeHistogramProvider = stepManager->getHistogramProvider();
      this->segmentationColorizer->setHistogramProvider(
          volumeHistogramProvider);
      if (volumeRadioButton->isChecked()) {
        histogram->setHistogramProvider(volumeHistogramProvider);
      }
    }
  });
  connect(histogramButton, &QPushButton::clicked, this,
          [histogramButton, radioWidget, this]() -> void {
            this->histogram->setVisible(!this->histogram->isVisible());
            radioWidget->setVisible(!radioWidget->isVisible());
            if (this->histogram->isVisible()) {
              histogramButton->setText("Hide");
            } else {
              histogramButton->setText("Show");
            }
          });

  // Trigger a histogram color update when the colorizer changes by setting it
  // again
  connect(
      this->segmentationColorizer->getColorizer().data(),
      &vx::Colorizer::mappingChanged, this, [=]() {
        histogram->setColorizer(this->segmentationColorizer->getColorizer());
      });

  // Operation history
  QHBoxLayout* historyLayout = new QHBoxLayout();
  QLabel* historyLabel = new QLabel("Operation History");
  historyLabel->setStyleSheet("font-weight: bold;");
  historyLayout->addWidget(historyLabel);
  historyLayout->addStretch();
  QPushButton* historyButton = new QPushButton("Show");
  historyLayout->addWidget(historyButton);
  mainVBox->addLayout(historyLayout);

  historyListView = new QListView();
  historyListView->setModel(historyViewModel);
  historyListView->setFixedHeight(150);
  historyListView->setVisible(false);
  historyListView->setAlternatingRowColors(true);
  historyListView->setStyleSheet("QListView::item { padding: 2px; }");
  historyListView->setSelectionMode(QAbstractItemView::ExtendedSelection);

  auto actDelete = new QAction("Delete", this);
  connect(actDelete, &QAction::triggered, this, [this]() {
    QList<int> stepIndices;
    auto indices = this->historyListView->selectionModel()->selectedRows();
    for (auto idx : indices) {
      stepIndices.append(idx.row());
    }
    this->stepManager->removeSteps(stepIndices);
  });
  historyListView->setContextMenuPolicy(Qt::ActionsContextMenu);
  historyListView->addActions({actDelete});

  mainVBox->addWidget(historyListView);
  connect(
      historyButton, &QPushButton::clicked, this,
      [historyButton, this]() -> void {
        this->historyListView->setVisible(!this->historyListView->isVisible());
        if (this->historyListView->isVisible()) {
          historyButton->setText("Hide");
        } else {
          historyButton->setText("Show");
        }
      });
}

QList<int> SegmentationWidget::getSelectedIndices() {
  QList<int> labelIdx;
  QItemSelectionModel* tableSelectModel = this->tableView->selectionModel();
  if (tableSelectModel->hasSelection()) {
    QModelIndexList rowSelection = tableSelectModel->selectedRows();
    for (int i = 0; i < rowSelection.count(); i++) {
      labelIdx.append(rowSelection.at(i).row());
    }
  }
  return labelIdx;
}

QList<SegmentationType> SegmentationWidget::getSelectedLabelIDs() {
  QList<SegmentationType> labelIDs;
  QItemSelectionModel* tableSelectModel = this->tableView->selectionModel();
  if (tableSelectModel->hasSelection()) {
    QModelIndexList rowSelection = tableSelectModel->selectedRows();
    for (int i = 0; i < rowSelection.count(); i++) {
      quint64 index = rowSelection.at(i).row();
      labelIDs.append(labelViewModel->getLabelTable()
                          ->getRowColumnData(index, "LabelID")
                          .toUInt());
    }
  }
  return labelIDs;
}

QList<SegmentationType> SegmentationWidget::getLabelIDs() {
  QList<SegmentationType> labelIDs;
  for (int rowIndex = 0; rowIndex < labelViewModel->rowCount(); rowIndex++) {
    labelIDs.append(labelViewModel->getLabelTable()
                        ->getRowColumnData(rowIndex, "LabelID")
                        .toUInt());
  }
  return labelIDs;
}

QList<SegmentationType> SegmentationWidget::getNotSelectedLabelIDs() {
  QList<SegmentationType> selectedLabels = getSelectedLabelIDs();
  QList<SegmentationType> allLabels = getLabelIDs();
  QList<SegmentationType> notSelectedLabels;

  for (auto label : allLabels) {
    if (!selectedLabels.contains(label)) {
      notSelectedLabels.append(label);
    }
  }

  return notSelectedLabels;
}

void SegmentationWidget::uncheckBrushes() {
  isChangingBrushState = true;
  this->brushToggleButton->setChecked(false);
  this->eraserToggleButton->setChecked(false);
  isChangingBrushState = false;
}

void SegmentationWidget::uncheckLasso() {
  this->lassoToggleButton->setChecked(false);
}

void SegmentationWidget::setSliceHistogramProvider(
    QSharedPointer<vx::HistogramProvider> provider) {
  sliceHistogramProvider = provider;
  if (sliceRadioButton) {
    if (sliceRadioButton->isChecked()) {
      histogram->setHistogramProvider(provider);
    }
  }
}

ColorizerWidget* SegmentationWidget::getColorizerWidget() {
  return segmentationColorizer;
}

void SegmentationWidget::setActiveSelectionInfo(bool incremental,
                                                qint64 updateVoxelCount,
                                                qint64 totalVoxelCount) {
  if (incremental) {
    this->activeSelectionVoxelCount =
        std::max(this->activeSelectionVoxelCount + updateVoxelCount, (qint64)0);
  } else {
    this->activeSelectionVoxelCount = updateVoxelCount;
  }

  QString count = QString::number(this->activeSelectionVoxelCount);
  float p = totalVoxelCount == 0 ? 0
                                 : (float)this->activeSelectionVoxelCount /
                                       totalVoxelCount * 100;
  QString percentage = QString("%1%").arg(p, 0, 'f', 1);
  this->selectedVoxelCount->setText(QString("Voxels: %1").arg(count));
  this->selectedVoxelPercentage->setText(
      QString("Percentage: %1").arg(percentage));
}

void SegmentationWidget::setHoverInfo(double voxelValue, QString voxelLabel,
                                      bool selected) {
  this->hoveredVoxelValue->setText(QString("Voxel value: %1").arg(voxelValue));
  this->hoveredVoxelLabel->setText(QString("Voxel label: %1").arg(voxelLabel));
  this->hoveredSelectedLabel->setText(
      vx::format("Selected: {}", selected ? "yes" : "no"));
}

int SegmentationWidget::addLabel() {
  if (this->labelViewModel->getLabelIdCounter() >
      (std::numeric_limits<SegmentationType>::max() >> 1)) {
    qWarning() << "Can not create more labels than allowed by SegmentationType";
    return -1;
  }

  int labelID = this->labelViewModel->getLabelIdCounter();
  stepManager->addLabelTableRow(
      labelID, QString("Label #%1").arg(labelID), "",
      QVariant::fromValue(
          Color(defaultColorPalette.at(labelID % defaultColorPalette.size()))
              .asTuple()),
      true);

  if (this->labelViewModel->getLabelIdCounter() >
      (std::numeric_limits<SegmentationType>::max() >> 1)) {
    this->addLabelButton->setEnabled(false);
  }
  return labelID;
}

int SegmentationWidget::addLabelByColor(Color color) {
  if (this->labelViewModel->getLabelIdCounter() >
      (std::numeric_limits<SegmentationType>::max() >> 1)) {
    qWarning() << "Can not create more labels than allowed by SegmentationType";
    return -1;
  }

  int labelID = this->labelViewModel->getLabelIdCounter();
  stepManager->addLabelTableRow(labelID, QString("Label #%1").arg(labelID), "",
                                QVariant::fromValue(color.asTuple()), true);

  if (this->labelViewModel->getLabelIdCounter() >
      (std::numeric_limits<SegmentationType>::max() >> 1)) {
    this->addLabelButton->setEnabled(false);
  }
  return labelID;
}

void SegmentationWidget::colorizeLabels() {
  QList<int> rowIndices = getSelectedIndices();
  if (rowIndices.empty()) {
    return;
  }
  QColor color =
      QColorDialog::getColor(Color(labelViewModel->getLabelTable()
                                       ->getRowColumnData(0, "Color")
                                       .value<TupleVector<double, 4>>())
                                 .asQColor());
  if (color.isValid()) {
    for (int i = 0; i < rowIndices.size(); i++) {
      int labelID = labelViewModel->getLabelTable()
                        ->getRowColumnData(rowIndices.at(i), "LabelID")
                        .toInt();
      stepManager->createMetaStep(labelID, "Color",
                                  QVariant::fromValue(Color(color).asTuple()));
    }
  }
}

void SegmentationWidget::selectLabelVoxels() {
  QList<SegmentationType> labelIds = this->getSelectedLabelIDs();
  this->stepManager->setManualSelection(labelIds);
}

void SegmentationWidget::deleteLabels() {
  QList<SegmentationType> selectedLabelIDs = getSelectedLabelIDs();

  stepManager->createRemoveLabelStep(selectedLabelIDs);
}

void SegmentationWidget::addSelectionToLabel() {
  QList<SegmentationType> labelIDs = this->getSelectedLabelIDs();
  if (labelIDs.size() > 1) {
    qWarning() << "SegmentationWidget::addSelectionToLabels(): Too many labels "
                  "selected";
    return;
  }

  if (labelIDs.size() < 1) {
    int labelID = this->addLabel();
    labelIDs.append(labelID);
  }

  this->stepManager->createAssignmentStep((qlonglong)labelIDs.at(0));
  this->resetSegmentationWidgets();
  Q_EMIT(this->resetActiveSelection());
  this->tableView->clearSelection();
}

void SegmentationWidget::subtractSelectionFromLabels() {
  QList<int> rowIndices = getSelectedIndices();
  for (int i = rowIndices.size() - 1; i >= 0; i--) {
    int labelID = labelViewModel->getLabelTable()
                      ->getRowColumnData(rowIndices.at(i), "LabelID")
                      .toUInt();
    stepManager->createSubtractStep(labelID);
  }
  this->resetSegmentationWidgets();
  Q_EMIT(this->resetActiveSelection());
  this->tableView->clearSelection();
}

void SegmentationWidget::resetSelection() {
  this->stepManager->clearSelection();
  this->resetSegmentationWidgets();
  Q_EMIT(this->resetActiveSelection());
}

void SegmentationWidget::resetSegmentationWidgets() {
  this->isResettingFields = true;
  Q_EMIT(this->resetUIWidgets());
  Q_EMIT(this->stepManager->resetUIWidgets());
  this->brushToggleButton->setChecked(false);
  this->lassoToggleButton->setChecked(false);
  this->isResettingFields = false;
}
