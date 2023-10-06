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

#include <Voxie/MathQt.hpp>

#include <Voxie/Data/PositionInterface.hpp>
#include <Voxie/Data/Prototypes.forward.hpp>

#include <Voxie/Node/DataNode.hpp>
#include <Voxie/Node/NodePrototype.hpp>

#include <functional>

#include <inttypes.h>

#include <QtCore/QObject>
#include <QtCore/QPair>
#include <QtCore/QTime>
#include <QtCore/QVariant>
#include <QtCore/QtGlobal>

#include <QtDBus/QDBusAbstractAdaptor>
#include <QtDBus/QDBusContext>
#include <QtDBus/QDBusObjectPath>

#include <QQuaternion>
#include <QVector3D>

/**
 * @ingroup data
 */
namespace vx {
class VolumeData;
class BoundingBox3D;

class VOXIECORESHARED_EXPORT VolumeNode : public PositionInterface {
  Q_OBJECT
  REFCOUNTEDOBJ_DECL(VolumeNode)

 private:
  VolumeProperties* properties;
  QSharedPointer<VolumeData> volumeDataPointer;

 public:
  static QSharedPointer<NodePrototype> getPrototypeSingleton();

  explicit VolumeNode();
  ~VolumeNode();

  QSharedPointer<Data> data() override;

  void setVolumeData(const QSharedPointer<VolumeData>& data);

 protected:
  void setDataImpl(const QSharedPointer<Data>& data) override;

 public:
  const QSharedPointer<VolumeData>& volumeData() const {
    return volumeDataPointer;
  }

  QVector3D origin() const;
  QQuaternion orientation() const;
  QVector3D size() const;
  QVector3D volumeCenter() const;
  float diagonalSize() const;

  // TODO: Rename to boundingBoxGlobal and consider object rotation and movement
  BoundingBox3D boundingBox();

  /**
   * @brief Calculates 3D object coordinate [m] to global 3D coordinate [m]
   * transformation matrix
   * @return transformation matrix
   */
  AffineMap<double, 3UL, 3UL> getObjectToGlobalTrafo();

 Q_SIGNALS:
  /**
   * @brief pointListChanged is signaled if one or multiple items in the point
   * list are edited, added or removed.
   */
  void pointListChanged();

  // Forwarded from properties
  // TODO: This are currently needed for
  // forwardSignalFromPropertyNodeOnReconnect(), but probably
  // forwardSignalFromPropertyNodeOnReconnect() should be changed to allow
  // forwarding property changed events directly (then it also could pass to
  // value for the OnReconnect() variant). Probably there also should be some
  // template magic so that one doesn't have to pass both the getter and the
  // changed handler for both properties.
  void rotationChanged(QQuaternion value);
  void translationChanged(QVector3D value);

  // TODO: Rename to boundingBoxGlobal and consider object rotation and movement
  void boundingBoxChanged();
};
}  // namespace vx
