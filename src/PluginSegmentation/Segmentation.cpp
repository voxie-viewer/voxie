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

#include "Segmentation.hpp"
#include <qcolordialog.h>
#include <qheaderview.h>
#include <Voxie/Node/Types.hpp>

#include <Voxie/Data/ContainerData.hpp>
#include <VoxieBackend/IO/Operation.hpp>
#include <VoxieBackend/IO/OperationRegistry.hpp>

#include <Voxie/Gui/WindowMode.hpp>

#include <PluginSegmentation/Prototypes.hpp>
#include <QApplication>
#include <QInputDialog>
#include <QLabel>
#include <QObject>
#include <QSharedPointer>

#include <Voxie/Node/NodePrototype.hpp>

#include <Voxie/Data/TableData.hpp>

#include <PluginSegmentation/SegmentationUtils.hpp>
#include <VoxieBackend/IO/SharpThread.hpp>

VX_NODE_INSTANTIATION(vx::filters::Segmentation)

using namespace vx::filters;
using namespace vx;
using namespace vx::io;

enum class WindowMode;

Segmentation::Segmentation()
    : FilterNode(getPrototypeSingleton()),
      properties(new SegmentationProperties(this)) {
  setAutomaticDisplayName("Segmentation");

  // Actually in the GUI visible table columns
  TableColumn tableColumn0 =
      TableColumn("Export", vx::types::BooleanType(), "Export",
                  *(new QMap<QString, QDBusVariant>()));
  TableColumn tableColumn1 =
      TableColumn("LabelID", vx::types::IntType(), "LabelID",
                  *(new QMap<QString, QDBusVariant>()));
  TableColumn tableColumn2 =
      TableColumn("Name", vx::types::StringType(), "Name",
                  *(new QMap<QString, QDBusVariant>()));
  TableColumn tableColumn3 =
      TableColumn("Visibility", vx::types::BooleanType(), "Visibility",
                  *(new QMap<QString, QDBusVariant>()));
  TableColumn tableColumn4 =
      TableColumn("Voxels", vx::types::IntType(), "Voxels",
                  *(new QMap<QString, QDBusVariant>()));
  TableColumn tableColumn5 =
      TableColumn("Percent", vx::types::FloatType(), "Percent",
                  *(new QMap<QString, QDBusVariant>()));

  // Color column is placed inside the Name column in the GUI
  TableColumn tableColumn6 =
      TableColumn("Color", vx::types::ColorType(), "Color",
                  *(new QMap<QString, QDBusVariant>()));

  // Description is currently not displayed in the GUI
  TableColumn tableColumn7 =
      TableColumn("Description", vx::types::StringType(), "Description",
                  *(new QMap<QString, QDBusVariant>()));

  QList<TableColumn> columns = {tableColumn0, tableColumn1, tableColumn2,
                                tableColumn3, tableColumn4, tableColumn5,
                                tableColumn6, tableColumn7};
  this->labelTablePointer = TableData::create(columns);
  this->stepManager = new StepManager(this->properties, this);

  labelViewModel =
      new LabelViewModel(this->labelTablePointer, this->stepManager);
  historyViewModel = new HistoryViewModel(this->properties);
  segmentationWidget = new SegmentationWidget(labelViewModel, historyViewModel,
                                              this->stepManager);

  this->setCustomUi(segmentationWidget);
  connect(this, &QObject::destroyed, segmentationWidget, &QAction::deleteLater);

  // addPropertySection(segmentationWidget);
  // TODO: Should changing the input / initial segmentation also rerun all
  // steps?
  connect(this->properties, &SegmentationProperties::inputChanged, this,
          &Segmentation::initData);
  connect(this->properties, &SegmentationProperties::initialSegmentationChanged,
          this, &Segmentation::initData);

  connect(this->segmentationWidget, &SegmentationWidget::resetActiveSelection,
          this, &Segmentation::deactivateBrushes);
  connect(this->segmentationWidget, &SegmentationWidget::resetActiveSelection,
          this, &Segmentation::deactivateLasso);

  // Update active selection labels
  connect(this->stepManager, &StepManager::updateSelectedVoxelCount, this,
          [this](qint64 voxelCount, bool incremental) {
            qint64 totalVoxelCount = 0;
            if (voxelCount > 0 || incremental) {
              auto inputVoxelData = qSharedPointerDynamicCast<VolumeDataVoxel>(
                  dynamic_cast<VolumeNode*>(this->properties->input())
                      ->volumeData());
              auto size = inputVoxelData->getDimensions();
              totalVoxelCount = size.x * size.y * size.z;
            }
            this->segmentationWidget->setActiveSelectionInfo(
                incremental, voxelCount, totalVoxelCount);
          });

  connect(voxieRoot().activeVisualizerProvider(),
          &vx::ActiveVisualizerProvider::activeVisualizerChanged, this,
          [=](vx::VisualizerNode* object) {
            auto interf = dynamic_cast<SliceVisualizerI*>(object);

            if (interf) {
              segmentationWidget->setSliceHistogramProvider(
                  interf->getSliceHistogramProvider());
            }
          });

  auto colorizerWidget = segmentationWidget->getColorizerWidget();
  connect(colorizerWidget->getColorizer().data(),
          &vx::Colorizer::mappingChanged, this, [this]() {
            auto colorizerWidget2 = segmentationWidget->getColorizerWidget();
            auto colorizer = colorizerWidget2->getColorizer();
            if (colorizer) {
              colorizerWidget2->setColorizer(colorizer);
              SliceVisualizerXY->setHistogramColorizer(colorizer);
              SliceVisualizerYZ->setHistogramColorizer(colorizer);
              SliceVisualizerXZ->setHistogramColorizer(colorizer);
            }
          });
  connect(
      voxieRoot().activeVisualizerProvider(),
      &vx::ActiveVisualizerProvider::customUiOpened, this, [this](Node* node) {
        if (SliceVisualizerXZ && SliceVisualizerYZ && SliceVisualizerXY) {
          return;
        }
        int sliceVisualizersFound = 0;
        QMdiArea* documentArea =
            ((gui::CoreWindow*)voxieRoot().mainWindow())->getMdiArea();
        QVector2D size = QVector2D((int)(documentArea->width() / 2),
                                   (int)(documentArea->height() / 2));

        for (auto n : node->childNodes()) {
          if (n->nodeKind() == vx::NodeKind::Visualizer) {
            if (std::get<1>(n->manualDisplayName()) ==
                "SegmentationViewSliceXY") {
              this->SliceVisualizerXY = dynamic_cast<SliceVisualizerI*>(n);
              this->stepManager->brushSelectionXY->setVisualizer(
                  SliceVisualizerXY);
              this->stepManager->lassoSelectionXY->setVisualizer(
                  SliceVisualizerXY);
              configureSliceVisualizer((vx::VisualizerNode*)n,
                                       this->SliceVisualizerXY,
                                       QVector2D(size.x(), 0), size);
              SliceVisualizerXY->setRotation(rotationXY);
              sliceVisualizersFound++;

            } else if (std::get<1>(n->manualDisplayName()) ==
                       "SegmentationViewSliceYZ") {
              this->SliceVisualizerYZ = dynamic_cast<SliceVisualizerI*>(n);
              this->stepManager->brushSelectionYZ->setVisualizer(
                  SliceVisualizerYZ);
              this->stepManager->lassoSelectionYZ->setVisualizer(
                  SliceVisualizerYZ);
              configureSliceVisualizer((vx::VisualizerNode*)n,
                                       this->SliceVisualizerYZ,
                                       QVector2D(size.x(), size.y()), size);
              SliceVisualizerYZ->setRotation(rotationYZ);
              sliceVisualizersFound++;

            } else if (std::get<1>(n->manualDisplayName()) ==
                       "SegmentationViewSliceXZ") {
              this->SliceVisualizerXZ = dynamic_cast<SliceVisualizerI*>(n);
              this->stepManager->brushSelectionXZ->setVisualizer(
                  SliceVisualizerXZ);
              this->stepManager->lassoSelectionXZ->setVisualizer(
                  SliceVisualizerXZ);
              configureSliceVisualizer((vx::VisualizerNode*)n,
                                       this->SliceVisualizerXZ,
                                       QVector2D(0, size.y()), size);
              SliceVisualizerXZ->setRotation(rotationXZ);
              sliceVisualizersFound++;
            }
          }
        }

        // Spawn new slice visualizers if not all were existing
        if (sliceVisualizersFound < 3 && node == this &&
            dynamic_cast<ContainerNode*>(this->properties->output())) {
          if (!SliceVisualizerXZ && !SliceVisualizerYZ && !SliceVisualizerXY) {
            spawnSliceVisualizers();
          }
        }
      });
}

void Segmentation::initData() {
  // init input volume
  auto inputNodePtr = this->properties->input();
  QSharedPointer<VolumeNode> inputNode;
  if (inputNodePtr)
    inputNode =
        qSharedPointerDynamicCast<VolumeNode>(inputNodePtr->thisShared());

  // TODO: Something here is broken when the inputNode gets deleted

  if (!inputNode) {
    Q_EMIT(this->segmentationWidget->isInputExisting(false));
    return;
  } else {
    Q_EMIT(this->segmentationWidget->isInputExisting(true));
    this->stepManager->setVolume(inputNode);
  }

  // init compound
  auto inputVoxelData =
      qSharedPointerDynamicCast<VolumeDataVoxel>(inputNode->volumeData());
  if (!inputVoxelData) {
    qWarning() << "Segmentation filter got non-voxel data";
    // TODO: What to do here?
    Q_EMIT(this->segmentationWidget->isInputExisting(false));
    return;
  }

  vx::VectorSizeT3 inputSize = inputVoxelData->getDimensions();

  // TODO: Should "InitialSegmentation" be a property of the Segmentation or
  // should there be a first step which imports the segmentation result?
  auto initialSegmentationNode =
      dynamic_cast<ContainerNode*>(this->properties->initialSegmentation());
  auto initialSegmentation = initialSegmentationNode
                                 ? initialSegmentationNode->getCompoundPointer()
                                 : QSharedPointer<ContainerData>();
  // TODO: Non-voxel data?
  auto initialSegmentationLabel =
      initialSegmentation ? qSharedPointerDynamicCast<VolumeDataVoxel>(
                                initialSegmentation->getElement("labelVolume"))
                          : QSharedPointer<VolumeDataVoxel>();
  auto initialSegmentationTable =
      initialSegmentation ? qSharedPointerDynamicCast<TableData>(
                                initialSegmentation->getElement("labelTable"))
                          : QSharedPointer<TableData>();
  if (initialSegmentation &&
      (!initialSegmentationLabel || !initialSegmentationTable)) {
    qWarning() << "Got broken initial segmentation";
    initialSegmentation.reset();
    initialSegmentationLabel.reset();
    initialSegmentationTable.reset();
  }

  auto labelContainer =
      dynamic_cast<ContainerNode*>(this->properties->output());

  if (!labelContainer) {
    // TODO: Should creation of the container be delayed until it is used the first time?
    qWarning() << "No Label Container connected, create a new one";
    // create ContainerNode
    labelContainer = dynamic_cast<ContainerNode*>(
        ContainerNode::getPrototypeSingleton()
            ->create(QMap<QString, QVariant>(), QList<Node*>(),
                     QMap<QString, QDBusVariant>())
            .data());
    this->properties->setOutput(labelContainer);
  }

  // set slice visualizers to new input volume
  if (SliceVisualizerXZ && SliceVisualizerYZ && SliceVisualizerXY) {
    SliceVisualizerXZ->setVolume(inputNode.data());
    SliceVisualizerXZ->setRotation(rotationXZ);

    SliceVisualizerYZ->setVolume(inputNode.data());
    SliceVisualizerYZ->setRotation(rotationYZ);

    SliceVisualizerXY->setVolume(inputNode.data());
    SliceVisualizerXY->setRotation(rotationXY);
  }
  QSharedPointer<ContainerData> containerData =
      labelContainer->getCompoundPointer();
  // TODO: What is op being used for? Remove it?
  auto op = Operation::create();
  // if dimensions changed reinit the whole volume
  // TODO: This probably should look at the actual size of the container instead
  // of using a cached version in lastDimensions.
  if (lastDimensions.x != inputSize.x || lastDimensions.y != inputSize.y ||
      lastDimensions.z != inputSize.z || !containerData) {
    // reinit label volume
    lastDimensions = inputSize;

    // create output volume from input volume
    QSharedPointer<vx::VolumeDataVoxel> outputData =
        VolumeDataVoxel::createVolume(
            {inputSize.x, inputSize.y, inputSize.z},
            vx::DataTypeTraitsByType<SegmentationType>::getDataType(),
            inputVoxelData->volumeOrigin(), inputVoxelData->gridSpacing());

    // TODO: Why is another operation being created here?
    // auto op = Operation::create();

    // TODO: Clean up updates

    // Add ContainerData
    if (!containerData) {
      containerData = ContainerData::create("LabelContainer");
      auto outerUpdate = containerData->createUpdate();
      (*containerData).insertElement("labelVolume", outputData, outerUpdate);
      (*containerData)
          .insertElement("labelTable", this->labelTablePointer, outerUpdate);
      outerUpdate->finish({});
    } else {
      // TODO: pass update to insertElement function and check if it is existent
      auto outerUpdate = containerData->createUpdate();
      (*containerData).insertElement("labelVolume", outputData, outerUpdate);
      outerUpdate->finish({});
    }

    labelContainer->setCompoundPointer(containerData);
  }

  auto outerUpdate = containerData->createUpdate();

  // TODO: This should not happen on the main thread
  // TODO: Use task / cancellationToken?
  if (initialSegmentationLabel) {
    // initialize volume with inital data
    // TODO: Should this use some generic volume conversion routine?
    // TODO: Recreate label statistics here instead of just loading them from
    // the file?
    initialSegmentationLabel->performInGenericContext([&](auto& initial) {
      iterateAllLabelVolumeVoxels(
          containerData, outerUpdate, nullptr, nullptr, [&](const auto& cb) {
            cb([&](size_t& x, size_t& y, size_t& z,
                   const QSharedPointer<VolumeDataVoxelInst<SegmentationType>>&
                       labelData) {
              auto value = initial.getVoxel(x, y, z);
              labelData->setVoxel(x, y, z, value);
              // if (value) qDebug() << x << y << z << value;
            });
          });
    });
  } else {
    // initialize volume with 0s
    iterateAllLabelVolumeVoxels(
        containerData, outerUpdate, nullptr, nullptr, [&](const auto& cb) {
          cb([&](size_t& x, size_t& y, size_t& z,
                 const QSharedPointer<VolumeDataVoxelInst<SegmentationType>>&
                     labelData) { labelData->setVoxel(x, y, z, 0); });
        });
  }

  auto labelTable = qSharedPointerDynamicCast<TableData>(
      containerData->getElement("labelTable"));

  if (initialSegmentationTable) {
    // TODO: Verify that the table layout matches
    auto update =
        labelTable->createUpdate({{containerData->getPath(), outerUpdate}});
    labelTable->clear(update);
    for (const auto& row : initialSegmentationTable->getRowsByIndex()) {
      auto newRow = labelTable->addRow(update, row.data());
      qDebug() << "Inserted data from row" << row.rowID() << "to row" << newRow;
    }
    update->finish({});
  } else {
    auto update =
        labelTable->createUpdate({{containerData->getPath(), outerUpdate}});
    labelTable->clear(update);
    update->finish({});
  }

  outerUpdate->finish({});
}

void Segmentation::spawnSliceVisualizers() {
  // XY-Visualizer
  QMdiArea* documentArea =
      ((gui::CoreWindow*)voxieRoot().mainWindow())->getMdiArea();
  QVector2D size = QVector2D((int)(documentArea->width() / 2),
                             (int)(documentArea->height() / 2));
  auto positionXY = QVector2D(size.x(), 0);

  if (!SliceVisualizerXY) {
    SliceVisualizerXY = spawnSingleSliceVisualizer(rotationXY, positionXY, size,
                                                   "SegmentationViewSliceXY");
    this->stepManager->brushSelectionXY->setVisualizer(SliceVisualizerXY);
    this->stepManager->lassoSelectionXY->setVisualizer(SliceVisualizerXY);
  }

  // XZ-Visualizer
  auto positionXZ = QVector2D(0, size.y());

  if (!SliceVisualizerXZ) {
    SliceVisualizerXZ = spawnSingleSliceVisualizer(rotationXZ, positionXZ, size,
                                                   "SegmentationViewSliceXZ");
    this->stepManager->brushSelectionXZ->setVisualizer(SliceVisualizerXZ);
    this->stepManager->lassoSelectionXZ->setVisualizer(SliceVisualizerXZ);
  }

  // YZ-Visualizer
  auto positionYZ = QVector2D(size.x(), size.y());

  if (!SliceVisualizerYZ) {
    SliceVisualizerYZ = spawnSingleSliceVisualizer(rotationYZ, positionYZ, size,
                                                   "SegmentationViewSliceYZ");
    this->stepManager->brushSelectionYZ->setVisualizer(SliceVisualizerYZ);
    this->stepManager->lassoSelectionYZ->setVisualizer(SliceVisualizerYZ);
  }
}

SliceVisualizerI* Segmentation::spawnSingleSliceVisualizer(
    QQuaternion planeOrientation, QVector2D guiPosition, QVector2D size,
    QString manualDisplayName) {
  auto sliceVisualizer = dynamic_cast<vx::VisualizerNode*>(
      voxieRoot()
          .components()
          ->getComponentTyped<NodePrototype>(
              "de.uni_stuttgart.Voxie.Visualizer.Slice", false)
          ->create(
              {{"de.uni_stuttgart.Voxie.Visualizer.Slice.SegmentationFilter",
                QVariant::fromValue(vx::PropertyValueConvertRaw<
                                    QDBusObjectPath, vx::Node*>::toRaw(this))},
               {"de.uni_stuttgart.Voxie.Visualizer.Slice.Volume",
                QVariant::fromValue(
                    vx::PropertyValueConvertRaw<QDBusObjectPath, vx::Node*>::
                        toRaw(this->properties->input()))},
               {"de.uni_stuttgart.Voxie.Visualizer.Slice.LabelContainer",
                QVariant::fromValue(
                    vx::PropertyValueConvertRaw<QDBusObjectPath, vx::Node*>::
                        toRaw(dynamic_cast<ContainerNode*>(
                            this->properties->output())))},

               /* {"de.uni_stuttgart.Voxie.View2D.VerticalSize",
                0.1}, */
               /* {"de.uni_stuttgart.Voxie.View2D.CenterPoint",
                QVariant::fromValue(std::tuple<double, double>(10.0, 10.0))},
              */

               {"de.uni_stuttgart.Voxie.Visualizer.Slice.Plane.Orientation",
                QVariant::fromValue(vx::PropertyValueConvertRaw<
                                    std::tuple<double, double, double, double>,
                                    QQuaternion>::toRaw(planeOrientation))}},
              QList<Node*>(), QMap<QString, QDBusVariant>())
          .data());

  sliceVisualizer->setManualDisplayName(
      std::make_tuple(true, manualDisplayName));

  voxieRoot().setVisualizerPosition(sliceVisualizer,
                                    vectorCast<double>(toVector(guiPosition)));
  voxieRoot().setVisualizerSize(sliceVisualizer,
                                vectorCast<double>(toVector(size)));

  SliceVisualizerI* interf = dynamic_cast<SliceVisualizerI*>(sliceVisualizer);

  configureSliceVisualizer(sliceVisualizer, interf, guiPosition, size);

  return interf;
}

void Segmentation::configureSliceVisualizer(vx::VisualizerNode* sliceVisualizer,
                                            vx::SliceVisualizerI* interf,
                                            QVector2D guiPosition,
                                            QVector2D size) {
  voxieRoot().setVisualizerPosition(sliceVisualizer,
                                    vectorCast<double>(toVector(guiPosition)));
  voxieRoot().setVisualizerSize(sliceVisualizer,
                                vectorCast<double>(toVector(size)));

  connect(voxieRoot().activeVisualizerProvider(),
          &vx::ActiveVisualizerProvider::customUiClosed, this,
          [=](Node* widget) {
            if (widget == this)
              voxieRoot().setVisualizerWindowMode(sliceVisualizer,
                                                  WindowMode::Minimized);
          });
  connect(voxieRoot().activeVisualizerProvider(),
          &vx::ActiveVisualizerProvider::customUiOpened, this,
          [=](Node* widget) {
            if (widget == this) {
              voxieRoot().setVisualizerWindowMode(sliceVisualizer,
                                                  WindowMode::Normal);
            }
          });

  connect(this->segmentationWidget, &SegmentationWidget::toggledBrush, this,
          [interf](bool isSet) {
            if (isSet) {
              interf->activateBrushSelectionTool();
            } else {
              interf->deactivateBrushSelectionTool();
            }
          });
  connect(this->segmentationWidget, &SegmentationWidget::changedRadius, this,
          [interf](int radius) { interf->setBrushRadius(radius); });
  connect(this->segmentationWidget, &SegmentationWidget::toggledLasso, this,
          [interf](bool isSet) {
            if (isSet) {
              interf->activateLassoSelectionTool();
            } else {
              interf->deactivateLassoSelectionTool();
            }
          });
}

QSharedPointer<QObject> Segmentation::getPropertyUIData(QString propertyName) {
  return Node::getPropertyUIData(propertyName);
}

QSharedPointer<RunFilterOperation> Segmentation::calculate(
    bool isAutomaticFilterRun) {
  Q_UNUSED(isAutomaticFilterRun);
  QSharedPointer<RunFilterOperation> operation(
      new RunFilterOperation(), [](QObject* obj) { obj->deleteLater(); });

  qDebug() << "Segmentation::calculate";
  operation->onFinished(
      this, [](const QSharedPointer<Operation::ResultError>& result) {
        qDebug() << "Segmentation::calculate finished" << result;
      });

  OperationRegistry::instance()->addOperation(operation);

  auto stepList = this->properties->stepList();

  // Reset the output
  initData();

  // Execute Steps of SegmentationStepList
  // TODO: This has to be connected to the operation, otherwise the operation
  // will not finish at all.
  this->stepManager->runStepList(stepList);

  return operation;
}

StepManager* Segmentation::getStepManager() { return this->stepManager; }

void Segmentation::deactivateBrushes() {
  if (!isChangingBrushes) {
    isChangingBrushes = true;
    this->segmentationWidget->uncheckBrushes();
    this->SliceVisualizerXY->deactivateBrushSelectionTool();
    this->SliceVisualizerXZ->deactivateBrushSelectionTool();
    this->SliceVisualizerYZ->deactivateBrushSelectionTool();
    isChangingBrushes = false;
  }
}

void Segmentation::deactivateLasso() {
  if (!isChangingLasso) {
    isChangingLasso = true;
    this->segmentationWidget->uncheckLasso();
    this->SliceVisualizerXY->deactivateLassoSelectionTool();
    this->SliceVisualizerXZ->deactivateLassoSelectionTool();
    this->SliceVisualizerYZ->deactivateLassoSelectionTool();
    isChangingLasso = false;
  }
}

void Segmentation::updatePosition(QVector3D position) {
  if (!this->properties->input()) return;
  auto inputData = qSharedPointerDynamicCast<VolumeDataVoxel>(
      dynamic_cast<VolumeNode*>(this->properties->input())->volumeData());

  double value;
  inputData->performInGenericContext([position, &value](auto& volume_inst) {
    value = volume_inst.getVoxelMetric(
        position, vx::InterpolationMethod::NearestNeighbor);
  });

  auto labelContainer =
      dynamic_cast<ContainerNode*>(this->properties->output());

  auto labelData =
      qSharedPointerDynamicCast<VolumeDataVoxelInst<SegmentationType>>(
          labelContainer->getCompoundPointer()->getElement("labelVolume"));
  SegmentationType labelID = (SegmentationType)labelData->getVoxelMetric(
      position, vx::InterpolationMethod::NearestNeighbor);

  QString labelName = "";
  bool selected = getBit(labelID, segmentationShift);
  clearBit(labelID, segmentationShift);

  // Get label name if it exists in the label table
  if (labelID != 0) {
    const QList<TableRow> rows = labelTablePointer->getRowsByIndex();
    const auto row = std::find_if(
        rows.begin(), rows.end(), [this, labelID](TableRow const& tr) {
          return tr.data().at(labelTablePointer->getColumnIndexByName(
                     "LabelID")) == labelID;
        });
    if (row != rows.end()) {
      labelName = getTableRowByLabelID(labelID, this->labelTablePointer)
                      .data()
                      .at(this->labelTablePointer->getColumnIndexByName("Name"))
                      .toString();
    }
  }

  this->segmentationWidget->setHoverInfo(value, labelName, selected);
}

bool Segmentation::isAllowedChild(vx::NodeKind object) {
  return object == NodeKind::Data || object == NodeKind::Visualizer;
}

bool Segmentation::isCreatableChild(NodeKind object) {
  return object == NodeKind::Visualizer;
}
