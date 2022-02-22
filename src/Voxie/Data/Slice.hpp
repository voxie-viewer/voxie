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

#include <Voxie/Voxie.hpp>

#include <Voxie/Data/VolumeNode.hpp>
#include <Voxie/Node/PropertyNode.hpp>
#include <VoxieBackend/Data/InterpolationMethod.hpp>
#include <VoxieBackend/Data/PlaneInfo.hpp>
#include <VoxieBackend/Data/SliceImage.hpp>

#include <QtCore/QObject>
#include <QtCore/QRectF>

#include <QtDBus/QDBusAbstractAdaptor>
#include <QtDBus/QDBusContext>

namespace vx {

// TODO: remove Slice class?

/**
 * @brief The Slice class is a Plane associated to a VolumeNode instance.
 * The plane is placed inside the VolumeNode's coordinatesystem.
 * It is used to extract SliceImages from the voxel dataset.
 */
class VOXIECORESHARED_EXPORT Slice : public QObject {
  Q_OBJECT
 public:
  explicit Slice(VolumeNode* parent);

  Q_PROPERTY(vx::PlaneInfo Plane READ getCuttingPlane WRITE setPlane)
  /**
   * @return this Slice's Plane (a copy of it).
   */
  // return type needs to be fully qualified for moc
  vx::PlaneInfo getCuttingPlane() const { return this->cuttingPlane; }
  void setPlane(const vx::PlaneInfo& plane);
  vx::PlaneInfo& getPlane();

  Q_PROPERTY(vx::VolumeNode* dataSet READ getDataset)
  /**
   * @return this Slice's VolumeNode instance.
   */
  // return type needs to be fully qualified for moc
  vx::VolumeNode* getDataset() const { return this->dataset; }

  /**
   * @param point on slice plane.
   * @param interpolation method for obtaining the value
   * @return value of the datasets voxel that is located at given point on the
   * plane.
   */
  float getPixelValue(QPointF point,
                      InterpolationMethod interpolation =
                          vx::InterpolationMethod::Linear) const {
    return getPixelValue(point.x(), point.y(), interpolation);
  }

  /**
   * @param x coordinate of point on slice plane
   * @param y coordinate of point on slice plane
   * @param interpolation method for obtaining the value
   * @return value of the datasets voxel that is located at given point on the
   * plane.
   */
  float getPixelValue(qreal x, qreal y,
                      InterpolationMethod interpolation =
                          vx::InterpolationMethod::Linear) const;

  /**
   * @return slice plane's x-Axis vector in 3D space.
   */
  QVector3D xAxis() const { return this->cuttingPlane.tangent(); }

  /**
   * @return slice plane's y-Axis vector in 3D space.
   */
  QVector3D yAxis() const { return this->cuttingPlane.cotangent(); }

  /**
   * @return slice plane's normal vector.
   */
  QVector3D normal() const { return this->cuttingPlane.normal(); }

  /**
   * @return slice plane's origin in 3D space.
   */
  QVector3D origin() const { return this->cuttingPlane.origin; }

  /**
   * @brief shifts slice plane's origin to another point on the plane.
   * @param planePoint point on this slice's plane.
   */
  void setOrigin(const QPointF& planePoint);

  /**
   * @brief sets slice plane's origin to given point in 3D space.
   * @param origin
   */
  void setOrigin(const QVector3D& origin);

  /**
   * @brief sets slice plane's origin to given point in 3D space.
   * @param x
   * @param y
   * @param z
   */
  void setOrigin(qreal x, qreal y, qreal z) { setOrigin(QVector3D(x, y, z)); }

  /**
   * @return this slice's rotation.
   */
  QQuaternion rotation() const { return this->cuttingPlane.rotation; }

  /**
   * @brief sets this slice's rotation
   * @param rotation
   */
  void setRotation(const QQuaternion& rotation);

  /**
   * @brief sets this slice's rotation
   * @param scalar part of quaternion
   * @param x vector part of quaternion
   * @param y vector part of quaternion
   * @param z vector part of quaternion
   */
  void setRotation(qreal scalar, qreal x, qreal y, qreal z) {
    this->setRotation(QQuaternion(scalar, x, y, z));
  }

  /**
   * @brief rotates plane around given axis by degrees
   * @see rotateAroundAxis(qreal, const QVector3D&)
   */
  void rotateAroundAxis(qreal degrees, qreal x, qreal y, qreal z);

  /**
   * @brief rotates plane around given axis by degrees
   * @param degress angle in degrees
   * @param axis to rotate around
   */
  void rotateAroundAxis(qreal degrees, const QVector3D& axis) {
    this->rotateAroundAxis(degrees, axis.x(), axis.y(), axis.z());
  }

  /**
   * @brief moves origin in direction of normal by delta
   * @param delta distance to move
   */
  void translateAlongNormal(qreal delta);

  /**
   * @brief calculates a rectangle on the slice that contains the intersection
   * of the slices plane with the dataset volume
   * @return bounding rectangle on plane that contains whole intersection with
   * volume
   */
  QRectF getBoundingRectangle() const;

  /**
   * @brief generates image from intersecting voxels with plane in give area.
   * @param sliceArea area on plane from which the image is created
   * @param imageSize size of resulting image (should match sliceAreas aspect
   * ratio for undistorted image)
   * @param interpolation either linear or nearestNeighbor
   */
  SliceImage generateImage(const QRectF& sliceArea, const QSize& imageSize,
                           InterpolationMethod interpolation =
                               vx::InterpolationMethod::Linear) const;

  static SliceImage generateImage(
      VolumeData* data, const PlaneInfo& plane, const QRectF& sliceArea,
      const QSize& imageSize,
      InterpolationMethod interpolation = vx::InterpolationMethod::Linear);

  /**
   * @brief saves image of slice to file (e.g. jpg or png)
   * @param file filepath where image should be saved to
   * @param width images width
   * @param height images height
   * @param interpolation when true image generating will use linear
   * interpolation
   * @param lowestValue all voxels below this value will be black
   * @param highestValue all voxels above this value will be white
   * @see saveSliceImage(QString, QSize, QRectF, bool, qreal, qreal)
   * @return true on success
   */
  bool saveSliceImage(QString file, uint width, uint height,
                      bool interpolation = true, qreal lowestValue = 0,
                      qreal highestValue = 1) const;

  /**
   * @brief saves image of slicearea to file (e.g. jpg or png)
   * @param file filepath where image should be saved to
   * @param imageSize size of image
   * @param sliceArea area from which image is generated
   *  (area will be expanded to fit image size aspect ratio if necessary)
   * @param interpolation when true image generating will use linear
   * interpolation
   * @param lowestValue all voxels below this value will be black
   * @param highestValue all voxels above this value will be white
   * @return true on success
   */
  bool saveSliceImage(QString file, QSize imageSize, QRectF sliceArea,
                      bool interpolation = true, qreal lowestValue = 0,
                      qreal highestValue = 1) const;

 Q_SIGNALS:
  /**
   * @brief emited when this slice's plane changed (rotated or origin moved).
   * @param oldPlane
   * @param newPlane
   * @param equivalent is true when old and new plane are identical (same normal
   * and new origin on old plane).
   */
  void planeChanged(vx::Slice* slice, vx::PlaneInfo oldPlane,
                    vx::PlaneInfo newPlane, bool equivalent);

 private:
  PlaneInfo cuttingPlane;
  VolumeNode* dataset = nullptr;
};

}  // namespace vx

Q_DECLARE_METATYPE(vx::Slice*)
