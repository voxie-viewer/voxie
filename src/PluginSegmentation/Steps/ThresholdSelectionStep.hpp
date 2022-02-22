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
#include <PluginSegmentation/Prototypes.forward.hpp>
#include <PluginSegmentation/Prototypes.hpp>
#include <PluginSegmentation/SegmentationUtils.hpp>
#include <QTimer>
#include <QtCore/QSharedPointer>
#include <Voxie/Data/ContainerData.hpp>
#include <Voxie/Data/TableData.hpp>
#include <Voxie/IO/RunFilterOperation.hpp>
#include <Voxie/Node/SegmentationStep.hpp>
#include <VoxieBackend/Data/Data.hpp>
#include <VoxieBackend/Data/VolumeDataVoxel.hpp>
#include <VoxieClient/Array.hpp>
#include "PluginSegmentation/Gui/ThresholdSelectionWidget.hpp"

namespace vx {
class VolumeNode;

/*! ThresholdSelectionStep
 * SegmentationStep for selecting voxels between given lower and
 * upper theshold range. Iterates all voxels in the input volume and sets the
 * corresponding voxel selection bit in the labelVolume if threshold conditions
 * are met.
 */
class ThresholdSelectionStep : public SegmentationStep {
  NODE_PROTOTYPE_DECL(ThresholdSelectionStep)
 private:
  ThresholdSelectionWidget* thresholdSelection;

  void initializeCustomUIPropSections() override;

  void onThresBoxChange();

  QTimer* timeOutTimer;
  static constexpr uint16_t timeOutVal =
      150;  // [ms] count down value of single shot timer

 public:
  QWidget* getCustomPropertySectionContent(const QString& name) override;

  ThresholdSelectionStep(double lowerThreshold, double upperThreshold,
                         VolumeNode& volumeIn);
  ThresholdSelectionStep();
  ~ThresholdSelectionStep();

  QSharedPointer<OperationResult> calculate(
      QSharedPointer<ParameterCopy> parameterCopy,
      QSharedPointer<ContainerData> containerData,
      QDBusObjectPath inputVolume) override;

  QString getInfoString() override;

  void setProperties(double& lowerThresh, double& upperThresh,
                     VolumeNode* volumeIn);
  ThresholdSelectionStepProperties* getProperties() {
    return this->properties;
  };

  void resetUIWidget() override;

  void onVoxelOpFinished(bool status) override;

  bool isAllowedChild(NodeKind node) override;
  bool isAllowedParent(NodeKind node) override;
  bool isCreatableChild(NodeKind node) override;
  QList<QString> supportedDBusInterfaces() override;
};
}  // namespace vx
