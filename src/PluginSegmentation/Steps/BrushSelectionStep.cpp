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

#include "BrushSelectionStep.hpp"
#include <VoxieBackend/Data/VolumeDataVoxelInst.hpp>

VX_NODE_INSTANTIATION(vx::BrushSelectionStep)

// TODO: Currently multiple brush operations with Select and Erase will be
// combined into a single BrushSelectionStep, but this will generate a different
// result when re-running the step because it does not honor the order of the
// Select and Erase operations.

// TODO: Also, doing (brush) selection operations without doing an assignment
// step afterwards, then re-running the filter, then doing other selection
// operations and an assignment step seems to do the wrong thing.

using namespace vx;
using namespace vx::io;

BrushSelectionStep::BrushSelectionStep(
    QList<std::tuple<vx::Vector<double, 3>, double>> selectCentersWithRadiuses,
    QList<std::tuple<vx::Vector<double, 3>, double>> eraseCentersWithRadiuses,
    QVector3D volumeOrigin, QQuaternion volumeOrientation, QVector3D voxelSize,
    PlaneInfo plane)
    : SegmentationStep("BrushSelectionStep", getPrototypeSingleton()),
      properties(new BrushSelectionStepProperties(this)) {
  this->setProperties(selectCentersWithRadiuses, eraseCentersWithRadiuses,
                      volumeOrigin, volumeOrientation, voxelSize, plane.origin,
                      plane.rotation);
}

BrushSelectionStep::BrushSelectionStep(const BrushSelectionStep& oldStep)
    : SegmentationStep("BrushSelectionStep", getPrototypeSingleton()),
      properties(new BrushSelectionStepProperties(this)) {
  this->setProperties(oldStep.properties->brushSelectCentersWithRadius(),
                      oldStep.properties->brushEraseCentersWithRadius(),
                      oldStep.properties->volumeOrigin(),
                      oldStep.properties->volumeOrientation(),
                      oldStep.properties->voxelSize(),
                      oldStep.properties->planeOrigin(),
                      oldStep.properties->planeOrientation());
}

BrushSelectionStep::BrushSelectionStep()
    : SegmentationStep("BrushSelectionStep", getPrototypeSingleton()),
      properties(new BrushSelectionStepProperties(this)) {}

BrushSelectionStep::~BrushSelectionStep() {}

QSharedPointer<OperationResult> BrushSelectionStep::calculate(
    QSharedPointer<ParameterCopy> parameterCopy,
    QSharedPointer<ContainerData> containerData, QDBusObjectPath inputVolume) {
  Q_UNUSED(inputVolume);

  return this->runThreaded("BrushSelectionStep", [parameterCopy, containerData,
                                                  this](const QSharedPointer<
                                                        Operation>& op) {
    BrushSelectionStepPropertiesCopy propertyCopy(
        parameterCopy->properties()[parameterCopy->mainNodePath()]);

    auto compoundVoxelData = qSharedPointerDynamicCast<VolumeDataVoxel>(
        containerData->getElement("labelVolume"));

    if (qFuzzyCompare(compoundVoxelData->origin(),
                      propertyCopy.volumeOrigin()) &&
        qFuzzyCompare(compoundVoxelData->getSpacing(),
                      propertyCopy.voxelSize())) {
      qint64 voxelCount = 0;

      QList<voxel_index> voxelsToSelect;
      QList<voxel_index> voxelsToClear;

      int numberOfCenters = propertyCopy.brushSelectCentersWithRadius().size() +
                            propertyCopy.brushEraseCentersWithRadius().size();

      int numberOfCalculatedCenters = 0;

      for (auto& entry : propertyCopy.brushSelectCentersWithRadius()) {
        auto calculator = BrushCalculator(
            // TODO: Avoid QVector3D
            toQVector(vectorCastNarrow<float>(std::get<0>(entry))),
            std::get<1>(entry), compoundVoxelData, propertyCopy.planeOrigin(),
            propertyCopy.planeOrientation());
        calculator.growRegion();
        voxelsToSelect.append(calculator.getFoundVoxels());
        numberOfCalculatedCenters += 1;
        op->updateProgress(1.0f * numberOfCalculatedCenters / numberOfCenters);
      }

      auto selectFunc =
          [&voxelCount](
              quint32& x, quint32& y, quint32& z,
              const QSharedPointer<VolumeDataVoxelInst<SegmentationType>>&
                  labelData) {
            SegmentationType voxelVal =
                (SegmentationType)labelData->getVoxel(x, y, z);
            if (getBit(voxelVal, segmentationShift) == 0) {
              voxelCount++;
              setBit(voxelVal, segmentationShift);
              labelData->setVoxel(x, y, z, voxelVal);
            }
          };

      iterateAllPassedLabelVolumeVoxels(selectFunc, voxelsToSelect,
                                        containerData, op, false);

      for (auto& entry : propertyCopy.brushEraseCentersWithRadius()) {
        auto calculator = BrushCalculator(
            // TODO: Avoid QVector3D
            toQVector(vectorCastNarrow<float>(std::get<0>(entry))),
            std::get<1>(entry), compoundVoxelData, propertyCopy.planeOrigin(),
            propertyCopy.planeOrientation());
        calculator.growRegion();
        voxelsToClear.append(calculator.getFoundVoxels());
        numberOfCalculatedCenters += 1;
        op->updateProgress(1.0f * numberOfCalculatedCenters / numberOfCenters);
      }

      auto clearFunc =
          [&voxelCount](
              quint32& x, quint32& y, quint32& z,
              const QSharedPointer<VolumeDataVoxelInst<SegmentationType>>&
                  labelData) {
            SegmentationType voxelVal =
                (SegmentationType)labelData->getVoxel(x, y, z);
            if (getBit(voxelVal, segmentationShift) != 0) {
              voxelCount--;
              clearBit(voxelVal, segmentationShift);
              labelData->setVoxel(x, y, z, voxelVal);
            }
          };

      iterateAllPassedLabelVolumeVoxels(clearFunc, voxelsToClear, containerData,
                                        op, false);
      Q_EMIT(this->updateSelectedVoxelCount(voxelCount, true));
    } else {
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.InvalidOperation",
          QString("The orientation, the origin and the voxel size of the step "
                  "and the labelVolume should match"));
    }
  });
}

QSharedPointer<vx::io::Operation> BrushSelectionStep::selectPassedVoxels(
    std::tuple<vx::Vector<double, 3>, double> centerWithRadius,
    QSharedPointer<ParameterCopy> parameterCopy,
    QSharedPointer<ContainerData> containerData,
    QSharedPointer<vx::io::Operation> op) {
  Q_UNUSED(parameterCopy);
  auto compoundVoxelData = qSharedPointerDynamicCast<VolumeDataVoxel>(
      containerData->getElement("labelVolume"));

  qint64 voxelCount = 0;

  auto voxelFunc =
      [&voxelCount](quint32& x, quint32& y, quint32& z,
                    const QSharedPointer<VolumeDataVoxelInst<SegmentationType>>&
                        labelData) {
        SegmentationType voxelVal =
            (SegmentationType)labelData->getVoxel(x, y, z);
        if (getBit(voxelVal, segmentationShift) == 0) {
          voxelCount++;
          setBit(voxelVal, segmentationShift);
          labelData->setVoxel(x, y, z, voxelVal);
        }
      };

  auto calculator = BrushCalculator(
      // TODO: Avoid QVector3D
      toQVector(vectorCastNarrow<float>(std::get<0>(centerWithRadius))),
      std::get<1>(centerWithRadius), compoundVoxelData,
      this->properties->planeOrigin(), this->properties->planeOrientation());
  calculator.growRegion();
  auto voxelsToSelect = calculator.getFoundVoxels();

  iterateAllPassedLabelVolumeVoxels(voxelFunc, voxelsToSelect, containerData,
                                    op, false);

  this->addSelectCenterWithRadius(centerWithRadius);
  Q_EMIT(this->updateSelectedVoxelCount(voxelCount, true));
  return op;
}

QSharedPointer<vx::io::Operation> BrushSelectionStep::erasePassedVoxels(
    std::tuple<vx::Vector<double, 3>, double> centerWithRadius,
    QSharedPointer<ParameterCopy> parameterCopy,
    QSharedPointer<ContainerData> containerData,
    QSharedPointer<vx::io::Operation> op) {
  Q_UNUSED(parameterCopy);
  auto compoundVoxelData = qSharedPointerDynamicCast<VolumeDataVoxel>(
      containerData->getElement("labelVolume"));

  qint64 voxelCount = 0;

  auto voxelFunc =
      [&voxelCount](quint32& x, quint32& y, quint32& z,
                    const QSharedPointer<VolumeDataVoxelInst<SegmentationType>>&
                        labelData) {
        SegmentationType voxelVal =
            (SegmentationType)labelData->getVoxel(x, y, z);
        if (getBit(voxelVal, segmentationShift) != 0) {
          voxelCount--;
          clearBit(voxelVal, segmentationShift);
          labelData->setVoxel(x, y, z, voxelVal);
        }
      };

  auto calculator = BrushCalculator(
      // TODO: Avoid QVector3D
      toQVector(vectorCastNarrow<float>(std::get<0>(centerWithRadius))),
      std::get<1>(centerWithRadius), compoundVoxelData,
      this->properties->planeOrigin(), this->properties->planeOrientation());
  calculator.growRegion();
  auto voxelsToErase = calculator.getFoundVoxels();

  iterateAllPassedLabelVolumeVoxels(voxelFunc, voxelsToErase, containerData, op,
                                    false);

  this->addEraseCenterWithRadius(centerWithRadius);
  Q_EMIT(this->updateSelectedVoxelCount(voxelCount, true));
  return op;
}

void BrushSelectionStep::resetUIWidget() {}

void BrushSelectionStep::onVoxelOpFinished(bool status) { Q_UNUSED(status); }

void BrushSelectionStep::addEraseCenterWithRadius(
    std::tuple<vx::Vector<double, 3>, double> centerWithRadius) {
  auto oldCenters = this->properties->brushEraseCentersWithRadius();
  oldCenters.append(centerWithRadius);
  this->properties->setBrushEraseCentersWithRadius(oldCenters);
}

void BrushSelectionStep::addSelectCenterWithRadius(
    std::tuple<vx::Vector<double, 3>, double> centerWithRadius) {
  auto oldCenters = this->properties->brushSelectCentersWithRadius();
  oldCenters.append(centerWithRadius);
  this->properties->setBrushSelectCentersWithRadius(oldCenters);
}

void BrushSelectionStep::setPlaneProperties(PlaneInfo plane) {
  this->properties->setPlaneOrigin(plane.origin);
  this->properties->setPlaneOrientation(plane.rotation);
}

QString BrushSelectionStep::getInfoString() {
  return QString("Brush Selection");
}

bool BrushSelectionStep::isAllowedChild(NodeKind object) {
  Q_UNUSED(object);
  return false;
}

void BrushSelectionStep::setProperties(
    QList<std::tuple<vx::Vector<double, 3>, double>> selectCentersWithRadiuses,
    QList<std::tuple<vx::Vector<double, 3>, double>> eraseCentersWithRadiuses,
    QVector3D volumeOrigin, QQuaternion volumeOrientation, QVector3D voxelSize,
    QVector3D planeOrigin, QQuaternion planeOrientation) {
  this->properties->setVolumeOrientation(volumeOrientation);
  this->properties->setVolumeOrigin(volumeOrigin);
  this->properties->setBrushSelectCentersWithRadius(selectCentersWithRadiuses);
  this->properties->setBrushEraseCentersWithRadius(eraseCentersWithRadiuses);
  this->properties->setVoxelSize(voxelSize);
  this->properties->setPlaneOrigin(planeOrigin);
  this->properties->setPlaneOrientation(planeOrientation);
}

bool BrushSelectionStep::isAllowedParent(NodeKind object) {
  return object == NodeKind::Data;
}
bool BrushSelectionStep::isCreatableChild(NodeKind) { return false; }

QList<QString> BrushSelectionStep::supportedDBusInterfaces() { return {}; }

void BrushSelectionStep::initializeCustomUIPropSections() {}

BrushCalculator::BrushCalculator(QVector3D brushCenter, double brushRadius,
                                 QSharedPointer<VolumeDataVoxel> originalVolume,
                                 QVector3D planeOrigin,
                                 QQuaternion planeOrientation)
    : IndexCalculator(originalVolume, planeOrigin, planeOrientation) {
  this->brushCenter = brushCenter;
  this->brushRadiusSquared = pow(brushRadius, 2);

  this->brushCenterOnPlane =
      QPointF((this->threeDToPlaneTrafo * brushCenter).x(),
              (this->threeDToPlaneTrafo * brushCenter).y());

  QVector3D voxelCoord =
      this->PlaneToVoxelTrafo *
      QVector3D(brushCenterOnPlane.x(), brushCenterOnPlane.y(), 0);

  this->visitNext.append(voxel_index((quint32)std::floor(voxelCoord.x()),
                                     (quint32)std::floor(voxelCoord.y()),
                                     (quint32)std::floor(voxelCoord.z())));
}

bool inline BrushCalculator::isVoxelInsideBrushRadius(
    voxel_index currentVoxel) {
  QVector4D voxelCenter = QVector4D(std::get<0>(currentVoxel) + 0.5,
                                    std::get<1>(currentVoxel) + 0.5,
                                    std::get<2>(currentVoxel) + 0.5, 1);
  auto planePoint = QPointF(
      QVector4D::dotProduct(this->voxelToPlaneXProjection, voxelCenter),
      QVector4D::dotProduct(this->voxelToPlaneYProjection, voxelCenter));

  // check if distance in [m] to middle point is smaller than certain radius
  // [m]
  if (pow(this->brushCenterOnPlane.x() - planePoint.x(), 2) +
          pow(this->brushCenterOnPlane.y() - planePoint.y(), 2) <=
      this->brushRadiusSquared) {
    return true;
  } else {
    return false;
  }
}

bool BrushCalculator::areCriteriaMet(voxel_index& voxelIndex) {
  if (isVoxelInsideBrushRadius(voxelIndex) &&
      doesVoxelIntersectWithPlane(voxelIndex)) {
    return true;
  } else {
    return false;
  }
}
