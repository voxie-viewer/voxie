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

#include "CamProperties.hpp"
#include <QClipboard>
#include "ui_CamProperties.h"

#include <QDebug>

CamProperties::CamProperties(QWidget* parent)
    : QWidget(parent), ui(new Ui::CamProperties) {
  ui->setupUi(this);
  mode = "quaternion";
  cameraRotation = QQuaternion(1.0, 0.0, 0.0, 0.0);
  this->zoomMin = 0.0f;
  this->zoomMax = 1.0f;
  this->zoomCurrent = 66.7f;
}

CamProperties::~CamProperties() { delete ui; }

bool CamProperties::getFloat(const QString text, float* value) {
  bool ok = false;
  *value = text.toFloat(&ok);
  return ok;
}

void CamProperties::setRotation(QQuaternion rot) {
  if (this->ignoreUpdates) return;

  QLineEdit* wRotBox = this->findChild<QLineEdit*>("rotationWLineEdit");
  QLineEdit* xRotBox = this->findChild<QLineEdit*>("rotationXLineEdit");
  QLineEdit* yRotBox = this->findChild<QLineEdit*>("rotationYLineEdit");
  QLineEdit* zRotBox = this->findChild<QLineEdit*>("rotationZLineEdit");

  this->cameraRotation = rot;
  this->ignoreUpdates = true;
  if (mode == "quaternion") {
    wRotBox->setText(QString::number(this->cameraRotation.scalar()));
    xRotBox->setText(QString::number(this->cameraRotation.x()));
    yRotBox->setText(QString::number(this->cameraRotation.y()));
    zRotBox->setText(QString::number(this->cameraRotation.z()));

  } else if (mode == "euler") {
    xRotBox->setText(QString::number(this->cameraRotation.toEulerAngles().x()));
    yRotBox->setText(QString::number(this->cameraRotation.toEulerAngles().y()));
    zRotBox->setText(QString::number(this->cameraRotation.toEulerAngles().z()));
  }
  this->ignoreUpdates = false;
}

void CamProperties::setZoom(float zoom, float zoomMin, float zoomMax) {
  if (this->ignoreUpdates) return;

  QLineEdit* zoomBox = this->findChild<QLineEdit*>("zoomLineEdit");

  // Set zoomMin and zoomMax for local use
  this->zoomMin = zoomMin;
  this->zoomMax = zoomMax;

  // Calculate human readable zoom value between 0 and 100 with two decimal
  // digits.
  float zoomToView =
      round(((log(zoom) - log(zoomMin)) / (log(zoomMax) - log(zoomMin))) *
            10000.0f) /
      100.0f;

  // Set current zoom for local use in human readable form
  this->zoomCurrent = zoomToView;

  this->ignoreUpdates = true;
  zoomBox->setText(QString::number(this->zoomCurrent));
  this->ignoreUpdates = false;
}

float CamProperties::getWRotation() {
  return this->findChild<QLineEdit*>("rotationWLineEdit")->text().toFloat();
}

float CamProperties::getXRotation() {
  return this->findChild<QLineEdit*>("rotationXLineEdit")->text().toFloat();
}

float CamProperties::getYRotation() {
  return this->findChild<QLineEdit*>("rotationYLineEdit")->text().toFloat();
}

float CamProperties::getZRotation() {
  return this->findChild<QLineEdit*>("rotationZLineEdit")->text().toFloat();
}

void CamProperties::init() {
  Q_EMIT rotationRequest();
  Q_EMIT zoomRequest();
}

void CamProperties::on_rotationModeComboBox_currentTextChanged(
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

void CamProperties::on_rotationWLineEdit_textChanged(const QString& text) {
  if (this->ignoreUpdates) return;

  float value;
  if (getFloat(text, &value)) {
    this->cameraRotation =
        QQuaternion(value, getXRotation(), getYRotation(), getZRotation());

    if (this->cameraRotation.isNull()) {
      qWarning() << "QQuaternion in camera properties cant be (0,0,0,0)";
      this->rotationRequest();
    } else {
      this->ignoreUpdates = true;
      Q_EMIT rotationChanged(cameraRotation);
      // qDebug()<<this->cameraRotation;
      this->ignoreUpdates = false;
    }
  } else {
    qWarning() << "Wrong Syntax in camera Properties Input";
  }
}

void CamProperties::on_rotationXLineEdit_textChanged(const QString& text) {
  if (this->ignoreUpdates) return;

  float value;
  if (getFloat(text, &value)) {
    if (mode == "quaternion") {
      this->cameraRotation =
          QQuaternion(getWRotation(), value, getYRotation(), getZRotation());

    } else if (mode == "euler") {
      this->cameraRotation =
          QQuaternion::fromEulerAngles(value, getYRotation(), getZRotation());
    } else {
      qWarning() << "No Rotationmode detected RotX";
    }

    if (this->cameraRotation.isNull()) {
      qWarning() << "QQuaternion in camera properties cant be (0,0,0,0)";
      this->rotationRequest();
      return;
    }

    this->ignoreUpdates = true;
    Q_EMIT rotationChanged(cameraRotation);
    // qDebug()<<this->cameraRotation;
    this->ignoreUpdates = false;

  } else {
    qWarning() << "Wrong Syntax in camera Properties Input";
  }
}

void CamProperties::on_rotationYLineEdit_textChanged(const QString& text) {
  if (this->ignoreUpdates) return;

  float value;
  if (getFloat(text, &value)) {
    if (mode == "quaternion") {
      this->cameraRotation =
          QQuaternion(getWRotation(), getXRotation(), value, getZRotation());

    } else if (mode == "euler") {
      this->cameraRotation =
          QQuaternion::fromEulerAngles(getXRotation(), value, getZRotation());
    } else {
      qWarning() << "No Rotationmode detected: RotY";
    }

    if (this->cameraRotation.isNull()) {
      qWarning() << "QQuaternion in camera properties cant be (0,0,0,0)";
      this->rotationRequest();
      return;
    }
    this->ignoreUpdates = true;
    Q_EMIT rotationChanged(cameraRotation);
    // qDebug()<<this->cameraRotation;
    this->ignoreUpdates = false;

  } else {
    qWarning() << "Wrong Syntax in camera Properties Input";
  }
}

void CamProperties::on_rotationZLineEdit_textChanged(const QString& text) {
  if (this->ignoreUpdates) return;

  float value;
  if (getFloat(text, &value)) {
    if (mode == "quaternion") {
      this->cameraRotation =
          QQuaternion(getWRotation(), getXRotation(), getYRotation(), value);

    } else if (mode == "euler") {
      this->cameraRotation =
          QQuaternion::fromEulerAngles(getXRotation(), getYRotation(), value);
    } else {
      qWarning() << "No Rotationmode detected: RotZ";
    }

    if (this->cameraRotation.isNull()) {
      qWarning() << "QQuaternion in camera properties cant be (0,0,0,0)";
      this->rotationRequest();
      return;
    }
    this->ignoreUpdates = true;
    Q_EMIT rotationChanged(cameraRotation);
    // qDebug()<<this->cameraRotation;
    this->ignoreUpdates = false;

  } else {
    qWarning() << "Wrong Syntax in camera Properties Input";
  }
}

void CamProperties::on_zoomLineEdit_editingFinished() {
  QLineEdit* zoomBox = this->findChild<QLineEdit*>("zoomLineEdit");
  QString textValue = zoomBox->text();

  if (this->ignoreUpdates) return;

  float value;
  if (getFloat(textValue, &value)) {
    // Calculate zoom value from human readable value
    float zoomValue =
        exp((((value) / 100.0f) * (log(this->zoomMax) - log(this->zoomMin))) +
            log(this->zoomMin));

    // Check if zoom value is valid
    if (zoomValue >= this->zoomMin && zoomValue <= this->zoomMax) {
      // Update zoom value in visualizer
      this->ignoreUpdates = true;
      Q_EMIT zoomChanged(zoomValue);
      this->ignoreUpdates = false;

      // Set current zoom to current value
      this->zoomCurrent = textValue.toFloat();

    }
    // Set zoom box text to last valid value if zoom value was too small or too
    // big
    else {
      zoomBox->setText(QString::number(this->zoomCurrent));
    }

  }
  // Set zoom box text to last valid value if zoom value was no float
  else {
    zoomBox->setText(QString::number(this->zoomCurrent));
  }
}

void CamProperties::on_copyButton_pressed() {
  // Access system clipboard
  QClipboard* clipboard = QApplication::clipboard();

  // Get pointers to rotation and zoom lineEdits
  QLineEdit* wRotBox = this->findChild<QLineEdit*>("rotationWLineEdit");
  QLineEdit* xRotBox = this->findChild<QLineEdit*>("rotationXLineEdit");
  QLineEdit* yRotBox = this->findChild<QLineEdit*>("rotationYLineEdit");
  QLineEdit* zRotBox = this->findChild<QLineEdit*>("rotationZLineEdit");
  QLineEdit* zoomBox = this->findChild<QLineEdit*>("zoomLineEdit");

  // String to store in clipboard
  QString copyString = wRotBox->text() + " " + xRotBox->text() + " " +
                       yRotBox->text() + " " + zRotBox->text() + " " +
                       zoomBox->text();

  clipboard->setText(copyString);
}

void CamProperties::on_pasteButton_pressed() {
  // Access system clipboard
  QClipboard* clipboard = QApplication::clipboard();

  // Get pointers to rotation and zoom lineEdits
  QLineEdit* wRotBox = this->findChild<QLineEdit*>("rotationWLineEdit");
  QLineEdit* xRotBox = this->findChild<QLineEdit*>("rotationXLineEdit");
  QLineEdit* yRotBox = this->findChild<QLineEdit*>("rotationYLineEdit");
  QLineEdit* zRotBox = this->findChild<QLineEdit*>("rotationZLineEdit");
  QLineEdit* zoomBox = this->findChild<QLineEdit*>("zoomLineEdit");

  // Convert clipboard string to string list
  QStringList list =
      clipboard->text().split(QRegExp("\\s+"), QString::SkipEmptyParts);

  if (list.size() == 5) {
    // Check validity of clipboard content
    bool isValid = true;

    for (int i = 0; i < list.size(); i++) {
      if (isValid) {
        list[i].toFloat(&isValid);
      }
    }

    // Set lineEdits is values are valid
    if (isValid) {
      wRotBox->setText(list[0]);
      xRotBox->setText(list[1]);
      yRotBox->setText(list[2]);
      zRotBox->setText(list[3]);
      zoomBox->setText(list[4]);
    }
  }
}
