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

#include "MovableDataNode.hpp"

#include <VoxieClient/Map.hpp>

#include <Voxie/MathQt.hpp>

#include <Voxie/Data/BoundingBox3D.hpp>

#include <Voxie/Gui/ObjectProperties.hpp>

#include <Voxie/Node/PropertyValueConvertRaw.hpp>

using namespace vx;
using namespace vx::internal;

MovableDataNode::MovableDataNode(const QString& type,
                                 const QSharedPointer<NodePrototype>& prototype)
    : DataNode(type, prototype) {
  if (voxieRoot().isHeadless()) return;

  auto objProp = new ObjectProperties();

  objProp->setWindowTitle("Position Properties");

  // Properties changed by input in the GUI
  connect(objProp, &ObjectProperties::positionChanged, this,
          [this](QVector3D pos) { this->adjustPosition(pos, true); });
  connect(objProp, &ObjectProperties::rotationChanged, this,
          [this](QQuaternion rot) { this->adjustRotation(rot, true); });

  // GUI need the last valid Properties to reset itself
  connect(objProp, &ObjectProperties::positionRequest,
          [=]() { objProp->setPosition(this); });
  connect(objProp, &ObjectProperties::rotationRequest,
          [=]() { objProp->setRotation(this); });

  // Properties changed by interaction with the node
  connect(this, &MovableDataNode::adjustedPositionChanged,
          [=]() { objProp->setPosition(this); });
  connect(this, &MovableDataNode::adjustedRotationChanged,
          [=]() { objProp->setRotation(this); });

  // Global bounding box changes if position, rotation or object bounding box
  // changes
  connect(this, &MovableDataNode::adjustedPositionChanged, this,
          &MovableDataNode::boundingBoxGlobalChanged);
  connect(this, &MovableDataNode::adjustedRotationChanged, this,
          &MovableDataNode::boundingBoxGlobalChanged);
  connect(this, &MovableDataNode::boundingBoxObjectChanged, this,
          &MovableDataNode::boundingBoxGlobalChanged);

  objProp->init();

  this->addPropertySection(objProp);
}

void MovableDataNode::adjustPosition(QVector3D position, bool isAbsolute) {
  auto oldPos = adjustedPosition;
  if (isAbsolute) {
    adjustedPosition = position;
  } else {
    adjustedPosition += position;
  }
  Q_EMIT adjustedPositionChanged(this, oldPos);
  Q_EMIT changed();
}

void MovableDataNode::adjustRotation(QQuaternion rotation, bool isAbsolute) {
  auto oldRotation = adjustedRotation;
  if (isAbsolute) {
    adjustedRotation = rotation;
  } else {
    adjustedRotation = rotation * adjustedRotation;
  }
  adjustedRotation.normalize();
  Q_EMIT adjustedRotationChanged(this, oldRotation);
  Q_EMIT changed();
}

void MovableDataNode::objectPositionRequested() {
  Q_EMIT adjustedPositionChanged(this, getAdjustedPosition());
}

void MovableDataNode::objectRotationRequested() {
  Q_EMIT adjustedRotationChanged(this, getAdjustedRotation());
}

QVariant MovableDataNode::getNodePropertyCustom(QString key) {
  // TODO: Rename properties to MoveableDataNode
  if (key == "de.uni_stuttgart.Voxie.MovableDataNode.Translation") {
    return QVariant::fromValue(
        PropertyValueConvertRaw<vx::TupleVector<double, 3>, QVector3D>::toRaw(
            getAdjustedPosition()));
  } else if (key == "de.uni_stuttgart.Voxie.MovableDataNode.Rotation") {
    return QVariant::fromValue(
        PropertyValueConvertRaw<vx::TupleVector<double, 4>, QQuaternion>::toRaw(
            getAdjustedRotation()));
  } else {
    return DataNode::getNodePropertyCustom(key);
  }
}
void MovableDataNode::setNodePropertyCustom(QString key, QVariant value) {
  if (key == "de.uni_stuttgart.Voxie.MovableDataNode.Translation") {
    adjustPosition(
        PropertyValueConvertRaw<vx::TupleVector<double, 3>, QVector3D>::fromRaw(
            Node::parseVariant<vx::TupleVector<double, 3>>(value)),
        true);
  } else if (key == "de.uni_stuttgart.Voxie.MovableDataNode.Rotation") {
    adjustRotation(
        PropertyValueConvertRaw<vx::TupleVector<double, 4>, QQuaternion>::
            fromRaw(Node::parseVariant<vx::TupleVector<double, 4>>(value)),
        true);
  } else {
    return DataNode::setNodePropertyCustom(key, value);
  }
}

AffineMap<double, 3, 3> MovableDataNode::getObjectToGlobalTransformation() {
  auto translation =
      mapCast<double>(createTranslation(toVector(getAdjustedPosition())));
  AffineMap<double, 3, 3> rotation =
      rotationCast<double>(toRotation(getAdjustedRotation()));

  return translation * rotation;
}

BoundingBox3D MovableDataNode::boundingBoxGlobal() {
  auto bbObject = this->boundingBoxObject();
  auto objectToGlobal = this->getObjectToGlobalTransformation();

  auto bbGlobal = BoundingBox3D::empty();
  for (const auto& cornerObject : bbObject.corners())
    bbGlobal += BoundingBox3D::point(objectToGlobal.map(cornerObject));

  return bbGlobal;
}
