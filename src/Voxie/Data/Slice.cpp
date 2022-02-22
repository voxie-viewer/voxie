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

#include "Slice.hpp"

#include <VoxieBackend/Data/VolumeDataVoxel.hpp>
#include <VoxieBackend/Data/VolumeDataVoxelInst.hpp>

#include <VoxieBackend/OpenCL/CLInstance.hpp>

#include <Voxie/Data/Colorizer.hpp>

#include <VoxieClient/Exception.hpp>

#include <QtCore/QTime>

#include <QtGui/QMatrix4x4>

#include <QtGui/QIcon>

using namespace vx;
using namespace vx::internal;
using namespace vx;

int sliceCount = 0;

Slice::Slice(VolumeNode* parent) : QObject(), dataset(parent) {
  // new SliceAdaptor(this);

  Q_ASSERT(parent != nullptr);
  this->setParent(parent);

  // this->setDisplayName(QString("Slice") + QString::number(sliceCount));
  sliceCount++;

  this->cuttingPlane.origin = parent->volumeCenter();
}

float Slice::getPixelValue(qreal x, qreal y,
                           InterpolationMethod interpolation) const {
  QVector3D p = this->cuttingPlane.get3DPoint(x, y);
  auto voxelData =
      qSharedPointerDynamicCast<VolumeDataVoxel>(this->dataset->volumeData());
  if (voxelData) {
    return voxelData->performInGenericContext([&p, interpolation](auto& data) {
      return (float)data.getVoxelMetric(p.x(), p.y(), p.z(), interpolation);
    });
  }
  // TODO: Non-voxel datasets?
  return 0;
}

void Slice::setOrigin(const QPointF& planePoint) {
  QVector3D origin =
      this->cuttingPlane.get3DPoint(planePoint.x(), planePoint.y());
  if (origin == this->cuttingPlane.origin) {
    return;
  }
  PlaneInfo old = this->cuttingPlane;
  this->cuttingPlane.origin = origin;

  Q_EMIT this->planeChanged(this, old, this->cuttingPlane, true);
}

void Slice::setOrigin(const QVector3D& origin) {
  if (origin == this->cuttingPlane.origin) {
    return;
  }

  PlaneInfo old = this->cuttingPlane;
  this->cuttingPlane.origin = origin;

  Q_EMIT this->planeChanged(this, old, this->cuttingPlane,
                            old.isOnPlane(origin));
}

void Slice::setRotation(const QQuaternion& rot) {
  QQuaternion rotation = rot.normalized();
  if (rotation == this->cuttingPlane.rotation) {
    return;
  }

  PlaneInfo old = this->cuttingPlane;
  this->cuttingPlane.rotation = rotation;

  Q_EMIT this->planeChanged(
      this, old, this->cuttingPlane,
      (old.normal() - this->cuttingPlane.normal()).lengthSquared() < 1e-8);
}

void Slice::setPlane(const vx::PlaneInfo& plane) {
  QQuaternion rotation = plane.rotation.normalized();

  if (plane.origin == this->cuttingPlane.origin &&
      rotation == this->cuttingPlane.rotation) {
    return;
  }

  PlaneInfo old = this->cuttingPlane;
  this->cuttingPlane.origin = plane.origin;
  this->cuttingPlane.rotation = rotation;

  Q_EMIT this->planeChanged(
      this, old, this->cuttingPlane,
      (old.normal() - this->cuttingPlane.normal()).lengthSquared() < 1e-8 &&
          old.isOnPlane(plane.origin));
}

vx::PlaneInfo& Slice::getPlane() { return this->cuttingPlane; }

void Slice::rotateAroundAxis(qreal degrees, qreal x, qreal y, qreal z) {
  QQuaternion rotation = QQuaternion::fromAxisAndAngle(x, y, z, degrees);
  rotation = rotation * this->rotation();
  this->setRotation(rotation);
}

void Slice::translateAlongNormal(qreal delta) {
  if (delta != 0) {
    this->setOrigin(this->origin() + (this->normal() * delta));
  }
}

QRectF Slice::getBoundingRectangle() const {
  QVector<QVector3D> polygon3D;

  auto data = this->dataset->volumeData();
  if (!data) return QRectF();

  polygon3D = this->cuttingPlane.intersectVolume(-data->origin(),
                                                 data->getDimensionsMetric());

  if (polygon3D.length() < 2) {
    // no intersection with volume
    qDebug() << "no intersection with Volumne by getBoundingRectangle";
    return QRectF(0, 0, 0, 0);
  }

  QVector3D x = this->xAxis();
  QVector3D y = this->yAxis();
  QVector3D z = this->normal();
  QVector3D p = this->origin();

  // set up transformation matrices
  QMatrix4x4 translation;
  translation.setToIdentity();
  translation.translate(p);
  QMatrix4x4 rotation;
  rotation.setToIdentity();
  rotation.setColumn(0, x.toVector4D());
  rotation.setColumn(1, y.toVector4D());
  rotation.setColumn(2, z.toVector4D());

  QMatrix4x4 transform = translation * rotation;
  transform = transform.inverted();

  // find bounding rectangle
  qreal left, right, top, bottom;
  left = right = (transform * polygon3D[0]).x();
  top = bottom = (transform * polygon3D[0]).y();
  for (int i = 1; i < polygon3D.length(); i++) {
    // transform into plane coordinate system
    QVector3D transformed = transform * polygon3D[i];
    if (transformed.x() > right) {
      right = transformed.x();
    } else if (transformed.x() < left) {
      left = transformed.x();
    }

    if (transformed.y() > bottom) {
      bottom = transformed.y();
    } else if (transformed.y() < top) {
      top = transformed.y();
    }
  }

  return QRectF(left, top, right - left, bottom - top);
}

SliceImage Slice::generateImage(const QRectF& sliceArea, const QSize& imageSize,
                                InterpolationMethod interpolation) const {
  auto plane = this->getCuttingPlane();
  plane.origin = this->getDataset()->getAdjustedRotation().inverted() *
                 (plane.origin - this->getDataset()->getAdjustedPosition());
  plane.rotation =
      (this->getDataset()->getAdjustedRotation().inverted() * plane.rotation);
  return generateImage(this->getDataset()->volumeData().data(), plane,
                       sliceArea, imageSize, interpolation);
}

SliceImage Slice::generateImage(VolumeData* data, const vx::PlaneInfo& plane,
                                const QRectF& sliceArea, const QSize& imageSize,
                                InterpolationMethod interpolation) {
  auto voxelData = dynamic_cast<VolumeDataVoxel*>(data);
  QVector3D spacing;
  if (voxelData)
    spacing = voxelData->getSpacing();
  else
    // TODO: Non-voxel datasets?
    spacing = QVector3D(1, 1, 1);

  SliceImageContext imgContext = {plane, sliceArea, spacing};

  SliceImage img((size_t)imageSize.width(), (size_t)imageSize.height(),
                 imgContext, false);

  QVector3D origin = plane.origin;
  origin += plane.tangent() * sliceArea.x();
  origin += plane.cotangent() * sliceArea.y();

  try {
    data->extractSlice(origin, plane.rotation, imageSize,
                       sliceArea.width() / imageSize.width(),
                       sliceArea.height() / imageSize.height(), interpolation,
                       img);
  } catch (vx::Exception& e) {
    qWarning() << "Extracting slice failed";
  }

  return img;
}

bool Slice::saveSliceImage(QString file, uint width, uint height,
                           bool interpolation, qreal lowestValue,
                           qreal highestValue) const {
  return this->saveSliceImage(file, QSize(width, height),
                              getBoundingRectangle(), interpolation,
                              lowestValue, highestValue);
}

bool Slice::saveSliceImage(QString file, QSize imageSize, QRectF sliceArea,
                           bool interpolation, qreal lowestValue,
                           qreal highestValue) const {
  if (imageSize.width() == 0 || imageSize.height() == 0 ||
      sliceArea.width() == 0 || sliceArea.height() == 0) {
    return false;
  }

  qreal aspectImg = (imageSize.width() * 1.0) / imageSize.height();
  qreal aspectRect = sliceArea.width() / sliceArea.height();
  if (aspectImg > aspectRect) {
    // img wider
    sliceArea.setWidth(aspectImg * sliceArea.height());
  } else {
    // img taller
    sliceArea.setHeight(sliceArea.width() / aspectImg);
  }

  return vx::Colorizer::toQImageGray(
             this->generateImage(
                 sliceArea, imageSize,
                 interpolation ? vx::InterpolationMethod::Linear
                               : vx::InterpolationMethod::NearestNeighbor),
             lowestValue, highestValue)
      .save(file);
}
