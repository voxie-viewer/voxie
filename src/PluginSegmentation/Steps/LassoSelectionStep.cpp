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

#include "LassoSelectionStep.hpp"

using namespace vx;
using namespace vx::io;

LassoSelectionStep::LassoSelectionStep(QList<QVector3D> nodes,
                                       QVector3D volumeOrigin,
                                       QQuaternion volumeOrientation,
                                       QVector3D voxelSize, PlaneInfo plane)
    : SegmentationStep("LassoSelectionStep", getPrototypeSingleton()),
      properties(new LassoSelectionStepProperties(this)) {
  this->setProperties(nodes, volumeOrigin, volumeOrientation, voxelSize,
                      plane.origin, plane.rotation);
}

LassoSelectionStep::LassoSelectionStep(const LassoSelectionStep& oldStep)
    : SegmentationStep("LassoSelectionStep", getPrototypeSingleton()),
      properties(new LassoSelectionStepProperties(this)) {
  this->setProperties(
      oldStep.properties->polygonNodes(), oldStep.properties->volumeOrigin(),
      oldStep.properties->volumeOrientation(), oldStep.properties->voxelSize(),
      oldStep.properties->planeOrigin(),
      oldStep.properties->planeOrientation());
}

LassoSelectionStep::LassoSelectionStep()
    : SegmentationStep("LassoSelectionStep", getPrototypeSingleton()),
      properties(new LassoSelectionStepProperties(this)) {}

LassoSelectionStep::~LassoSelectionStep() {}

QSharedPointer<OperationResult> LassoSelectionStep::calculate(
    QSharedPointer<ParameterCopy> parameterCopy,
    QSharedPointer<ContainerData> containerData, QDBusObjectPath inputVolume) {
  Q_UNUSED(inputVolume);

  return this->runThreaded("AssignmentStep", [parameterCopy, containerData,
                                              this](const QSharedPointer<
                                                    Operation>& op) {
    LassoSelectionStepPropertiesCopy propertyCopy(
        parameterCopy->properties()[parameterCopy->mainNodePath()]);

    auto compoundVoxelData = qSharedPointerDynamicCast<VolumeDataVoxel>(
        containerData->getElement("labelVolume"));

    if (qFuzzyCompare(compoundVoxelData->origin(),
                      propertyCopy.volumeOrigin()) &&
        qFuzzyCompare(compoundVoxelData->getSpacing(),
                      propertyCopy.voxelSize())) {
      qint64 voxelCount = 0;

      auto calculator = LassoCalculator(
          propertyCopy.polygonNodes(), compoundVoxelData,
          propertyCopy.planeOrigin(), propertyCopy.planeOrientation());

      calculator.growRegion();

      auto selectFunc =
          [&voxelCount](
              quint32& x, quint32& y, quint32& z,
              QSharedPointer<VolumeDataVoxelInst<SegmentationType>> labelData) {
            SegmentationType voxelVal =
                (SegmentationType)labelData->getVoxel(x, y, z);
            if (getBit(voxelVal, segmentationShift) == 0) {
              voxelCount++;
              setBit(voxelVal, segmentationShift);
              labelData->setVoxel(x, y, z, voxelVal);
            }
          };

      iterateAllPassedLabelVolumeVoxels(selectFunc, calculator.getFoundVoxels(),
                                        containerData, op, false);

      Q_EMIT(this->updateSelectedVoxelCount(voxelCount, true));

    } else {
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.InvalidOperation",
          QString("The orientation, the origin and the voxel size of the step "
                  "and the labelVolume should match"));
    }
  });
}

void LassoSelectionStep::resetUIWidget() {}

void LassoSelectionStep::onVoxelOpFinished(bool status) { Q_UNUSED(status); }

QString LassoSelectionStep::getInfoString() {
  return QString("Lasso Selection");
}

bool LassoSelectionStep::isAllowedChild(NodeKind object) {
  Q_UNUSED(object);
  return false;
}

void LassoSelectionStep::setProperties(QList<QVector3D> nodes,
                                       QVector3D volumeOrigin,
                                       QQuaternion volumeOrientation,
                                       QVector3D voxelSize,
                                       QVector3D planeOrigin,
                                       QQuaternion planeOrientation) {
  this->properties->setVolumeOrientation(volumeOrientation);
  this->properties->setVolumeOrigin(volumeOrigin);
  this->properties->setPolygonNodes(nodes);
  this->properties->setVoxelSize(voxelSize);
  this->properties->setPlaneOrigin(planeOrigin);
  this->properties->setPlaneOrientation(planeOrientation);
}

bool LassoSelectionStep::isAllowedParent(NodeKind object) {
  return object == NodeKind::Data;
}
bool LassoSelectionStep::isCreatableChild(NodeKind) { return false; }

QList<QString> LassoSelectionStep::supportedDBusInterfaces() { return {}; }

void LassoSelectionStep::initializeCustomUIPropSections() {}

NODE_PROTOTYPE_IMPL(LassoSelectionStep)

LassoCalculator::LassoCalculator(QList<QVector3D> nodes,
                                 QSharedPointer<VolumeDataVoxel> originalVolume,
                                 QVector3D planeOrigin,
                                 QQuaternion planeOrientation)
    : IndexCalculator(originalVolume, planeOrigin, planeOrientation) {
  // Convert 3d nodes [m] to plane coordinates
  for (auto node : nodes) {
    QVector3D planePoint = this->threeDToPlaneTrafo * node;
    this->nodesPlane.append(QPointF(planePoint.x(), planePoint.y()));
  }

  this->boundingBox = getBoundingBox();

  // calculate a circle around each node and use the voxels/pixels as seed
  // points of the flatfield algorithm
  QList<QPointF> pointsOnCircle;
  for (auto nodePlane : this->nodesPlane) {
    pointsOnCircle.append(this->getPointsOnEllipse(
        nodePlane, 10.0, this->voxelSize.x(), this->voxelSize.y()));
  }

  for (QPointF circlePoint : pointsOnCircle) {
    // not yet rounded
    QVector3D voxelCoord =
        PlaneToVoxelTrafo * QVector3D(circlePoint.x(), circlePoint.y(), 0);

    if (this->isPointInVolume(voxelCoord)) {
      this->visitNext.append(voxel_index((quint32)std::floor(voxelCoord.x()),
                                         (quint32)std::floor(voxelCoord.y()),
                                         (quint32)std::floor(voxelCoord.z())));
    }
  }
}

bool inline LassoCalculator::isVoxelInsidePolygon(voxel_index& currentVoxel) {
  QVector4D voxelCenter = QVector4D(std::get<0>(currentVoxel) + 0.5,
                                    std::get<1>(currentVoxel) + 0.5,
                                    std::get<2>(currentVoxel) + 0.5, 1);

  qreal xPos =
      QVector4D::dotProduct(this->voxelToPlaneXProjection, voxelCenter);
  qreal yPos =
      QVector4D::dotProduct(this->voxelToPlaneYProjection, voxelCenter);

  if (this->boundingBox.contains(xPos, yPos)) {
    // Ray Casting algorithm
    int i, j;
    bool c = false;
    for (i = 0, j = this->nodesPlane.size() - 1; i < this->nodesPlane.size();
         j = i++) {
      if (((this->nodesPlane[i].y() > yPos) !=
           (this->nodesPlane[j].y() > yPos)) &&
          (xPos < (this->nodesPlane[j].x() - this->nodesPlane[i].x()) *
                          (yPos - this->nodesPlane[i].y()) /
                          (this->nodesPlane[j].y() - this->nodesPlane[i].y()) +
                      this->nodesPlane[i].x()))
        c = !c;
    }
    return c;
  } else {
    // Voxel is not even in bounding box
    return false;
  }
}

bool LassoCalculator::areCriteriaMet(voxel_index& voxelIndex) {
  if (doesVoxelIntersectWithPlane(voxelIndex) &&
      isVoxelInsidePolygon(voxelIndex)) {
    return true;
  } else
    return false;
}

QList<QPointF> LassoCalculator::getPointsOnEllipse(QPointF center, int r,
                                                   float xShift, float yShift) {
  if (r < 0) return {};

  // Run midpoint circle algorithm to get all pixels on the ellipsoid
  int x = r, y = 0;

  QList<QPointF> pointsOnCircle;
  float xCenter = center.x();
  float yCenter = center.y();

  // initial points on the axes
  pointsOnCircle.append(QPointF(r * xShift + xCenter, yCenter));
  pointsOnCircle.append(QPointF(-r * xShift + xCenter, yCenter));
  pointsOnCircle.append(QPointF(xCenter, r * yShift + yCenter));
  pointsOnCircle.append(QPointF(xCenter, -r * yShift + yCenter));

  // Initialising the value of P
  int P = 1 - r;
  while (x > y) {
    y++;

    // Mid-point is inside or on the perimeter
    if (P <= 0) P = P + 2 * y + 1;
    // Mid-point is outside the perimeter
    else {
      x--;
      P = P + 2 * y - 2 * x + 1;
    }

    // All the perimeter points have already been printed
    if (x < y) break;

    // Generated point and reflection in other octants after translation

    pointsOnCircle.append(QPointF(x + xCenter, y * yShift + yCenter));
    pointsOnCircle.append(QPointF(-x + xCenter, y * yShift + yCenter));
    pointsOnCircle.append(QPointF(x + xCenter, -y * yShift + yCenter));
    pointsOnCircle.append(QPointF(-x + xCenter, -y * yShift + yCenter));

    // If the generated point is on the line x = y then
    // the perimeter points have already been added
    if (x != y) {
      pointsOnCircle.append(
          QPointF(y * yShift + xCenter, x * xShift + yCenter));
      pointsOnCircle.append(
          QPointF(-y * yShift + xCenter, x * xShift + yCenter));
      pointsOnCircle.append(
          QPointF(y * yShift + xCenter, -x * xShift + yCenter));
      pointsOnCircle.append(
          QPointF(-y * yShift + xCenter, -x * xShift + yCenter));
    }
  }
  return pointsOnCircle;
}

QRectF LassoCalculator::getBoundingBox() {
  qreal minX = std::numeric_limits<qreal>::max();
  qreal maxX = std::numeric_limits<qreal>::min();
  qreal minY = std::numeric_limits<qreal>::max();
  qreal maxY = std::numeric_limits<qreal>::min();

  for (QPointF node : this->nodesPlane) {
    if (node.x() > maxX) maxX = node.x();
    if (node.x() < minX) minX = node.x();
    if (node.y() > maxY) maxY = node.y();
    if (node.y() < minY) minY = node.y();
  }
  return QRectF(minX - 10 * this->voxelSize.x(),
                minY - 10 * this->voxelSize.y(),
                (maxX - minX) + 10 * this->voxelSize.x(),
                (maxY - minY) + 10 * this->voxelSize.y());
}
