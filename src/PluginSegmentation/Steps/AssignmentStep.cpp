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

#include "AssignmentStep.hpp"
#include <PluginSegmentation/SegmentationUtils.hpp>
#include <VoxieBackend/IO/Operation.hpp>
#include <VoxieBackend/IO/OperationRegistry.hpp>

VX_NODE_INSTANTIATION(vx::AssignmentStep)

using namespace vx;
using namespace vx::io;

AssignmentStep::AssignmentStep(qlonglong labelID)
    : SegmentationStep("AssignmentStep", getPrototypeSingleton()),
      properties(new AssignmentStepProperties(this)) {
  this->properties->setLabelID(labelID);
}

AssignmentStep::AssignmentStep()
    : SegmentationStep("AssignmentStep", getPrototypeSingleton()),
      properties(new AssignmentStepProperties(this)) {}

AssignmentStep::~AssignmentStep() {}

QSharedPointer<OperationResult> AssignmentStep::calculate(
    QSharedPointer<ParameterCopy> parameterCopy,
    QSharedPointer<ContainerData> containerData, QDBusObjectPath inputVolume) {
  Q_UNUSED(inputVolume);
  return this->runThreaded(
      "AssignmentStep", [parameterCopy, containerData,
                         this](const QSharedPointer<Operation>& op) {
        AssignmentStepPropertiesCopy propertyCopy(
            parameterCopy->properties()[parameterCopy->mainNodePath()]);

        int labelID = propertyCopy.labelID();

        if (!(labelID == -1)) {
          this->addSelection(containerData, labelID, op);

        } else {
          throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                              QString("No LabelID provided in assignmentStep"));
        }
      });
}

QString AssignmentStep::getInfoString() {
  return QString("Assign voxels to label %1")
      .arg(QString::number(properties->labelID()));
}

void AssignmentStep::addSelection(QSharedPointer<ContainerData> containerData,
                                  int labelID,
                                  QSharedPointer<vx::io::Operation> op) {
  auto labelTable = qSharedPointerDynamicCast<TableData>(
      containerData->getElement("labelTable"));

  // check that label index does not exceed maximum value / 2
  if (labelID >= (std::numeric_limits<SegmentationType>::max() >> 1)) {
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        QString("AssignmentStep::addSelection(): labelID "
                                "exceeds available data range"));
  }

  QMutex changeTrackerOverallMutex;
  LabelChangeTracker changeTrackerOverall;

  auto outerUpdate = containerData->createUpdate();

  auto task = Task::create();
  forwardProgressFromTaskToOperation(task.data(), op.data());

  iterateAllLabelVolumeVoxels(
      containerData, outerUpdate, task.data(), op.data(),
      [&labelID, &changeTrackerOverallMutex,
       &changeTrackerOverall](const auto& cb) {
        LabelChangeTracker changeTrackerThread;

        cb([&](size_t& x, size_t& y, size_t& z,
               const QSharedPointer<VolumeDataVoxelInst<SegmentationType>>&
                   labelData) {
          SegmentationType voxelValue = labelData->getVoxel(x, y, z);

          if (!getBit((SegmentationType)voxelValue, segmentationShift)) return;

          changeTrackerThread.set(labelData, x, y, z, labelID, false);
        });

        {
          QMutexLocker locker(&changeTrackerOverallMutex);
          changeTrackerOverall.mergeChangesFrom(changeTrackerThread);
        }
      });

  // TODO: Reuse outerUpdate
  updateStatistics(containerData, changeTrackerOverall);

  // TODO: Use changeTrackerOverall->getSelectedChange() / check that the result
  // is 0?
  // qDebug() << "getSelectedChange()" <<
  // changeTrackerOverall.getSelectedChange();

  // TODO: Reuse outerUpdate
  Q_EMIT(this->updateSelectedVoxelCount((qint64)0, false));

  outerUpdate->finish({});
}

void AssignmentStep::resetUIWidget() {}

void AssignmentStep::onVoxelOpFinished(bool status) { Q_UNUSED(status); }

bool AssignmentStep::isAllowedChild(NodeKind node) {
  Q_UNUSED(node);
  return false;
}

bool AssignmentStep::isAllowedParent(NodeKind node) {
  return node == NodeKind::Data;
}
bool AssignmentStep::isCreatableChild(NodeKind) { return false; }

QList<QString> AssignmentStep::supportedDBusInterfaces() { return {}; }

void AssignmentStep::initializeCustomUIPropSections() {}
