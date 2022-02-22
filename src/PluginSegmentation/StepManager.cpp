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

#include "StepManager.hpp"
#include <PluginSegmentation/Steps/AssignmentStep.hpp>
#include <PluginSegmentation/Steps/MetaStep.hpp>
#include <PluginSegmentation/Steps/RemoveLabelStep.hpp>
#include <PluginSegmentation/Steps/SubtractStep.hpp>
#include <Voxie/Data/VolumeNode.hpp>
#include <Voxie/Node/PropertyValueConvertRaw.hpp>
#include <VoxieBackend/IO/OperationRegistry.hpp>

using namespace vx;
using namespace vx::io;

StepManager::StepManager(SegmentationProperties* properties,
                         QObject* parent = nullptr)
    : QObject(parent), segmentationProperties(properties) {
  // initialize member filter steps

  registerStep(this->manualSelection);
  registerStep(this->brushSelectionXY);
  registerStep(this->brushSelectionXZ);
  registerStep(this->brushSelectionYZ);
  registerStep(this->lassoSelectionXY);
  registerStep(this->lassoSelectionXZ);
  registerStep(this->lassoSelectionYZ);

  // Needed because: selectPassedVoxels & erasePassedVoxels are not threaded
  connect(this->brushSelectionXY.data(),
          &SegmentationStep::updateSelectedVoxelCount, this,
          &StepManager::updateSelectedVoxelCount);
  connect(this->brushSelectionXZ.data(),
          &SegmentationStep::updateSelectedVoxelCount, this,
          &StepManager::updateSelectedVoxelCount);
  connect(this->brushSelectionYZ.data(),
          &SegmentationStep::updateSelectedVoxelCount, this,
          &StepManager::updateSelectedVoxelCount);

  Q_EMIT this->selectionIsFinished(true);
  Q_EMIT this->canApplyActiveSelection(false);

  histogramProvider = QSharedPointer<vx::HistogramProvider>::create();
}

StepManager::~StepManager() {
  // delete all member selection steps
  this->manualSelection->destroy();
  this->brushSelectionXZ->destroy();
  this->brushSelectionXY->destroy();
  this->brushSelectionYZ->destroy();
  this->lassoSelectionXZ->destroy();
  this->lassoSelectionXY->destroy();
  this->lassoSelectionYZ->destroy();
}

void StepManager::addLabelTableRow(qlonglong labelID, QString name,
                                   QString description, QVariant color,
                                   bool visibility) {
  QSharedPointer<MetaStep> metaStep = qSharedPointerDynamicCast<MetaStep>(
      MetaStep::getPrototypeSingleton()->create(
          {{"de.uni_stuttgart.Voxie.SegmentationStep.MetaStep."
            "ModificationKind",
            "AddLabel"},
           {"de.uni_stuttgart.Voxie.SegmentationStep.MetaStep."
            "LabelID",
            labelID},
           {"de.uni_stuttgart.Voxie.SegmentationStep.MetaStep."
            "Name",
            name},
           {"de.uni_stuttgart.Voxie.SegmentationStep.MetaStep."
            "Description",
            description},
           {"de.uni_stuttgart.Voxie.SegmentationStep.MetaStep."
            "Color",
            color},
           {"de.uni_stuttgart.Voxie.SegmentationStep.MetaStep."
            "Visibility",
            visibility}},
          QList<Node*>(), QMap<QString, QDBusVariant>(), false));
  runStepByKind(metaStep);
}

void StepManager::runStepList(const QList<vx::Node*>& stepList) {
  if (!isDataAvailable()) {
    return;
  }
  executionQueue.clear();

  if (this->runningStep.isNull()) {
    for (auto stepNode : stepList) {
      SegmentationStep* step = dynamic_cast<SegmentationStep*>(stepNode);

      executionQueue.append(
          qSharedPointerDynamicCast<SegmentationStep>(step->thisShared()));
    }
    runStep();
  }
}

void StepManager::runStep() {
  if (!isDataAvailable()) {
    return;
  }
  if (!this->executionQueue.isEmpty()) {
    QSharedPointer<SegmentationStep> step = this->executionQueue.takeFirst();
    Q_EMIT this->voxelOpIsFinished(false);

    connect(step.data(), &SegmentationStep::updateSelectedVoxelCount, this,
            &StepManager::updateSelectedVoxelCount);

    if (step->getStepKind() == StepKind::SelectionStep) {
      Q_EMIT this->selectionIsFinished(false);
      Q_EMIT this->canApplyActiveSelection(false);
    }
    QSharedPointer<ParameterCopy> parameterCopy = ParameterCopy::getParameters(
        {step.data(), getInputVolume().data()}, step.data());

    QSharedPointer<ContainerData> containerData = getOutputData();
    QSharedPointer<VolumeNode> volumeNode = getInputVolume();

    QSharedPointer<Operation> op;
    try {
      this->runningStep = step;
      QSharedPointer<OperationResult> opResult =
          step->calculate(parameterCopy, containerData, volumeNode->getPath());
      op = opResult->operation();
    } catch (vx::Exception& e) {
      op = OperationSimple::create();
      op->finish(createQSharedPointer<vx::io::Operation::ResultError>(
          createQSharedPointer<Exception>(e)));
    }

    op->onFinished(
        step.data(),
        [this,
         step](const QSharedPointer<vx::io::Operation::ResultError>& result) {
          Q_UNUSED(result);
          if (step->getStepKind() == StepKind::SelectionStep) {
            Q_EMIT selectionIsFinished(true);
            Q_EMIT canApplyActiveSelection(true);
          } else if (step->getStepKind() == StepKind::LabelStep) {
            Q_EMIT voxelOpIsFinished(true);
            Q_EMIT canApplyActiveSelection(false);
            if (!this->segmentationProperties->stepList().contains(step.data()))
              addStepToList(step->clone(true));
          } else {
            if (!this->segmentationProperties->stepList().contains(step.data()))
              addStepToList(step->clone(true));
            Q_EMIT voxelOpIsFinished(true);
          }

          this->runningStep.clear();
          this->runStep();
        });
  }
  return;
}

void StepManager::runStepByKind(QSharedPointer<SegmentationStep> step) {
  if (!isDataAvailable()) {
    return;
  }
  Q_EMIT this->voxelOpIsFinished(false);

  if (step->getStepKind() == StepKind::SelectionStep) {
    // iterate step queue backward and cancel selection steps in queue
    Q_EMIT this->selectionIsFinished(false);
    QMutableListIterator<QSharedPointer<SegmentationStep>> i(
        this->executionQueue);
    while (i.hasNext()) {
      QSharedPointer<SegmentationStep> loopStep = i.next();
      if (loopStep->getStepKind() == StepKind::SelectionStep) {
        i.remove();
      }
    }
    // append selection steps to special member selection step list first
    this->executionQueue.append(step);
    if (!this->selectionStepList.contains(step))
      this->selectionStepList.append(step);
  }

  else {
    this->executionQueue.append(step);
  }
  // only start new run if no current run is ongoing
  if (this->runningStep.isNull()) {
    this->runStep();
  }
}

QSharedPointer<VolumeNode> StepManager::getInputVolume() const {
  auto inputNodePtr = this->segmentationProperties->input();
  if (inputNodePtr)
    return qSharedPointerDynamicCast<VolumeNode>(inputNodePtr->thisShared());
  else
    return QSharedPointer<VolumeNode>();
}

QSharedPointer<ContainerData> StepManager::getOutputData() const {
  auto outputNodePtr =
      dynamic_cast<ContainerNode*>(this->segmentationProperties->output());
  if (outputNodePtr)
    return qSharedPointerDynamicCast<ContainerData>(
        outputNodePtr->getCompoundPointer()->thisShared());
  else
    return QSharedPointer<ContainerData>();
}

void StepManager::addStepToList(
    QSharedPointer<vx::SegmentationStep> step) const {
  QList<vx::Node*> steps = this->segmentationProperties->stepList();
  steps.append(step.data());
  this->segmentationProperties->setStepList(steps);
  return;
}

void StepManager::printList() {
  QList<vx::Node*> stepList = this->segmentationProperties->stepList();
  qDebug() << "------START stepList------";
  for (auto stepEntry : stepList) {
    auto step = dynamic_cast<SegmentationStep*>(stepEntry);
    try {
      qDebug() << step->getInfoString();
    } catch (...) {
      qDebug() << "Print steplist error";
    }
  }
  qDebug() << "------END stepList------";
}

void StepManager::printSelectionStepList() {
  qDebug() << "------START Selection stepList------";
  for (auto step : this->selectionStepList) {
    try {
      qDebug() << step->getInfoString();
    } catch (...) {
      qDebug() << "Print Selection stepList error";
    }
  }
  qDebug() << "------END Selection stepList------";
}

void StepManager::removeSteps(const QList<int>& indices) {
  QList<vx::Node*> stepList = this->segmentationProperties->stepList();
  QListIterator<int> i(indices);

  i.toBack();
  while (i.hasPrevious()) {
    int idx = i.previous();
    if (idx < stepList.size()) {
      auto step = dynamic_cast<SegmentationStep*>(stepList[idx]);
      stepList.removeAt(idx);
      step->destroy();
    }
  }
  this->segmentationProperties->setStepList(stepList);
}

void StepManager::createRemoveLabelStep(QList<SegmentationType> labelIDs) {
  QSharedPointer<RemoveLabelStep> removeLabelStep =
      qSharedPointerDynamicCast<RemoveLabelStep>(
          RemoveLabelStep::getPrototypeSingleton()->create(
              {}, QList<Node*>(), QMap<QString, QDBusVariant>(), false));

  // cast values
  QList<qint64> labels = QList<qint64>();
  for (auto label : labelIDs) {
    labels.append(label);
  }

  removeLabelStep->setProperties(labels);
  this->runStepByKind(removeLabelStep);
  return;
}

void StepManager::createMetaStep(qlonglong labelID, QString key,
                                 QVariant value) {
  QSharedPointer<MetaStep> metaStep = qSharedPointerDynamicCast<MetaStep>(
      MetaStep::getPrototypeSingleton()->create(
          {
              {"de.uni_stuttgart.Voxie.SegmentationStep.MetaStep."
               "ModificationKind",
               key},
              {"de.uni_stuttgart.Voxie.SegmentationStep.MetaStep."
               "LabelID",
               labelID},
              {"de.uni_stuttgart.Voxie.SegmentationStep.MetaStep." + key,
               value},
          },
          QList<Node*>(), QMap<QString, QDBusVariant>(), false));
  this->runStepByKind(metaStep);
}

void StepManager::createAssignmentStep(qlonglong labelID) {
  QSharedPointer<AssignmentStep> assignmentStep = qSharedPointerDynamicCast<
      AssignmentStep>(AssignmentStep::getPrototypeSingleton()->create(
      {{"de.uni_stuttgart.Voxie.SegmentationStep.AssignmentStep.LabelID",
        labelID}},
      QList<Node*>(), QMap<QString, QDBusVariant>(), false));
  if (!selectionStepList.empty()) {
    for (auto step : this->selectionStepList) {
      addStepToList(step->clone(true));
    }

    this->selectionStepList.clear();

    this->runStepByKind(assignmentStep);
  }
}

void StepManager::createSubtractStep(qint64 labelId) {
  QSharedPointer<SubtractStep> subtractStep =
      qSharedPointerDynamicCast<SubtractStep>(
          SubtractStep::getPrototypeSingleton()->create(
              {{"de.uni_stuttgart.Voxie.SegmentationStep.SubtractStep.LabelId",
                labelId}},
              QList<Node*>(), QMap<QString, QDBusVariant>(), false));

  if (!this->selectionStepList.empty()) {
    for (auto step : this->selectionStepList) {
      addStepToList(step->clone(true));
    }
    this->selectionStepList.clear();
    this->runStepByKind(subtractStep);
  }
  Q_EMIT(this->updateSelectedVoxelCount(0, false));
}

void StepManager::setManualSelection(QList<SegmentationType> labelIds) {
  if (!isDataAvailable()) {
    return;
  }

  // cast values
  QList<qint64> labels = QList<qint64>();
  for (auto label : labelIds) {
    labels.append(label);
  }
  // update parameters for the run
  this->manualSelection->setProperties(labels);
  this->runStepByKind(this->manualSelection->clone(false));
}

void StepManager::clearSelectionStepList() { this->selectionStepList.clear(); }

bool StepManager::isDataAvailable() const {
  bool dataIsAvailable = true;
  auto volumeNode = getInputVolume();
  auto containerData = getOutputData();
  if (!volumeNode) {
    dataIsAvailable = false;
  } else if (volumeNode.isNull()) {
    dataIsAvailable = false;
  }
  if (!containerData) {
    dataIsAvailable = false;
  } else if (containerData.isNull()) {
    dataIsAvailable = false;
  }
  if (!dataIsAvailable) {
    qWarning() << "StepManager::isDataAvailable encountered a "
                  "null pointer in the data used";
  }

  return dataIsAvailable;
}

void StepManager::setLassoSelection(QList<QVector3D> nodes, PlaneInfo plane,
                                    SliceVisualizerI* currentSV) {
  if (!isDataAvailable()) {
    return;
  }

  QSharedPointer<LassoSelectionStep> step =
      qSharedPointerDynamicCast<LassoSelectionStep>(
          this->getSelectionSteps(currentSV)["LassoStep"]);

  // update parameters for the run
  step->setProperties(
      nodes, getInputVolume()->origin(), getInputVolume()->orientation(),
      this->getLabelVolume()->getSpacing(), plane.origin, plane.rotation);

  this->runStepByKind(step->clone(false));
}

void StepManager::setBrushSelectionProperties(PlaneInfo plane,
                                              SliceVisualizerI* currentSV) {
  if (!isDataAvailable()) {
    return;
  }

  // update parameters for the run
  QSharedPointer<BrushSelectionStep> step =
      qSharedPointerDynamicCast<BrushSelectionStep>(
          this->getSelectionSteps(currentSV)["BrushStep"]);

  // Apeends Brush steps also, when plane did not change but tools was
  // deactivated & activated
  if (this->selectionStepList.removeOne(step)) {
    this->selectionStepList.append(step->clone(false));
  }

  step->setProperties(
      QList<std::tuple<QVector3D, double>>(),
      QList<std::tuple<QVector3D, double>>(), getInputVolume()->origin(),
      getInputVolume()->orientation(), this->getLabelVolume()->getSpacing(),
      plane.origin, plane.rotation);
}

void StepManager::addVoxelsToBrushSelection(
    std::tuple<QVector3D, double> centerWithRadius, PlaneInfo plane,
    SliceVisualizerI* currentSV) {
  if (!isDataAvailable()) {
    return;
  }
  Q_EMIT this->selectionIsFinished(false);
  Q_EMIT this->canApplyActiveSelection(false);

  QSharedPointer<BrushSelectionStep> step =
      qSharedPointerDynamicCast<BrushSelectionStep>(
          this->getSelectionSteps(currentSV)["BrushStep"]);

  QSharedPointer<ParameterCopy> parameterCopy =
      ParameterCopy::getParameters(step.data());
  auto op = OperationSimple::create();

  OperationRegistry::instance()->addOperation(op);
  connect(op.data(), &OperationSimple::finished, this,
          &StepManager::selectionIsFinished);
  QSharedPointer<ContainerData> containerData = getOutputData();

  BrushSelectionStepPropertiesCopy propertyCopy(
      parameterCopy->properties()[parameterCopy->mainNodePath()]);

  // check if plane orientation or origin has changed
  if (!(propertyCopy.planeOrientation() == plane.rotation) ||
      !(propertyCopy.planeOrigin() == plane.origin)) {
    // remove step that is currently in the queue
    if (this->selectionStepList.removeOne(step)) {
      // check if step was used
      if (propertyCopy.brushEraseCentersWithRadius().size() > 0 ||
          propertyCopy.brushSelectCentersWithRadius().size() > 0) {
        this->selectionStepList.append(step->clone(false));

        // update plane
        step->setPlaneProperties(plane);
      }
    }
  }

  switch (this->brushMode) {
    case select:
      step->selectPassedVoxels(centerWithRadius, parameterCopy, containerData,
                               op);
      break;
    case erase:
      step->erasePassedVoxels(centerWithRadius, parameterCopy, containerData,
                              op);
      break;
  }

  if (!this->selectionStepList.contains(step)) {
    this->selectionStepList.append(step);
  }

  Q_EMIT this->selectionIsFinished(true);
  Q_EMIT this->canApplyActiveSelection(true);
}

QMap<QString, QSharedPointer<SegmentationStep>> StepManager::getSelectionSteps(
    SliceVisualizerI* sV) {
  QSharedPointer<BrushSelectionStep> brushStep;
  QSharedPointer<LassoSelectionStep> lassoStep;

  if (sV == this->brushSelectionXY->getVisualizer()) {
    brushStep = this->brushSelectionXY;
    lassoStep = this->lassoSelectionXY;
  } else if (sV == this->brushSelectionXZ->getVisualizer()) {
    brushStep = this->brushSelectionXZ;
    lassoStep = this->lassoSelectionXZ;

  } else if (sV == this->brushSelectionYZ->getVisualizer()) {
    brushStep = this->brushSelectionYZ;
    lassoStep = this->lassoSelectionYZ;

  } else {
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.Error",
        QString("In StepManager: SliceVisualizer has no corresponding step"));
  }

  QMap<QString, QSharedPointer<SegmentationStep>> stepMap;
  stepMap["BrushStep"] = brushStep;
  stepMap["LassoStep"] = lassoStep;

  return stepMap;
}

void StepManager::setVolume(QSharedPointer<VolumeNode> const volumeNode) {
  disconnect(this->histogramConnection);
  auto volumeDataVoxel =
      qSharedPointerDynamicCast<VolumeDataVoxel>(volumeNode->volumeData());
  if (volumeDataVoxel) {
    this->volumeHistogramProvider = volumeDataVoxel->getHistogramProvider(
        HistogramProvider::DefaultBucketCount);

    this->histogramConnection =
        connect(volumeHistogramProvider.data(), &HistogramProvider::dataChanged,
                histogramProvider.data(), &HistogramProvider::setData);

    histogramProvider->setData(this->volumeHistogramProvider->getData());
  }
  Q_EMIT this->inputVolChanged(volumeNode.data());
}

QList<QSharedPointer<vx::SegmentationStep>> StepManager::spawnDefaultSteps(
    LabelViewModel* labelViewModel, const QItemSelectionModel* selectionModel) {
  auto nonSpecialSteps = QList<QSharedPointer<vx::SegmentationStep>>();

  auto componentsTyped =
      voxieRoot().components()->listComponentsTyped<NodePrototype>();

  for (auto nodePrototype : componentsTyped) {
    if (nodePrototype->nodeKind() == NodeKind::SegmentationStep) {
      if (!nodePrototype->rawJson()["IsSpecialStep"].toBool()) {
        // Get step
        auto step = qSharedPointerDynamicCast<SegmentationStep>(
            voxieRoot()
                .components()
                ->getComponentTyped<NodePrototype>(nodePrototype->name(), false)
                ->create({}, QList<Node*>(), QMap<QString, QDBusVariant>(),
                         false));

        step->setHistogramProvider(this->getHistogramProvider());

        connect(this, &StepManager::inputVolChanged, this, [=](Node* node) {
          Q_UNUSED(node);
          if (this->getHistogramProvider()) {
            volumeHistogramProvider = this->getHistogramProvider();
            step->setHistogramProvider(volumeHistogramProvider);
          }
        });

        step->fillPropertySection(labelViewModel);

        connect(this, &StepManager::resetUIWidgets, step.data(),
                [step]() { step->resetUIWidget(); });

        connect(this, &StepManager::voxelOpIsFinished, step.data(),
                &SegmentationStep::onVoxelOpFinished);

        // Add the run_step button to the widget
        if (!nodePrototype->rawJson()["StartsRunAutomatic"].toBool()) {
          connect(
              step->getRunStepButton(), &QPushButton::clicked, this,
              [this, step, nodePrototype, labelViewModel, selectionModel]() {
                for (auto property : nodePrototype->nodeProperties()) {
                  if (property->type() != vx::types::LabelListType()) {
                    continue;
                  }

                  // Write labels that are selected in the label table UI into
                  // the label list property
                  if (property->rawJson()["SetFromSelectedLabels"].toBool(
                          false)) {
                    QList<quint64> labelList;
                    for (int index = 0; index < labelViewModel->rowCount();
                         index++) {
                      if (selectionModel->isSelected(
                              selectionModel->model()->index(index, 0))) {
                        labelList.push_back(
                            labelViewModel->getLabelIDbyRowIdx(index));
                      }
                    }
                    step->setNodeProperty(property->name(),
                                          QVariant::fromValue(labelList));
                  }
                }
                this->runStepByKind(step);
              });
          step->getPropertySection()->addProperty(step->getRunStepButton());
        } else {
          connect(step.data(), &SegmentationStep::triggerCalculation, this,
                  [this, step]() { this->runStepByKind(step); });
        }
        nonSpecialSteps.append(step);
      }
    }
  }

  return nonSpecialSteps;
}

void StepManager::clearSelection() {
  if (!isDataAvailable()) {
    return;
  }
  Q_EMIT this->canApplyActiveSelection(false);
  Q_EMIT this->selectionIsFinished(false);

  auto voxelFunc =
      [](size_t& x, size_t& y, size_t& z,
         QSharedPointer<VolumeDataVoxelInst<SegmentationType>> labelData) {
        SegmentationType voxelVal =
            (SegmentationType)labelData->getVoxel(x, y, z);
        clearBit(voxelVal, segmentationShift);
        labelData->setVoxel(x, y, z, voxelVal);
      };

  auto op = OperationSimple::create();
  OperationRegistry::instance()->addOperation(op);
  connect(op.data(), &OperationSimple::finished, this,
          &StepManager::selectionIsFinished);

  // clear executionQueue and add current step with freezed parameters
  QMutableListIterator<QSharedPointer<SegmentationStep>> i(
      this->executionQueue);
  while (i.hasNext()) {
    QSharedPointer<SegmentationStep> loopStep = i.next();
    if (loopStep->getStepKind() == StepKind::SelectionStep) {
      loopStep->deleteLater();
      i.remove();
    }
  }
  auto runnable = functionalRunnable([op, voxelFunc, this]() {
    iterateAllLabelVolumeVoxels(voxelFunc, getOutputData(), op);
  });
  QThreadPool::globalInstance()->start(runnable);

  this->selectionStepList.clear();
  Q_EMIT(this->updateSelectedVoxelCount(0, false));
  return;
}
