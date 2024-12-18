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

#include <Voxie/Data/MovableDataNode.hpp>
#include <Voxie/Data/Prototypes.forward.hpp>
#include <VoxieBackend/Data/SurfaceData.hpp>

#include <Voxie/Node/DataNode.hpp>
#include <Voxie/Node/NodePrototype.hpp>

#include <Voxie/Voxie.hpp>

#include <QtCore/QObject>
#include <QtCore/QVector>

#include <QLabel>
#include <QtGui/QVector3D>

#include <QInputDialog>
#include <QSharedPointer>
#include <array>

namespace vx {
class VolumeNode;

class VOXIECORESHARED_EXPORT SurfaceNode : public MovableDataNode {
  Q_OBJECT
  VX_REFCOUNTEDOBJECT
  VX_NODE_IMPLEMENTATION("de.uni_stuttgart.Voxie.Data.Surface")

  QLabel* numberOfVerticesLabel;
  QLabel* numberOfTrianglesLabel;
  QLabel* attributesLabel;

  QSharedPointer<SurfaceData> surface_;

  VolumeNode* getOriginatingVolumeNode();
  VolumeNode* getOriginatingVolumeNode(QList<Node*> parents);

 public:
  SurfaceNode();

  QSharedPointer<Data> data() override;

  ~SurfaceNode();

  QSharedPointer<SurfaceData> surface() { return this->surface_; }
  void setSurface(QSharedPointer<SurfaceData> newSurface);

  /**
   * @brief updateProperties updates the displayed properties of the surface in
   * the property section
   */
  void updateProperties();

  void adjustPosition(QVector3D position, bool isAbsolute = false) override;
  void adjustRotation(QQuaternion rotation, bool isAbsolute = false) override;

  BoundingBox3D boundingBoxObject() override;

 protected:
  void setDataImpl(const QSharedPointer<Data>& data) override;
};
}  // namespace vx
