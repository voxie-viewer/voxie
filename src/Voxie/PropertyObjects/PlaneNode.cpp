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

#include "PlaneNode.hpp"
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QVBoxLayout>

#include <Voxie/Data/Prototypes.hpp>

#include <Voxie/Node/NodePrototype.hpp>
#include <Voxie/Node/PropertyValueConvertRaw.hpp>

#include <QObject>
#include <QPushButton>

using namespace vx;

PlaneNode::PlaneNode(vx::PlaneInfo* plane)
    : PropertyNode("PlaneNode", getPrototypeSingleton()),
      properties(new PlaneProperties(this)) {
  this->propertyName = "Plane";
  this->setAutomaticDisplayName(this->propertyName);

  this->_plane = createQSharedPointer<PlaneInfo>();
  if (plane) {
    this->_plane->origin = plane->origin;
    this->_plane->rotation = plane->rotation;
  } else {
    this->_plane->origin = QVector3D(0, 0, 0);
    this->_plane->rotation = QQuaternion(1, 0, 0, 0);
  }

  PropertySection* section = new PropertySection();
  this->addPropertySection(section);

  /*
  this->positionEditX = new QLineEdit();
  this->positionEditX->setValidator(new QDoubleValidator());
  this->positionEditY = new QLineEdit();
  this->positionEditY->setValidator(new QDoubleValidator());
  this->positionEditZ = new QLineEdit();
  this->positionEditZ->setValidator(new QDoubleValidator());
  this->rotationScalarEdit = new QLineEdit();
  this->rotationScalarEdit->setValidator(new QDoubleValidator());
  this->rotationEditX = new QLineEdit();
  this->rotationEditX->setValidator(new QDoubleValidator());
  this->rotationEditY = new QLineEdit();
  this->rotationEditY->setValidator(new QDoubleValidator());
  this->rotationEditZ = new QLineEdit();
  this->rotationEditZ->setValidator(new QDoubleValidator());

  // TODO: this does not emit planeChanged
  this->positionEditX->connect(
      this->positionEditX, &QLineEdit::textChanged, this, [&](QString text) {
        bool validate;
        float number = text.toFloat(&validate);
        if (validate) {
          this->_plane.data()->origin.setX(number);
          this->positionEditX->setStyleSheet("background: 0");
          Q_EMIT this->originChanged(this->_plane.data()->origin);
        } else {
          this->positionEditX->setStyleSheet("background: red;");
        }
      });

  this->positionEditY->connect(
      this->positionEditY, &QLineEdit::textChanged, this, [&](QString text) {
        bool validate;
        float number = text.toFloat(&validate);
        if (validate) {
          this->_plane.data()->origin.setY(number);
          this->positionEditY->setStyleSheet("background: 0");
          Q_EMIT this->originChanged(this->_plane.data()->origin);
        } else {
          this->positionEditY->setStyleSheet("background: red;");
        }
      });

  this->positionEditZ->connect(
      this->positionEditZ, &QLineEdit::textChanged, this, [&](QString text) {
        bool validate;
        float number = text.toFloat(&validate);
        if (validate) {
          this->_plane.data()->origin.setZ(number);
          this->positionEditZ->setStyleSheet("background: 0");
          Q_EMIT this->originChanged(this->_plane.data()->origin);
        } else {
          this->positionEditZ->setStyleSheet("background: red;");
        }
      });

  this->rotationScalarEdit->connect(
      this->rotationScalarEdit, &QLineEdit::textChanged, this,
      [&](QString text) {
        bool validate;
        float number = text.toFloat(&validate);
        if (validate) {
          this->_plane.data()->rotation.setScalar(number);
          this->rotationScalarEdit->setStyleSheet("background: 0");
          Q_EMIT this->rotationChanged(this->_plane.data()->rotation);
        } else {
          this->rotationScalarEdit->setStyleSheet("background: red;");
        }
      });

  this->rotationEditX->connect(
      this->rotationEditX, &QLineEdit::textChanged, this, [&](QString text) {
        bool validate;
        float number = text.toFloat(&validate);
        if (validate) {
          this->_plane.data()->rotation.setX(number);
          this->rotationEditX->setStyleSheet("background: 0");
          Q_EMIT this->rotationChanged(this->_plane.data()->rotation);
        } else {
          this->rotationEditX->setStyleSheet("background: red;");
        }
      });

  this->rotationEditY->connect(
      this->rotationEditY, &QLineEdit::textChanged, this, [&](QString text) {
        bool validate;
        float number = text.toFloat(&validate);
        if (validate) {
          this->_plane.data()->rotation.setY(number);
          this->rotationEditY->setStyleSheet("background: 0");
          Q_EMIT this->rotationChanged(this->_plane.data()->rotation);
        } else {
          this->rotationEditY->setStyleSheet("background: red;");
        }
      });

  this->rotationEditZ->connect(
      this->rotationEditZ, &QLineEdit::textChanged, this, [&](QString text) {
        bool validate;
        float number = text.toFloat(&validate);
        if (validate) {
          this->_plane.data()->rotation.setZ(number);
          this->rotationEditZ->setStyleSheet("background: 0;");
          Q_EMIT this->rotationChanged(this->_plane.data()->rotation);
        } else {
          this->rotationEditZ->setStyleSheet("background: red;");
        }
      });

  QLabel* label1 = new QLabel("Position X");
  QLabel* label2 = new QLabel("Position Y");
  QLabel* label3 = new QLabel("Position Z");
  QLabel* label4 = new QLabel("Rotation Scalar");
  QLabel* label5 = new QLabel("Rotation X");
  QLabel* label6 = new QLabel("Rotation Y");
  QLabel* label7 = new QLabel("Rotation Z");
  section->addProperty(label1);
  section->addProperty(positionEditX);
  section->addProperty(label2);
  section->addProperty(positionEditY);
  section->addProperty(label3);
  section->addProperty(positionEditZ);
  section->addProperty(label4);
  section->addProperty(rotationScalarEdit);
  section->addProperty(label5);
  section->addProperty(rotationEditX);
  section->addProperty(label6);
  section->addProperty(rotationEditY);
  section->addProperty(label7);
  section->addProperty(rotationEditZ);
  */
}

void PlaneNode::setRotation(QQuaternion rotation) {
  PlaneInfo old = *this->plane();

  this->_plane.data()->rotation = rotation;
  /*
  this->rotationScalarEdit->setText(QString::number(rotation.scalar()));
  this->rotationEditX->setText(QString::number(rotation.x()));
  this->rotationEditY->setText(QString::number(rotation.y()));
  this->rotationEditZ->setText(QString::number(rotation.z()));
  */

  Q_EMIT this->rotationChanged(rotation);
  Q_EMIT this->planeChanged(
      old, *this->plane(),
      (old.normal() - this->plane()->normal()).lengthSquared() < 1e-8);
}

void PlaneNode::setOrigin(QVector3D origin) {
  PlaneInfo old = *this->plane();

  this->_plane.data()->origin = origin;
  /*
  this->positionEditX->setText(QString::number(origin.x()));
  this->positionEditY->setText(QString::number(origin.y()));
  this->positionEditZ->setText(QString::number(origin.z()));
  */

  Q_EMIT this->originChanged(origin);
  Q_EMIT this->planeChanged(old, *this->plane(), old.isOnPlane(origin));
}

// TODO: replace this by a property managed by the Node class?
QVariant PlaneNode::getNodePropertyCustom(QString key) {
  if (key == "de.uni_stuttgart.Voxie.Property.Plane.Origin") {
    return QVariant::fromValue(
        PropertyValueConvertRaw<vx::TupleVector<double, 3>, QVector3D>::toRaw(
            this->plane()->origin));
  } else if (key == "de.uni_stuttgart.Voxie.Property.Plane.Orientation") {
    return QVariant::fromValue(
        PropertyValueConvertRaw<vx::TupleVector<double, 4>, QQuaternion>::toRaw(
            this->plane()->rotation));
  } else {
    return Node::getNodePropertyCustom(key);
  }
}

void PlaneNode::setNodePropertyCustom(QString key, QVariant value) {
  if (key == "de.uni_stuttgart.Voxie.Property.Plane.Origin") {
    setOrigin(
        PropertyValueConvertRaw<vx::TupleVector<double, 3>, QVector3D>::fromRaw(
            parseVariant<vx::TupleVector<double, 3>>(value)));
  } else if (key == "de.uni_stuttgart.Voxie.Property.Plane.Orientation") {
    setRotation(
        PropertyValueConvertRaw<vx::TupleVector<double, 4>, QQuaternion>::
            fromRaw(parseVariant<vx::TupleVector<double, 4>>(value)));
  } else {
    return Node::setNodePropertyCustom(key, value);
  }
}

NODE_PROTOTYPE_IMPL_2(Plane, Node)
