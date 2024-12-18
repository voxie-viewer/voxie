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

// QDBusConnection should be included as early as possible:
// https://bugreports.qt.io/browse/QTBUG-48351 /
// https://bugreports.qt.io/browse/QTBUG-48377
#include <QtDBus/QDBusConnection>

#include "SegmentationUtils.hpp"

#include <QtGui/QColor>

namespace vx {

QList<QColor> defaultColorPalette = QList<QColor>{
    QColor(0xee, 0xee, 0x00), QColor(0x00, 0x99, 0x33),
    QColor(0x00, 0x66, 0x99), QColor(0x99, 0x00, 0xcc),
    QColor(0xcc, 0x66, 0x00),
};

LabelChangeTracker::LabelChangeTracker() {}
LabelChangeTracker::~LabelChangeTracker() {}

const QMap<qint64, qint64>& LabelChangeTracker::getChanges() const {
  return changeMap;
}
qint64 LabelChangeTracker::getSelectedChange() const { return selected; }

void LabelChangeTracker::mergeChangesFrom(const LabelChangeTracker& other) {
  for (const auto& key : other.changeMap.keys())
    changeMap[key] += other.changeMap[key];
  selected += other.selected;
}

IndexCalculator::IndexCalculator(QSharedPointer<VolumeDataVoxel> originalVolume,
                                 QVector3D planeOrigin,
                                 QQuaternion planeOrientation) {
  this->voxelSize = originalVolume->getSpacing();

  this->voxelDiagonal = voxelSize.length();

  this->distanceToPlaneThreshold =
      this->voxelDiagonal * this->distanceScaleFactor;

  this->volumeSize = originalVolume->getDimensions();

  // calculate all Transformations
  PlaneInfo plane(planeOrigin, planeOrientation);

  this->threeDToPlaneTrafo = plane.getPlaneTo3DTrafo().inverted();

  QMatrix4x4 voxelTo3DTrafo = originalVolume->getVoxelTo3DTrafo();

  this->voxelToPlaneTrafo = threeDToPlaneTrafo * voxelTo3DTrafo;

  this->PlaneToVoxelTrafo = this->voxelToPlaneTrafo.inverted();

  this->voxelToPlaneProjection =
      plane.get3DToPlaneProjection() * voxelTo3DTrafo;

  this->voxelToPlaneXProjection = this->voxelToPlaneProjection.row(0);
  this->voxelToPlaneYProjection = this->voxelToPlaneProjection.row(1);
  this->voxelToPlaneZTrafo = this->voxelToPlaneTrafo.row(2);
}

IndexCalculator::~IndexCalculator() {}

void IndexCalculator::growRegion() {
  // Flatfield Algorithm:
  while (!visitNext.empty()) {
    auto currentVoxel = visitNext.takeLast();
    visitedVoxels.insert(currentVoxel);

    if (this->areCriteriaMet(currentVoxel)) {
      foundVoxels.append(currentVoxel);
      // look at pixels that should be visited next
      addNeighboursToVisitNext(currentVoxel);
    }
  }
}

bool IndexCalculator::doesVoxelIntersectWithPlane(voxel_index& voxelIndex) {
  quint32 x = std::get<0>(voxelIndex);
  quint32 y = std::get<1>(voxelIndex);
  quint32 z = std::get<2>(voxelIndex);

  bool signFirstDistance = false;

  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      for (int k = 0; k < 2; k++) {
        QVector4D voxelCorner = QVector4D(x + i, y + j, z + k, 1);

        float cornerDistance =
            QVector4D::dotProduct(this->voxelToPlaneZTrafo, voxelCorner);

        if (i == 0 && j == 0 && k == 0)
          signFirstDistance = std::signbit(cornerDistance);
        else {
          if (signFirstDistance != std::signbit(cornerDistance)) return true;
        }

        if (abs(cornerDistance) >
            this->voxelDiagonal + this->distanceToPlaneThreshold) {
          return false;
        }

        else if (abs(cornerDistance) < this->distanceToPlaneThreshold) {
          return true;
        }
      }
    }
  }
  return false;
}

QList<voxel_index> IndexCalculator::getFoundVoxels() {
  return this->foundVoxels;
}

void forwardProgressFromTaskToOperation(Task* task,
                                        vx::io::Operation* operation) {
  QObject::connect(task, &Task::taskChanged, operation,
                   [operation](const QSharedPointer<const Task::Info>& info) {
                     operation->updateProgress(info->progress());
                   });
  operation->updateProgress(task->progress());
}

}  // namespace vx
