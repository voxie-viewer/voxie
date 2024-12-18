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

#include "ObjectProperties.hpp"
#include "ui_ObjectProperties.h"

ObjectProperties::ObjectProperties(QWidget* parent, bool showPosition,
                                   bool showRotation)
    : QWidget(parent), ui(new Ui::ObjectProperties) {
  ui->setupUi(this);
  mode = "quaternion";
  objectRotation = QQuaternion(1.0f, 0.0f, 0.0f, 0.0f);

  // TODO: Split this into position and rotation UIs

  if (!showPosition) {
    delete this->findChild<QLayout*>("positionLayout");
    delete this->findChild<QWidget*>("positionLabel");
    delete this->findChild<QWidget*>("positionXLineEdit");
    delete this->findChild<QWidget*>("positionYLineEdit");
    delete this->findChild<QWidget*>("positionZLineEdit");
    setMinimumSize(minimumWidth(), 0);
  }

  if (!showRotation) {
    this->findChild<QWidget*>("rotationFrame")->setVisible(false);
    setMinimumSize(minimumWidth(), 0);
  }
}

ObjectProperties::~ObjectProperties() { delete ui; }

bool ObjectProperties::getFloat(const QString text, float* value) {
  bool ok = false;
  *value = text.toFloat(&ok);
  return ok;
}

float ObjectProperties::getXPosition() {
  return this->findChild<QLineEdit*>("positionXLineEdit")->text().toFloat();
}

float ObjectProperties::getYPosition() {
  return this->findChild<QLineEdit*>("positionYLineEdit")->text().toFloat();
}
float ObjectProperties::getZPosition() {
  return this->findChild<QLineEdit*>("positionZLineEdit")->text().toFloat();
}

float ObjectProperties::getWRotation() {
  return this->findChild<QLineEdit*>("rotationWLineEdit")->text().toFloat();
}

float ObjectProperties::getXRotation() {
  return this->findChild<QLineEdit*>("rotationXLineEdit")->text().toFloat();
}

float ObjectProperties::getYRotation() {
  return this->findChild<QLineEdit*>("rotationYLineEdit")->text().toFloat();
}

float ObjectProperties::getZRotation() {
  return this->findChild<QLineEdit*>("rotationZLineEdit")->text().toFloat();
}

void ObjectProperties::init() {
  Q_EMIT positionRequest();
  Q_EMIT rotationRequest();
}

void ObjectProperties::setPosition(const QVector3D& position) {
  QLineEdit* xPosBox = this->findChild<QLineEdit*>("positionXLineEdit");
  QLineEdit* yPosBox = this->findChild<QLineEdit*>("positionYLineEdit");
  QLineEdit* zPosBox = this->findChild<QLineEdit*>("positionZLineEdit");

  this->ignoreUpdates = true;
  xPosBox->setText(QString::number(position.x()));
  yPosBox->setText(QString::number(position.y()));
  zPosBox->setText(QString::number(position.z()));
  this->ignoreUpdates = false;
}

void ObjectProperties::setRotation(const QQuaternion& rotation) {
  QLineEdit* wRotBox = this->findChild<QLineEdit*>("rotationWLineEdit");
  QLineEdit* xRotBox = this->findChild<QLineEdit*>("rotationXLineEdit");
  QLineEdit* yRotBox = this->findChild<QLineEdit*>("rotationYLineEdit");
  QLineEdit* zRotBox = this->findChild<QLineEdit*>("rotationZLineEdit");

  this->ignoreUpdates = true;
  this->objectRotation = rotation;
  if (mode == "quaternion") {
    wRotBox->setText(QString::number(objectRotation.scalar()));
    xRotBox->setText(QString::number(objectRotation.x()));
    yRotBox->setText(QString::number(objectRotation.y()));
    zRotBox->setText(QString::number(objectRotation.z()));
  } else if (mode == "euler") {
    xRotBox->setText(QString::number(objectRotation.toEulerAngles().x()));
    yRotBox->setText(QString::number(objectRotation.toEulerAngles().y()));
    zRotBox->setText(QString::number(objectRotation.toEulerAngles().z()));
  }
  this->ignoreUpdates = false;
}

void ObjectProperties::setPosition(MovableDataNode* selectedMovableDataNode) {
  if (this->ignoreUpdates) return;

  QLineEdit* xPosBox = this->findChild<QLineEdit*>("positionXLineEdit");
  QLineEdit* yPosBox = this->findChild<QLineEdit*>("positionYLineEdit");
  QLineEdit* zPosBox = this->findChild<QLineEdit*>("positionZLineEdit");

  if (!selectedMovableDataNode) {
    xPosBox->clear();
    yPosBox->clear();
    zPosBox->clear();
    return;
  }

  this->ignoreUpdates = true;
  auto pos = selectedMovableDataNode->getAdjustedPosition();
  xPosBox->setText(QString::number(pos.x()));
  yPosBox->setText(QString::number(pos.y()));
  zPosBox->setText(QString::number(pos.z()));

  this->ignoreUpdates = false;
}

void ObjectProperties::setRotation(MovableDataNode* selectedMovableDataNode) {
  if (this->ignoreUpdates) return;

  QLineEdit* wRotBox = this->findChild<QLineEdit*>("rotationWLineEdit");
  QLineEdit* xRotBox = this->findChild<QLineEdit*>("rotationXLineEdit");
  QLineEdit* yRotBox = this->findChild<QLineEdit*>("rotationYLineEdit");
  QLineEdit* zRotBox = this->findChild<QLineEdit*>("rotationZLineEdit");

  if (!selectedMovableDataNode) {
    qWarning() << "Selected VolumeNode is not valid";
    wRotBox->clear();
    xRotBox->clear();
    yRotBox->clear();
    zRotBox->clear();
  } else {
    this->ignoreUpdates = true;
    this->objectRotation = selectedMovableDataNode->getAdjustedRotation();
    if (mode == "quaternion") {
      this->findChild<QLineEdit*>("rotationWLineEdit")
          ->setText(QString::number(objectRotation.scalar()));
      this->findChild<QLineEdit*>("rotationXLineEdit")
          ->setText(QString::number(objectRotation.x()));
      this->findChild<QLineEdit*>("rotationYLineEdit")
          ->setText(QString::number(objectRotation.y()));
      this->findChild<QLineEdit*>("rotationZLineEdit")
          ->setText(QString::number(objectRotation.z()));

    } else if (mode == "euler") {
      this->findChild<QLineEdit*>("rotationXLineEdit")
          ->setText(QString::number(objectRotation.toEulerAngles().x()));
      this->findChild<QLineEdit*>("rotationYLineEdit")
          ->setText(QString::number(objectRotation.toEulerAngles().y()));
      this->findChild<QLineEdit*>("rotationZLineEdit")
          ->setText(QString::number(objectRotation.toEulerAngles().z()));
    }
  }

  this->ignoreUpdates = false;
}

QVector3D ObjectProperties::getPosition() {
  return QVector3D(getXPosition(), getYPosition(), getZPosition());
}

QQuaternion ObjectProperties::getRotation() {
  return QQuaternion(getWRotation(), getXRotation(), getYRotation(),
                     getZRotation())
      .normalized();
}

void ObjectProperties::on_rotationModeComboBox_currentTextChanged(
    const QString& mode) {
  QLineEdit* scalar = this->findChild<QLineEdit*>("rotationWLineEdit");

  if (mode == "Quaternion (WXYZ)") {
    this->mode = "quaternion";
    scalar->setEnabled(true);

  } else if (mode == "Euler Angles (XYZ)") {
    this->mode = "euler";
    scalar->setText("");
    scalar->setEnabled(false);
  }
  Q_EMIT rotationRequest();
}

void ObjectProperties::on_positionXLineEdit_textChanged(const QString& text) {
  if (this->ignoreUpdates) return;

  float value;

  if (getFloat(text, &value)) {
    this->ignoreUpdates = true;
    Q_EMIT positionChanged(QVector3D(value, getYPosition(), getZPosition()),
                           true);
    this->ignoreUpdates = false;
  }
}

void ObjectProperties::on_positionYLineEdit_textChanged(const QString& text) {
  if (this->ignoreUpdates) return;

  float value;
  if (getFloat(text, &value)) {
    this->ignoreUpdates = true;
    Q_EMIT positionChanged(QVector3D(getXPosition(), value, getZPosition()),
                           true);
    this->ignoreUpdates = false;
  }
}

void ObjectProperties::on_positionZLineEdit_textChanged(const QString& text) {
  if (this->ignoreUpdates) return;

  float value;
  if (getFloat(text, &value)) {
    this->ignoreUpdates = true;
    Q_EMIT positionChanged(QVector3D(getXPosition(), getYPosition(), value),
                           true);
    this->ignoreUpdates = false;
  }
}

void ObjectProperties::on_rotationWLineEdit_textChanged(const QString& text) {
  if (this->ignoreUpdates) return;

  float value;
  if (getFloat(text, &value)) {
    this->objectRotation =
        QQuaternion(value, getXRotation(), getYRotation(), getZRotation())
            .normalized();

    if (this->objectRotation.isNull()) {
      qWarning() << "QQuaternion in object properties cant be (0,0,0,0)";
      this->rotationRequest();
    } else {
      this->ignoreUpdates = true;
      Q_EMIT rotationChanged(objectRotation, true);
      // qDebug()<<this->objectRotation;
      this->ignoreUpdates = false;
    }
  } else {
    qWarning() << "Wrong Syntax in object Properties Input";
  }
}

void ObjectProperties::on_rotationXLineEdit_textChanged(const QString& text) {
  if (this->ignoreUpdates) return;

  float value;
  if (getFloat(text, &value)) {
    if (mode == "quaternion") {
      this->objectRotation =
          QQuaternion(getWRotation(), value, getYRotation(), getZRotation())
              .normalized();

    } else if (mode == "euler") {
      this->objectRotation =
          QQuaternion::fromEulerAngles(value, getYRotation(), getZRotation())
              .normalized();
    } else {
      qWarning() << "No Rotationmode detected RotX";
    }

    if (this->objectRotation.isNull()) {
      qWarning() << "QQuaternion in object properties cant be (0,0,0,0)";
      this->rotationRequest();
      return;
    }

    this->ignoreUpdates = true;
    Q_EMIT rotationChanged(objectRotation, true);
    // qDebug()<<this->objectRotation;
    this->ignoreUpdates = false;

  } else {
    qWarning() << "Wrong Syntax in object Properties Input";
  }
}

void ObjectProperties::on_rotationYLineEdit_textChanged(const QString& text) {
  if (this->ignoreUpdates) return;

  float value;
  if (getFloat(text, &value)) {
    if (mode == "quaternion") {
      this->objectRotation =
          QQuaternion(getWRotation(), getXRotation(), value, getZRotation())
              .normalized();

    } else if (mode == "euler") {
      this->objectRotation =
          QQuaternion::fromEulerAngles(getXRotation(), value, getZRotation())
              .normalized();
    } else {
      qWarning() << "No Rotationmode detected: RotY";
    }

    if (this->objectRotation.isNull()) {
      qWarning() << "QQuaternion in object properties cant be (0,0,0,0)";
      this->rotationRequest();
      return;
    }
    this->ignoreUpdates = true;
    Q_EMIT rotationChanged(objectRotation, true);
    // qDebug()<<this->objectRotation;
    this->ignoreUpdates = false;

  } else {
    qWarning() << "Wrong Syntax in object Properties Input";
  }
}

void ObjectProperties::on_rotationZLineEdit_textChanged(const QString& text) {
  if (this->ignoreUpdates) return;

  float value;
  if (getFloat(text, &value)) {
    if (mode == "quaternion") {
      this->objectRotation =
          QQuaternion(getWRotation(), getXRotation(), getYRotation(), value)
              .normalized();

    } else if (mode == "euler") {
      this->objectRotation =
          QQuaternion::fromEulerAngles(getXRotation(), getYRotation(), value)
              .normalized();
    } else {
      qWarning() << "No Rotationmode detected: RotZ";
    }

    if (this->objectRotation.isNull()) {
      qWarning() << "QQuaternion in object properties cant be (0,0,0,0)";
      this->rotationRequest();
      return;
    }
    this->ignoreUpdates = true;
    Q_EMIT rotationChanged(objectRotation, true);
    // qDebug()<<this->objectRotation;
    this->ignoreUpdates = false;

  } else {
    qWarning() << "Wrong Syntax in object Properties Input";
  }
}
