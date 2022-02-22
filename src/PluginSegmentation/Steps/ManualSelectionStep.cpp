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

#include "ManualSelectionStep.hpp"

#include <PluginSegmentation/SegmentationUtils.hpp>

#include <Voxie/Data/VolumeNode.hpp>

using namespace vx;
using namespace vx::io;

ManualSelectionStep::ManualSelectionStep(const QList<qint64>& labelIds)
    : SegmentationStep("ManualSelectionStep", getPrototypeSingleton()),
      properties(new ManualSelectionStepProperties(this)) {
  this->properties->setLabelIds(labelIds);
}

ManualSelectionStep::~ManualSelectionStep() {}

ManualSelectionStep::ManualSelectionStep()
    : SegmentationStep("ManualSelectionStep", getPrototypeSingleton()),
      properties(new ManualSelectionStepProperties(this)) {}

QSharedPointer<OperationResult> ManualSelectionStep::calculate(
    QSharedPointer<ParameterCopy> parameterCopy,
    QSharedPointer<ContainerData> containerData, QDBusObjectPath inputVolume) {
  Q_UNUSED(inputVolume);
  return this->runThreaded(
      "ManualSelectionStep", [parameterCopy, containerData,
                              this](const QSharedPointer<Operation>& op) {
        ManualSelectionStepPropertiesCopy propertyCopy(
            parameterCopy->properties()[parameterCopy->mainNodePath()]);

        qint64 voxelCount = 0;

        QList<qint64> labelIdList = propertyCopy.labelIds();
        auto voxelFunc =
            [&labelIdList, &voxelCount](
                size_t& x, size_t& y, size_t& z,
                QSharedPointer<VolumeDataVoxelInst<SegmentationType>>
                    labelData) {
              SegmentationType voxelVal =
                  (SegmentationType)labelData->getVoxel(x, y, z);

              // if label id is choosen to be selected, set the selection bit
              clearBit(voxelVal, segmentationShift);
              if (labelIdList.contains(voxelVal)) {
                setBit(voxelVal, segmentationShift);
                voxelCount++;
                labelData->setVoxel(x, y, z, voxelVal);
              }
            };

        iterateAllLabelVolumeVoxels(voxelFunc, containerData, op, false);

        Q_EMIT(this->updateSelectedVoxelCount(voxelCount, false));
      });
}

QString ManualSelectionStep::getInfoString() {
  QString info = "Select label: ";
  QList<qint64> labelIdList = this->properties->labelIds();
  QList<qint64>::iterator i;
  for (i = labelIdList.begin(); i != --labelIdList.end(); ++i) {
    info.append(QString::number(*i));
    info.append(",");
  }
  info.append(QString::number(labelIdList.last()));
  return info;
}

void ManualSelectionStep::resetUIWidget() {}

void ManualSelectionStep::onVoxelOpFinished(bool status) { Q_UNUSED(status); }

bool ManualSelectionStep::isAllowedChild(NodeKind node) {
  return node == NodeKind::Data;
}

void ManualSelectionStep::setProperties(const QList<qint64>& labelIds) {
  this->properties->setLabelIds(labelIds);
}

bool ManualSelectionStep::isAllowedParent(NodeKind node) {
  return node == NodeKind::Data;
}
bool ManualSelectionStep::isCreatableChild(NodeKind) { return false; }

QList<QString> ManualSelectionStep::supportedDBusInterfaces() { return {}; }

void ManualSelectionStep::initializeCustomUIPropSections() {}

NODE_PROTOTYPE_IMPL(ManualSelectionStep)
