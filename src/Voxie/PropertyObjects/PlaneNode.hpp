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

#include <Voxie/Data/PreviewBox.hpp>
#include <Voxie/Data/PropertySection.hpp>
#include <Voxie/Data/Prototypes.forward.hpp>
#include <Voxie/Data/QuaternionWidget.hpp>
#include <VoxieBackend/Data/PlaneInfo.hpp>

#include <Voxie/Node/PropertyNode.hpp>

#include <QCheckBox>
#include <QIcon>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLineEdit>

namespace vx {
class VOXIECORESHARED_EXPORT PlaneNode : public PropertyNode {
  Q_OBJECT

  NODE_PROTOTYPE_DECL_SEP(property_prop::Plane, PlaneNode)

  /*
  QLineEdit* positionEditX;
  QLineEdit* positionEditY;
  QLineEdit* positionEditZ;
  QLineEdit* rotationScalarEdit;
  QLineEdit* rotationEditX;
  QLineEdit* rotationEditY;
  QLineEdit* rotationEditZ;
  */

  QSharedPointer<PlaneInfo> _plane;

 public:
  PlaneNode(PlaneInfo* plane = 0);

  virtual QVariant getNodePropertyCustom(QString key) override;
  virtual void setNodePropertyCustom(QString key, QVariant value) override;

  QSharedPointer<const PlaneInfo> plane() { return this->_plane; }

  // TODO: Add setPlane() function or make it a property managed by Node
  // class?

  void setRotation(QQuaternion rotation);

  void setOrigin(QVector3D origin);

 Q_SIGNALS:
  // TODO: remove rotationChanged and originChanged?
  void rotationChanged(QQuaternion rotation);
  void originChanged(QVector3D origin);
  void planeChanged(vx::PlaneInfo oldPlane, vx::PlaneInfo newPlane,
                    bool equivalent);
};
}  // namespace vx
