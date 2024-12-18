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

#include "SubtractStep.hpp"

#include <PluginSegmentation/SegmentationUtils.hpp>

#include <VoxieBackend/IO/Operation.hpp>
#include <VoxieBackend/IO/OperationRegistry.hpp>

VX_NODE_INSTANTIATION(vx::SubtractStep)

// TODO: Make "LabelID" vs. "LabelId" consistent between SubtractStep,
// AssignmentStep etc.

using namespace vx;
using namespace vx::io;

SubtractStep::SubtractStep(qlonglong labelId)
    : SegmentationStep("SubtractStep", getPrototypeSingleton()),
      properties(new SubtractStepProperties(this)) {
  this->properties->setLabelId(labelId);
}

SubtractStep::SubtractStep()
    : SegmentationStep("SubtractStep", getPrototypeSingleton()),
      properties(new SubtractStepProperties(this)) {}

SubtractStep::~SubtractStep() {}

QSharedPointer<OperationResult> SubtractStep::calculate(
    QSharedPointer<ParameterCopy> parameterCopy,
    QSharedPointer<ContainerData> containerData, QDBusObjectPath inputVolume) {
  Q_UNUSED(inputVolume);

  return this->runThreaded(
      "SubtractStep",
      [parameterCopy, containerData](const QSharedPointer<Operation>& op) {
        SubtractStepPropertiesCopy propertyCopy(
            parameterCopy->properties()[parameterCopy->mainNodePath()]);

        auto labelId = propertyCopy.labelId();

        if (labelId == -1) {
          throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                              QString("No LabelID provided in SubtractStep"));
        }

        auto labelTable = qSharedPointerDynamicCast<TableData>(
            containerData->getElement("labelTable"));

        QMutex changeTrackerOverallMutex;
        LabelChangeTracker changeTrackerOverall;

        auto outerUpdate = containerData->createUpdate();

        auto task = Task::create();
        forwardProgressFromTaskToOperation(task.data(), op.data());

        iterateAllLabelVolumeVoxels(
            containerData, outerUpdate, task.data(), op.data(),
            [&labelId, &changeTrackerOverallMutex,
             &changeTrackerOverall](const auto& cb) {
              LabelChangeTracker changeTrackerThread;

              cb([&](size_t& x, size_t& y, size_t& z,
                     const QSharedPointer<
                         VolumeDataVoxelInst<SegmentationType>>& labelData) {
                SegmentationType voxelValue = labelData->getVoxel(x, y, z);

                if (!getBit((SegmentationType)voxelValue, segmentationShift))
                  return;

                clearBit(voxelValue, segmentationShift);

                if (voxelValue == labelId) voxelValue = 0;

                // Clear selection and possible change label
                changeTrackerThread.set(labelData, x, y, z, voxelValue, false);
              });

              {
                QMutexLocker locker(&changeTrackerOverallMutex);
                changeTrackerOverall.mergeChangesFrom(changeTrackerThread);
              }
            });

        // TODO: Reuse outerUpdate
        updateStatistics(containerData, changeTrackerOverall);

        // TODO: Doesn't this also have to update SelectedVoxelCount?
        // Currently this is done in StepManager::createSubtractStep(), but this
        // will not work when the filter is re-run
        // See also AssignmentStep.cpp

        outerUpdate->finish({});
      });
}

void SubtractStep::resetUIWidget() {}

void SubtractStep::onVoxelOpFinished(bool status) { Q_UNUSED(status); }

QString SubtractStep::getInfoString() {
  return QString("Remove selection from label %1")
      .arg(QString::number(properties->labelId()));
}

bool SubtractStep::isAllowedChild(NodeKind object) {
  Q_UNUSED(object);
  return false;
}

bool SubtractStep::isAllowedParent(NodeKind object) {
  return object == NodeKind::Data;
}
bool SubtractStep::isCreatableChild(NodeKind) { return false; }

QList<QString> SubtractStep::supportedDBusInterfaces() { return {}; }

void SubtractStep::initializeCustomUIPropSections() {}
