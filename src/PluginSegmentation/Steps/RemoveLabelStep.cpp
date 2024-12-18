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

#include "RemoveLabelStep.hpp"
#include <VoxieBackend/IO/OperationRegistry.hpp>

VX_NODE_INSTANTIATION(vx::RemoveLabelStep)

using namespace vx;
using namespace vx::io;

RemoveLabelStep::RemoveLabelStep()
    : SegmentationStep("RemoveLabelStep", getPrototypeSingleton()),
      properties(new RemoveLabelStepProperties(this)) {}

RemoveLabelStep::~RemoveLabelStep() {}

QSharedPointer<OperationResult> RemoveLabelStep::calculate(
    QSharedPointer<ParameterCopy> parameterCopy,
    QSharedPointer<ContainerData> containerData, QDBusObjectPath inputVolume) {
  Q_UNUSED(inputVolume);

  return this->runThreaded(
      "RemoveLabelStep",
      [parameterCopy, containerData](const QSharedPointer<Operation>& op) {
        RemoveLabelStepPropertiesCopy propertyCopy(
            parameterCopy->properties()[parameterCopy->mainNodePath()]);

        auto labelTable = qSharedPointerDynamicCast<TableData>(
            containerData->getElement("labelTable"));

        QList<qlonglong> labelIDs64 = propertyCopy.labelIDs();
        QList<SegmentationType> labelIDs;

        // check for each label index that it does not exceed maximum value / 2
        for (qlonglong labelID64 : labelIDs64) {
          if (labelID64 > (std::numeric_limits<SegmentationType>::max() >> 1) ||
              labelID64 < 0) {
            throw vx::Exception(
                "de.uni_stuttgart.Voxie.Error",
                QString("RemoveLabelStep::calculate(): labelId: %1 exceeds "
                        "available data range")
                    .arg(QString::number(labelID64)));
          } else {
            labelIDs.append((SegmentationType)labelID64);
          }
        }

        // TODO: Track changes? Might not be needed because the label is deleted
        // anyway. Might still be useful to detect when the statistics were
        // wrong.
        QMutex changeTrackerOverallMutex;
        LabelChangeTracker changeTrackerOverall;

        auto outerUpdate = containerData->createUpdate();

        auto task = Task::create();
        forwardProgressFromTaskToOperation(task.data(), op.data());

        iterateAllLabelVolumeVoxels(
            containerData, outerUpdate, task.data(), op.data(),
            [&labelIDs, &changeTrackerOverallMutex,
             &changeTrackerOverall](const auto& cb) {
              LabelChangeTracker changeTrackerThread;

              cb([&](size_t& x, size_t& y, size_t& z,
                     const QSharedPointer<
                         VolumeDataVoxelInst<SegmentationType>>& labelData) {
                SegmentationType voxelVal =
                    (SegmentationType)labelData->getVoxel(x, y, z);
                auto isSelected = getBit(voxelVal, segmentationShift);

                clearBit(voxelVal, segmentationShift);

                if (!labelIDs.contains(voxelVal)) return;

                // Set to 0, but keep selection state
                changeTrackerThread.set(labelData, x, y, z, 0, isSelected);
              });

              {
                QMutexLocker locker(&changeTrackerOverallMutex);
                changeTrackerOverall.mergeChangesFrom(changeTrackerThread);
              }
            });

        // Remove rows from labelTable
        for (auto labelID : labelIDs) {
          TableRow row = getTableRowByLabelID(labelID, labelTable);
          auto update = labelTable->createUpdate(
              {{containerData->getPath(), outerUpdate}});

          labelTable->removeRow(update, row.rowID());

          update->finish({});
        }
        outerUpdate->finish({});
      });
}

void RemoveLabelStep::setProperties(QList<qlonglong> labelIDs) {
  this->properties->setLabelIDs(labelIDs);
}

QString RemoveLabelStep::getInfoString() {
  QList<qint64> list = properties->labelIDs();
  QString labelString;

  for (int i = 0; i < properties->labelIDs().size(); i++) {
    labelString += QString::number(list[i]);
    if (i < list.size() - 1) labelString += ",";
  }

  return QString("Remove label: %1").arg(labelString);
}

void RemoveLabelStep::resetUIWidget() {}

void RemoveLabelStep::onVoxelOpFinished(bool status) { Q_UNUSED(status); }

bool RemoveLabelStep::isAllowedChild(NodeKind node) {
  Q_UNUSED(node);
  return false;
}

bool RemoveLabelStep::isAllowedParent(NodeKind node) {
  return node == NodeKind::Data;
}
bool RemoveLabelStep::isCreatableChild(NodeKind) { return false; }

QList<QString> RemoveLabelStep::supportedDBusInterfaces() { return {}; }

void RemoveLabelStep::initializeCustomUIPropSections() {}
