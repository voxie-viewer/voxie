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
#include <VoxieBackend/Data/PlaneInfo.hpp>
#include <VoxieBackend/Data/SliceImage.hpp>
#include <VoxieBackend/Data/VolumeDataVoxel.hpp>

#include <QtCore/QObject>

#include <QtGui/QIcon>
#include <QtWidgets/QWidget>

namespace vx {

QMatrix4x4 inline getVoxelTo3DTrafo(QSharedPointer<VolumeDataVoxel> volume) {
  QMatrix4x4 voxelTo3D;
  voxelTo3D.translate(volume->origin());
  voxelTo3D.scale(volume->getSpacing());
  return voxelTo3D;
}

QMatrix4x4 inline getPlaneTo3DTrafo(PlaneInfo plane) {
  auto planeNormal = plane.normal();

  QMatrix4x4 inPlaneTranslation;
  inPlaneTranslation.translate(plane.origin);
  QMatrix4x4 inPlaneRotation;
  inPlaneRotation.setColumn(0, plane.tangent().toVector4D());
  inPlaneRotation.setColumn(1, plane.cotangent().toVector4D());
  inPlaneRotation.setColumn(2, planeNormal.toVector4D());
  QMatrix4x4 transform = inPlaneTranslation * inPlaneRotation;

  return transform;
}

QMatrix4x4 inline get3DToPlaneProjection(PlaneInfo plane) {
  QMatrix4x4 ThreeDToPlaneProjection;

  auto planeNormal = plane.normal();
  // Matrix with normal*normal.T:
  QMatrix4x4 normalMatrix(
      pow(planeNormal.x(), 2), planeNormal.x() * planeNormal.y(),
      planeNormal.x() * planeNormal.z(), 0., planeNormal.x() * planeNormal.y(),
      pow(planeNormal.y(), 2), planeNormal.y() * planeNormal.z(), 0.,
      planeNormal.x() * planeNormal.z(), planeNormal.y() * planeNormal.z(),
      pow(planeNormal.z(), 2), 0., 0., 0., 0., 0);

  QMatrix4x4 threeDtoPlane = getPlaneTo3DTrafo(plane).inverted();
  QMatrix4x4 storage;
  storage -= (normalMatrix / planeNormal.lengthSquared());
  ThreeDToPlaneProjection = threeDtoPlane * storage;

  return ThreeDToPlaneProjection;
}

QMatrix4x4 inline getVoxelToPlaneProjection(
    QSharedPointer<VolumeDataVoxel> volume, PlaneInfo plane) {
  QMatrix4x4 voxelTo3DTrafo = getVoxelTo3DTrafo(volume);
  QMatrix4x4 ThreeDToPlaneProjection = get3DToPlaneProjection(plane);
  return ThreeDToPlaneProjection * voxelTo3DTrafo;
}

QMatrix4x4 inline getVoxelToPlaneTrafo(QSharedPointer<VolumeDataVoxel> volume,
                                       PlaneInfo plane) {
  QMatrix4x4 voxelTo3D = getVoxelTo3DTrafo(volume);
  QMatrix4x4 ThreeDToPlane = getPlaneTo3DTrafo(plane).inverted();
  return ThreeDToPlane * voxelTo3D;
}

QMatrix4x4 inline getPlaneToPixelTrafo(SliceImage sliceImg) {
  QMatrix4x4 planeToPixelTrafo;
  QVector3D translation(-sliceImg.context().planeArea.left(),
                        -sliceImg.context().planeArea.bottom(), 0);
  QVector3D rotation(
      sliceImg.getDimension().width() / (sliceImg.context().planeArea.width()),
      -sliceImg.getDimension().height() /
          (sliceImg.context().planeArea.height()),
      1);

  planeToPixelTrafo.scale(rotation);
  planeToPixelTrafo.translate(translation);

  return planeToPixelTrafo;
}

QMatrix4x4 inline getVoxelToPixelProjection(
    QSharedPointer<VolumeDataVoxel> volume, PlaneInfo plane,
    SliceImage sliceImg) {
  QMatrix4x4 voxelToPlaneProjection = getVoxelToPlaneProjection(volume, plane);

  return getPlaneToPixelTrafo(sliceImg) * voxelToPlaneProjection;
}

QMatrix4x4 inline getPixelTo3DTrafo(PlaneInfo plane, SliceImage sliceImg) {
  QMatrix4x4 planeTo3DTrafo = getPlaneTo3DTrafo(plane);
  return planeTo3DTrafo * getPlaneToPixelTrafo(sliceImg).inverted();
}

// to get voxel indexes afterwards, the results has to be rounded
QMatrix4x4 inline getPixelToVoxelTransformation(
    QSharedPointer<VolumeDataVoxel> volume, PlaneInfo plane,
    SliceImage sliceImg) {
  return getVoxelTo3DTrafo(volume).inverted() *
         getPixelTo3DTrafo(plane, sliceImg);
}

}  // namespace vx
