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

#include "ThresholdSelectionStep.hpp"

#include <VoxieBackend/Data/VolumeDataVoxelInst.hpp>

#include <Voxie/Data/VolumeNode.hpp>

using namespace vx;
using namespace vx::io;

ThresholdSelectionStep::ThresholdSelectionStep(double lowerThreshold,
                                               double upperThreshold,
                                               VolumeNode& volumeIn)
    : SegmentationStep("ThresholdSelectionStep", getPrototypeSingleton()),
      properties(new ThresholdSelectionStepProperties(this)) {
  setProperties(lowerThreshold, upperThreshold, &volumeIn);
}

ThresholdSelectionStep::~ThresholdSelectionStep() {}

ThresholdSelectionStep::ThresholdSelectionStep()
    : SegmentationStep("ThresholdSelectionStep", getPrototypeSingleton()),
      properties(new ThresholdSelectionStepProperties(this)) {
  this->timeOutTimer = new QTimer(this);
  this->timeOutTimer->setSingleShot(true);
}

QSharedPointer<OperationResult> ThresholdSelectionStep::calculate(
    QSharedPointer<ParameterCopy> parameterCopy,
    QSharedPointer<ContainerData> containerData, QDBusObjectPath inputVolume) {
  return this->runThreaded(
      "ThresholdSelectionStep", [parameterCopy, containerData, inputVolume,
                                 this](const QSharedPointer<Operation>& op) {
        ThresholdSelectionStepPropertiesCopy propertyCopy(
            parameterCopy->properties()[parameterCopy->mainNodePath()]);

        auto originalVolume = QSharedPointer<VolumeDataVoxel>();

        if (propertyCopy.volume()) {
          originalVolume = qSharedPointerDynamicCast<VolumeDataVoxel>(
              dynamic_cast<VolumeNode*>(propertyCopy.volume())->data());

        }  // no volume in properties --> take inputVolume of Segmentation
        else {
          originalVolume = qSharedPointerDynamicCast<VolumeDataVoxel>(
              dynamic_cast<VolumeNode*>(
                  vx::PropertyValueConvertRaw<QDBusObjectPath,
                                              vx::Node*>::fromRaw(inputVolume))
                  ->data());
        }

        const double lower = propertyCopy.lowerThreshold();
        const double upper = propertyCopy.upperThreshold();

        // cast output to VolumeDataVoxelInst SegmentationType as we know its
        // type
        auto labelVolume =
            qSharedPointerDynamicCast<VolumeDataVoxelInst<SegmentationType>>(
                containerData->getElement("labelVolume"));

        auto outerUpdate = containerData->createUpdate();
        auto update = labelVolume->createUpdate(
            {{containerData->getPath(), outerUpdate}});
        qint64 voxelCount = 0;
        const vx::VectorSizeT3& inputDimensions =
            originalVolume->getDimensions();
        // cast VolumeVoxelData to VolumeVoxelDataInst inside lambda function
        originalVolume->performInGenericContext(
            [lower, upper, inputDimensions, labelVolume, op,
             &voxelCount](auto& originalVolumeInst) {
              for (size_t x = 0; x < inputDimensions.x; x++) {
                for (size_t y = 0; y < inputDimensions.y; y++) {
                  op->throwIfCancelled();
                  for (size_t z = 0; z < inputDimensions.z; z++) {
                    auto voxelValIn = originalVolumeInst.getVoxel(x, y, z);
                    SegmentationType voxelVal =
                        (SegmentationType)labelVolume->getVoxel(x, y, z);
                    if ((voxelValIn > lower) && (voxelValIn < upper)) {
                      // set label volume bits to mark value inside thresholds
                      setBit(voxelVal, segmentationShift);
                      voxelCount++;

                    } else {
                      // override old selections
                      clearBit(voxelVal, segmentationShift);
                    }
                    labelVolume->setVoxel(x, y, z, voxelVal);
                  }
                }
                op->updateProgress(1.0f * x / inputDimensions.x);
              }
            });
        update->finish({});
        outerUpdate->finish({});
        Q_EMIT(this->updateSelectedVoxelCount(voxelCount, false));
      });
}

QString ThresholdSelectionStep::getInfoString() {
  return QString("Threshold selection between value %1 and %2")
      .arg(QString::number(properties->lowerThreshold()),
           QString::number(properties->upperThreshold()));
}

bool ThresholdSelectionStep::isAllowedChild(NodeKind node) {
  Q_UNUSED(node);
  return false;
}

void ThresholdSelectionStep::setProperties(double& lowerThresh,
                                           double& upperThresh,
                                           VolumeNode* volumeIn) {
  this->properties->setLowerThreshold(lowerThresh);
  this->properties->setUpperThreshold(upperThresh);
  this->properties->setVolume(volumeIn);
}

bool ThresholdSelectionStep::isAllowedParent(NodeKind node) {
  return node == NodeKind::Data;
}
bool ThresholdSelectionStep::isCreatableChild(NodeKind) { return false; }

QList<QString> ThresholdSelectionStep::supportedDBusInterfaces() { return {}; }

void ThresholdSelectionStep::resetUIWidget() {}

void ThresholdSelectionStep::onVoxelOpFinished(bool status) {
  Q_UNUSED(status);
  // TODO: Nothing to do?
}

void ThresholdSelectionStep::initializeCustomUIPropSections() {
  // Threshold-Selection
  this->thresholdSelection = new ThresholdSelectionWidget(this->labelViewModel);
  this->thresholdSelection->init();
  this->thresholdSelection->getColorizerWidget()->setHistogramProvider(
      this->histogramProvider);

  connect(this->timeOutTimer, &QTimer::timeout, this,
          &ThresholdSelectionStep::onThresBoxChange);

  connect(
      this, &SegmentationStep::histogramProviderChanged, this,
      [=](QSharedPointer<vx::HistogramProvider> histProvider) {
        this->thresholdSelection->getColorizerWidget()->setHistogramProvider(
            histProvider);
      });

  connect(this->thresholdSelection->getColorizerWidget(),
          &ColorizerGradientWidget::entryValueChanged, this,
          [this]() { this->timeOutTimer->start(this->timeOutVal); });
}

void ThresholdSelectionStep::onThresBoxChange() {
  double lowerThresh = this->thresholdSelection->getLeftBox()->value();
  double upperThresh = this->thresholdSelection->getRightBox()->value();

  // Make sure that min is never smaller than max & vice versa
  this->thresholdSelection->getLeftBox()->setMaximum(upperThresh);
  this->thresholdSelection->getRightBox()->setMinimum(lowerThresh);

  this->properties->setLowerThreshold(lowerThresh);
  this->properties->setUpperThreshold(upperThresh);

  Q_EMIT this->triggerCalculation();
}

QWidget* ThresholdSelectionStep::getCustomPropertySectionContent(
    const QString& name) {
  this->initializeCustomUIPropSections();
  if (name ==
      "de.uni_stuttgart.Voxie.SegmentationStep.ThresholdSelectionStep."
      "ThresholdWidget") {
    return this->thresholdSelection;
  } else {
    return Node::getCustomPropertySectionContent(name);
  }
}

NODE_PROTOTYPE_IMPL(ThresholdSelectionStep)
