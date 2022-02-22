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

#include <Voxie/MathQt.hpp>
#include <VoxieClient/DBusTypeList.hpp>

#include <cmath>

#include <QtCore/QPointF>
#include <QtCore/QVector>

#include <QtGui/QMatrix4x4>
#include <QtGui/QQuaternion>
#include <QtGui/QVector3D>

namespace vx {

inline QVector3D invalidVec3() {
  double n = nan("");
  return QVector3D(n, n, n);
}

inline bool isInvalidVec3(const QVector3D& vec) {
  return std::isnan(QVector3D::dotProduct(vec, vec));
}

// TODO: Merge with vx::PlaneInfo?
struct Plane {
  /**
   * @brief origin of the planes coordinatesystem
   */
  QVector3D origin;
  /**
   * @brief rotation of the plane
   */
  QQuaternion rotation;
};

// TODO: Merge with vx::Plane?
/**
 * @brief The PlaneInfo struct defines a plane in 3D space.
 */
struct PlaneInfo {
  PlaneInfo() {}
  PlaneInfo(QVector3D origin, QQuaternion rotation)
      : origin(origin), rotation(rotation) {}
  PlaneInfo(const vx::Plane& plane)
      : origin(plane.origin), rotation(plane.rotation) {}
  operator vx::Plane() const {
    vx::Plane plane;
    plane.origin = origin;
    plane.rotation = rotation;
    return plane;
  }

  /**
   * @brief origin of the planes coordinatesystem
   */
  QVector3D origin;
  /**
   * @brief rotation of the plane
   */
  QQuaternion rotation;

  /**
   * @brief normal vector of this plane (perpendicular to plane).
   * @return  normal vector (unit vector)
   */
  QVector3D normal() const {
    return rotation.rotatedVector(QVector3D(0, 0, -1));
  }

  /**
   * @brief tangent vector, equivalent to planes x-axis
   * @return tangent vector (unit vector)
   */
  QVector3D tangent() const {
    return rotation.rotatedVector(QVector3D(1, 0, 0));
  }

  /**
   * @brief cotangent vector, equivalent to planes y-axis
   * @return cotangent vector (unit vector)
   */
  QVector3D cotangent() const {
    return rotation.rotatedVector(QVector3D(0, 1, 0));
  }

  /**
   * @brief get3DPoint translates a 2D point on the coordinatesystem spanned by
   * the plane to the corresponding point in 3D space.
   * @param x
   * @param y
   * @return point in 3D
   */
  QVector3D get3DPoint(qreal x, qreal y) const {
    QVector3D p = origin + (tangent() * x) + (cotangent() * y);
    return p -
           (distance(p) * normal());  // fix possible offset due to inaccuracy
  }

  /**
   * @see get3DPoint(qreal x, qreal y)
   */
  QVector3D get3DPoint(const QPointF& point) const {
    return get3DPoint(point.x(), point.y());
  }

  /**
   * @see get3DPoint(qreal x, qreal y)
   * @param destination
   */
  void get3DPoint(const QPointF& point, QVector3D& destination) const {
    destination = origin + (tangent() * point.x()) + (cotangent() * point.y());
  }

  /**
   * @brief projects vec onto plane and returns 2D point on plane
   * @param vec 3D point to be projected
   * @return 2D plane point of projected vector
   */
  QPointF get2DPlanePoint(const QVector3D& vec) const {
    QVector3D intersection = intersectLine(vec, normal());

    QMatrix4x4 translation;
    translation.setToIdentity();
    translation.translate(origin);
    QMatrix4x4 rotationM;
    rotationM.setToIdentity();
    rotationM.setColumn(0, tangent().toVector4D());
    rotationM.setColumn(1, cotangent().toVector4D());
    rotationM.setColumn(2, normal().toVector4D());
    QMatrix4x4 transform = translation * rotationM;
    transform = transform.inverted();

    return (transform * intersection).toPointF();
  }

  /**
   * @brief Calculates plane to 3D [m] transformation matrix (Affine-Trafo:
   * Rot+scaling). (Z-coordinate in plane coordinates should represents the
   * distane to the plane in 3D space [m])
   * @return plane to 3D [m] transformation matrix
   */
  QMatrix4x4 getPlaneTo3DTrafo() {
    QMatrix4x4 translation;
    translation.translate(origin);
    QMatrix4x4 rot(rotation.toRotationMatrix());
    QMatrix4x4 transform = translation * rot;
    return transform;
  }

  /**
   * @brief Calculates 3D plane coordinate [m] to global 3D coordinate [m]
   * transformation matrix
   * @return transformation matrix
   */
  AffineMap<double, 3, 3> getPlane3DToGlobalTrafo() {
    auto trans = mapCast<double>(createTranslation(toVector(origin)));
    AffineMap<double, 3, 3> rot = rotationCast<double>(toRotation(rotation));
    return trans * rot;
  }

  /**
   * @brief Calculates a 3D [m] to plane projection matrix (Affine-Trafo:
   * Rot+scaling). (Z-coordinate in plane coordinates should represents the
   * distane to the plane in 3D space [m]) (x, y coordinates are the projected
   * points)
   * @return  3D [m] to plane projection matrix
   */
  QMatrix4x4 get3DToPlaneProjection() {
    QMatrix4x4 ThreeDToPlaneProjection;
    // Matrix with normal*normal.T:
    QMatrix4x4 normalMatrix(
        pow(normal().x(), 2), normal().x() * normal().y(),
        normal().x() * normal().z(), 0., normal().x() * normal().y(),
        pow(normal().y(), 2), normal().y() * normal().z(), 0.,
        normal().x() * normal().z(), normal().y() * normal().z(),
        pow(normal().z(), 2), 0., 0., 0., 0., 0);

    QMatrix4x4 threeDtoPlane = getPlaneTo3DTrafo().inverted();
    QMatrix4x4 projection;
    projection -= (normalMatrix / normal().lengthSquared());
    ThreeDToPlaneProjection = threeDtoPlane * projection;

    return ThreeDToPlaneProjection;
  }

  /**
   * @brief Calculates a projection matrix from plane 3D coordinates [m] into
   * 2D on plane points [m].
   * projectionMatrix calculated after: I - (n*n.T)/(|n|**2)
   * @return projection matrix
   */
  Matrix<double, 3, 3> getPlane3DToPlaneProjection() {
    // construct normal*normal.transpose() matrix
    Matrix<double, 3, 3> normalMatrix = {
        {pow(normal().x(), 2), normal().x() * normal().y(),
         normal().x() * normal().z()},
        {normal().x() * normal().y(), pow(normal().y(), 2),
         normal().y() * normal().z()},
        {normal().x() * normal().z(), normal().y() * normal().z(),
         pow(normal().z(), 2)}};

    Matrix<double, 3, 3> plane3DToPlane2D = identityMatrix<double, 3>();
    plane3DToPlane2D -= (normalMatrix / normal().lengthSquared());

    return plane3DToPlane2D;
  }

  /**
   * @brief Calculates a projection matrix from plane gloabl coordinates [m]
   * into 2D on plane points [m].
   * @return projection matrix
   */
  AffineMap<double, 3UL, 3UL> getGlobalToPlane2DProjection() {
    auto threeDtoPlane = inverse(getPlane3DToGlobalTrafo());

    return threeDtoPlane * createLinearMap(getPlane3DToPlaneProjection());
  }

  /**
   * @brief Calculates corresponding plane rotation matrix from Quaternion
   * @return Plane rotation matrix
   */
  vx::Matrix<double, 3, 3> getRotationMatrix() {
    return rotationCast<double>(toRotation(rotation)).asRotationMatrix();
  }

  /**
   * @brief intersectLine calculates the intersection point of the plane with
   * given line.
   * @param linePoint
   * @param lineDirection
   * @return intersection point or invalidVec3 if line is parallel
   */
  QVector3D intersectLine(const QVector3D& linePoint,
                          const QVector3D& lineDirection) const {
    float denominator = QVector3D::dotProduct(lineDirection, normal());
    if (fabs(denominator) < 1e-6 /* isZero */) {
      // parallel
      return invalidVec3();
    } else {
      float numerator = QVector3D::dotProduct((origin - linePoint), normal());
      return linePoint + ((numerator / denominator) * lineDirection);
    }
    return linePoint;
  }

  /**
   * @brief intersectVolume calculates the bounding polygon of the planes
   * intersection with given volume.
   * @param volumeOrigin
   * @param volumeDimension
   * @return bounding polygon of intersection (unordered points)
   */
  QVector<QVector3D> intersectVolume(const QVector3D& volumeOrigin,
                                     const QVector3D& volumeDimension) const {
    // corners of volume box
    QVector3D bottomCorners[4] = {
        QVector3D(0, 0, 0) - volumeOrigin,
        QVector3D(0, 0, volumeDimension.z()) - volumeOrigin,
        QVector3D(0, volumeDimension.y(), 0) - volumeOrigin,
        QVector3D(0, volumeDimension.y(), volumeDimension.z()) - volumeOrigin};
    QVector3D topCorners[4];
    for (size_t i = 0; i < 4; i++) {
      topCorners[i] = bottomCorners[i] + QVector3D(volumeDimension.x(), 0, 0);
    }

    QVector<QVector3D> intersectionPoints;
    QVector3D vec;
    // check intersections with volume box
    {
      // check bottom lines
      vec = intersectLine(bottomCorners[0], QVector3D(0, 0, 1));
      if (!isInvalidVec3(vec)) {
        intersectionPoints.append(vec);
      }

      vec = intersectLine(bottomCorners[0], QVector3D(0, 1, 0));
      if (!isInvalidVec3(vec)) {
        intersectionPoints.append(vec);
      }

      vec = intersectLine(bottomCorners[3], QVector3D(0, 0, 1));
      if (!isInvalidVec3(vec)) {
        intersectionPoints.append(vec);
      }

      vec = intersectLine(bottomCorners[3], QVector3D(0, 1, 0));
      if (!isInvalidVec3(vec)) {
        intersectionPoints.append(vec);
      }

      // check top lines
      vec = intersectLine(topCorners[0], QVector3D(0, 0, 1));
      if (!isInvalidVec3(vec)) {
        intersectionPoints.append(vec);
      }

      vec = intersectLine(topCorners[0], QVector3D(0, 1, 0));
      if (!isInvalidVec3(vec)) {
        intersectionPoints.append(vec);
      }

      vec = intersectLine(topCorners[3], QVector3D(0, 0, 1));
      if (!isInvalidVec3(vec)) {
        intersectionPoints.append(vec);
      }

      vec = intersectLine(topCorners[3], QVector3D(0, 1, 0));
      if (!isInvalidVec3(vec)) {
        intersectionPoints.append(vec);
      }

      // check side lines
      vec = intersectLine(bottomCorners[0], QVector3D(1, 0, 0));
      if (!isInvalidVec3(vec)) {
        intersectionPoints.append(vec);
      }

      vec = intersectLine(bottomCorners[1], QVector3D(1, 0, 0));
      if (!isInvalidVec3(vec)) {
        intersectionPoints.append(vec);
      }

      vec = intersectLine(bottomCorners[2], QVector3D(1, 0, 0));
      if (!isInvalidVec3(vec)) {
        intersectionPoints.append(vec);
      }

      vec = intersectLine(bottomCorners[3], QVector3D(1, 0, 0));
      if (!isInvalidVec3(vec)) {
        intersectionPoints.append(vec);
      }
    }

    // remove out of box intersections
    for (int i = 0; i < intersectionPoints.length();) {
      vec = intersectionPoints[i] + volumeOrigin;
      if (vec.x() < 0 || vec.y() < 0 || vec.z() < 0 ||
          vec.x() > volumeDimension.x() || vec.y() > volumeDimension.y() ||
          vec.z() > volumeDimension.z()) {
        intersectionPoints.removeAt(i);
      } else {
        i++;
      }
    }

    return intersectionPoints;
  }

  /**
   * @brief isOnPlane checks if a point is on the plane.
   * @param point
   * @return true if distance to point is zero.
   */
  bool isOnPlane(const QVector3D& point) const {
    return (fabs(distance(point)) < 1e-6);
  }

  /**
   * @return distance of point to the plane, may be negative if is below the
   * plane.
   */
  float distance(const QVector3D& point) const {
    return point.distanceToPlane(origin, normal());
  }

  /**
   * @return distance as in hesse normal form of a plane (non-negative)
   */
  float distance() const { return (float)fabs(distance(QVector3D(0, 0, 0))); }

  /**
   * @brief getEquation returns the coordinate form of the plane.
   * @return
   */
  QVector4D getEquation() const {
    auto normal = rotation.rotatedVector(QVector3D(0, 0, -1));
    return QVector4D(normal.x(), normal.y(), normal.z(),
                     -(origin.x() * normal.x() + origin.y() * normal.y() +
                       origin.z() * normal.z()));
  }

  /**
   * @brief getEquation get the coordinate form of the plane and write it to the
   * given array.
   * @param eq A pointer to an float array with at least 4 cells.
   * @param adjustedPosition Position to adjust the plane
   * @param adjustedRotation Rotation to adjust the plane
   * @return
   */
  void getEquation(float* eq, QVector3D adjustedPosition = QVector3D(),
                   QQuaternion adjustedRotation = QQuaternion()) const {
    auto normal = rotation.rotatedVector(QVector3D(0, 0, -1));
    auto adjustedNormal = adjustedRotation.inverted().rotatedVector(normal);
    auto pos = origin - adjustedPosition;

    eq[0] = adjustedNormal.x();
    eq[1] = adjustedNormal.y();
    eq[2] = adjustedNormal.z();
    eq[3] = -((pos.x() * normal.x()) + (pos.y() * normal.y()) +
              (pos.z() * normal.z()));
  }
};

}  // namespace vx
Q_DECLARE_METATYPE(vx::PlaneInfo)
