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

#include <Voxie/Node/DataNode.hpp>

#include <QQuaternion>
#include <QVector3D>
#include <QtCore/QObject>
#include <Voxie/Voxie.hpp>

namespace vx {

// TODO: Rename to MovableDataNode?

/**
 * @brief The PositionInterface class implements the positioning and rotation
 * system of datasets and surfaces
 */
class VOXIECORESHARED_EXPORT PositionInterface : public DataNode {
  Q_OBJECT
 private:
  QVector3D adjustedPosition;
  QQuaternion adjustedRotation;

 public Q_SLOTS:
  void objectPositionRequested();
  void objectRotationRequested();

 public:
  PositionInterface(const QString& type,
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

 Q_SIGNALS:

  /**
   * @brief adjustedPositionChanged is signalled if the positionInterface's
   * adjusted
   * position is changed.
   * @param positionInterface
   * @param oldPosition
   */
  void adjustedPositionChanged(PositionInterface* positionInterface,
                               QVector3D oldPosition);

  /**
   * @brief adjustedRotationChanged is signalled if the positionInterface's
   * adjusted
   * rotation is changed.
   * @param positionInterface
   * @param oldRotation
   */
  void adjustedRotationChanged(PositionInterface* positionInterface,
                               QQuaternion oldRotation);

  /**
   * @brief changed is signaled after changes to the node are complete.
   */
  void changed();
};

namespace internal {
class PositionInterfaceAdaptor {
  PositionInterface* object;

 public:
  PositionInterfaceAdaptor(PositionInterface* object) : object(object) {}

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

Q_DECLARE_METATYPE(vx::PositionInterface*)
