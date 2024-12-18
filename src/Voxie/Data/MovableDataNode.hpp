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

#include <Voxie/Node/DataNode.hpp>

#include <QtCore/QObject>

#include <QtGui/QQuaternion>
#include <QtGui/QVector3D>

namespace vx {
// Note: Forward declaration is without default parameter for dstDim
template <typename T, std::size_t srcDim, std::size_t dstDim>
class AffineMap;

/**
 * @brief The MovableDataNode class implements the positioning and rotation
 * system of datasets and surfaces
 */
class VOXIECORESHARED_EXPORT MovableDataNode : public DataNode {
  Q_OBJECT
 private:
  // TODO: Rename "adjusted*" to something like "objectPosition"
  // TODO: Use vx::Vector etc.
  QVector3D adjustedPosition;
  QQuaternion adjustedRotation;

 public Q_SLOTS:
  void objectPositionRequested();
  void objectRotationRequested();

 public:
  MovableDataNode(const QString& type,
                  const QSharedPointer<NodePrototype>& prototype);

  /**
   * @brief adjustPosition Changes the adjusted position.
   * @param position The relative or absolute position.
   * @param isAbsolute If true, the position is interpreted as absolute and
   * overwrites the current position, if false it is interpreted as offset and
   * added to the current position.
   */
  virtual void adjustPosition(QVector3D position, bool isAbsolute = false);

  /**
   * @brief adjustRotation Changes the adjusted rotation.
   * @param rotation The relative or absolute rotation.
   * @param isAbsolute If true, the rotation is interpreted as absolute and
   * overwrites the current rotation, if false it is multiplied to the current
   * rotation.
   */
  virtual void adjustRotation(QQuaternion rotation, bool isAbsolute = false);

  QVector3D getAdjustedPosition() { return adjustedPosition; }
  QQuaternion getAdjustedRotation() { return adjustedRotation; }

  QVariant getNodePropertyCustom(QString key) override;
  void setNodePropertyCustom(QString key, QVariant value) override;

  /**
   * @brief Calculates 3D object coordinate [m] to global 3D coordinate [m]
   * transformation matrix
   * @return transformation matrix
   */
  AffineMap<double, 3, 3> getObjectToGlobalTransformation();

  // TODO: Move the bounding box to a different class?
  virtual BoundingBox3D boundingBoxObject() = 0;
  // TODO: If the actual bounding box is not aligned to the object coordinate
  // system, the bounding box calculated by MovableDataNode might be too large.
  // Allow the node to override boundingBoxGlobal() to produce a smaller
  // bounding box in this case? Also needs to consider where
  // boundingBoxGlobalChanged() is emitted.
  BoundingBox3D boundingBoxGlobal();

 Q_SIGNALS:

  /**
   * @brief adjustedPositionChanged is signalled if the movableDataNode's
   * adjusted
   * position is changed.
   * @param movableDataNode
   * @param oldPosition
   */
  void adjustedPositionChanged(MovableDataNode* movableDataNode,
                               QVector3D oldPosition);

  /**
   * @brief adjustedRotationChanged is signalled if the movableDataNode's
   * adjusted
   * rotation is changed.
   * @param movableDataNode
   * @param oldRotation
   */
  void adjustedRotationChanged(MovableDataNode* movableDataNode,
                               QQuaternion oldRotation);

  // TODO: Remove?
  /**
   * @brief changed is signaled after changes to the node are complete.
   */
  void changed();

  void boundingBoxObjectChanged();
  void boundingBoxGlobalChanged();
};

namespace internal {
class MovableDataNodeAdaptor {
  MovableDataNode* object;

 public:
  MovableDataNodeAdaptor(MovableDataNode* object) : object(object) {}

  void AdjustPosition(const double x, const double y, const double z) {
    object->adjustPosition(QVector3D(x, y, z), true);
  }

  void AdjustRotation(const double scalar, const double x, const double y,
                      const double z) {
    object->adjustRotation(QQuaternion(scalar, x, y, z), true);
  }
};
}  // namespace internal

}  // namespace vx

Q_DECLARE_METATYPE(vx::MovableDataNode*)
