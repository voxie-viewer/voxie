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

#include "LightSourceListElement.hpp"
#include "ui_LightSourceListElement.h"

LightSourceListElement::LightSourceListElement(QWidget* parent)
    : QWidget(parent), ui(new Ui::LightSourceListElement) {
  ui->setupUi(this);
  xPosBox = this->findChild<QLineEdit*>("positionXLineEdit");
  yPosBox = this->findChild<QLineEdit*>("positionYLineEdit");
  zPosBox = this->findChild<QLineEdit*>("positionZLineEdit");
  wDistanceBox = this->findChild<QLineEdit*>("distanceLineEdit");
  colButt = this->findChild<QPushButton*>("colorButton");
  checkBox = this->findChild<QCheckBox*>("lightSourceCheckBox");
}

LightSourceListElement::~LightSourceListElement() {
  removeGUI_Element_Pointer();
  delete ui;
}

void LightSourceListElement::init() {
  Q_EMIT positionRequest();
  Q_EMIT lightColorRequest();
  Q_EMIT activeRequest();
}

// #### Slots ####
void LightSourceListElement::positionRequestResponsed(QVector4D pos) {
  this->position = pos;
  if (xPosBox && yPosBox && zPosBox && wDistanceBox) {
    xPosBox->setText(QString::number(pos.x()));
    yPosBox->setText(QString::number(pos.y()));
    zPosBox->setText(QString::number(pos.z()));
    wDistanceBox->setText(QString::number(pos.w()));
  }
}
void LightSourceListElement::lightColorRequestResponsed(QColor color) {
  this->lightColor = color;
  if (colButt) setButtonColor(colButt, color);
}

void LightSourceListElement::activeRequestResponsed(bool active) {
  this->active = active;
  if (checkBox) checkBox->setChecked(active);
}

void LightSourceListElement::removeGUI_Element_Pointer() {
  xPosBox = NULL;
  yPosBox = NULL;
  zPosBox = NULL;
  wDistanceBox = NULL;

  colButt = NULL;
  checkBox = NULL;
}

void LightSourceListElement::setButtonColor(QPushButton* colButt,
                                            QColor color) {
  if (!colButt && !color.isValid()) return;

  QString colorButtonStyleSheet = "background-color:";
  colorButtonStyleSheet += (color.name());
  colorButtonStyleSheet += (" ; border: 1px solid black; ");
  colButt->setStyleSheet(colorButtonStyleSheet);
}

//#### GUI-Interaction ####
void LightSourceListElement::on_lightSourceCheckBox_clicked(bool checked) {
  this->active = checked;
  Q_EMIT activeChanged(checked);
  Q_EMIT updateViewRequest();
}

void LightSourceListElement::on_positionXLineEdit_editingFinished() {
  float value = 0.0;
  bool ok = false;

  if (xPosBox) {
    QString text = xPosBox->text();
    value = text.toFloat(&ok);

    if (ok) {
      QVector4D newPos = this->position;
      newPos.setX(value);
      this->position = newPos;
      Q_EMIT positionChanged(newPos);
      if (this->active) {
        Q_EMIT updateViewRequest();
      }
    } else {
      Q_EMIT positionRequest();
    }
  }
}

void LightSourceListElement::on_positionYLineEdit_editingFinished() {
  float value = 0.0;
  bool ok = false;

  if (yPosBox) {
    QString text = yPosBox->text();
    value = text.toFloat(&ok);

    if (ok) {
      QVector4D newPos = this->position;
      newPos.setY(value);
      this->position = newPos;
      Q_EMIT positionChanged(newPos);
      if (this->active) {
        Q_EMIT updateViewRequest();
      }
    } else {
      Q_EMIT positionRequest();
    }
  }
}

void LightSourceListElement::on_positionZLineEdit_editingFinished() {
  float value = 0.0;
  bool ok = false;

  if (zPosBox) {
    QString text = zPosBox->text();
    value = text.toFloat(&ok);

    if (ok) {
      QVector4D newPos = this->position;
      newPos.setZ(value);
      this->position = newPos;
      Q_EMIT positionChanged(newPos);
      if (this->active) {
        Q_EMIT updateViewRequest();
      }
    } else {
      Q_EMIT positionRequest();
    }
  }
}

void LightSourceListElement::on_distanceLineEdit_editingFinished() {
  float value = 0.0;
  bool ok = false;

  if (wDistanceBox) {
    QString text = wDistanceBox->text();
    value = text.toFloat(&ok);

    if (ok) {
      QVector4D newPos = this->position;
      newPos.setW(value);
      this->position = newPos;
      Q_EMIT positionChanged(newPos);
      if (this->active) {
        Q_EMIT updateViewRequest();
      }
    } else {
      Q_EMIT positionRequest();
    }
  }
}

void LightSourceListElement::on_colorButton_clicked() {
  if (colButt) {
    QColor color =
        QColorDialog::getColor(colButt->palette().window().color(), this);

    if (color.isValid()) {
      this->setButtonColor(colButt, color);
      this->lightColor = color;
      Q_EMIT lightColorChanged(color);
      if (this->active) {
        Q_EMIT updateViewRequest();
      }
    }
  }
}
