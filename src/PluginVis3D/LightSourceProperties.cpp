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

#include "LightSourceProperties.hpp"
#include "ui_LightSourceProperties.h"

LightSourceProperties::LightSourceProperties(QWidget* parent)
    : QWidget(parent), ui(new Ui::LightSourceProperties) {
  ui->setupUi(this);

  ambientColorButton = this->findChild<QPushButton*>("ambientColorButton");
  ambientlightSlider = this->findChild<QSlider*>("ambientlightSlider");
  ambientlightLineEdit = this->findChild<QLineEdit*>("ambientlightLineEdit");

  diffuselightSlider = this->findChild<QSlider*>("diffuselightSlider");
  diffuselightLineEdit = this->findChild<QLineEdit*>("diffuselightLineEdit");
  useAbsShadingValueCheckBox =
      this->findChild<QCheckBox*>("useAbsShadingValueCheckBox");

  propertieslightSourceElement =
      findChild<LightSourceListElement*>("addRowWidget");
  if (propertieslightSourceElement) {
    propertiesLightSource = new LightSource(propertieslightSourceElement);

    // ### Connections between LightSourceProperties and LightSource
    connect(this, &LightSourceProperties::positionRequest,
            propertiesLightSource, &LightSource::positionRequested);
    connect(propertiesLightSource, &LightSource::positionRequestResponse, this,
            &LightSourceProperties::positionRequestResponsed);

    connect(this, &LightSourceProperties::lightColorRequest,
            propertiesLightSource, &LightSource::lightColorRequested);
    connect(propertiesLightSource, &LightSource::lightColorRequestResponse,
            this, &LightSourceProperties::lightSourceColorRequestResponsed);

    connect(this, &LightSourceProperties::activeRequest, propertiesLightSource,
            &LightSource::activeRequested);
    connect(propertiesLightSource, &LightSource::activeRequestResponse, this,
            &LightSourceProperties::activeRequestResponsed);

    // ### LightSourceListElement and LightSourece for position ###
    connect(propertieslightSourceElement,
            &LightSourceListElement::positionChanged, propertiesLightSource,
            &LightSource::positionChangedByGUI);
    connect(propertieslightSourceElement,
            &LightSourceListElement::positionRequest, propertiesLightSource,
            &LightSource::positionRequested);
    connect(propertiesLightSource, &LightSource::positionRequestResponse,
            propertieslightSourceElement,
            &LightSourceListElement::positionRequestResponsed);

    // ### LightSourceListElement and LightSourece for lightColor ###
    connect(propertieslightSourceElement,
            &LightSourceListElement::lightColorChanged, propertiesLightSource,
            &LightSource::lightColorChangedByGUI);
    connect(propertieslightSourceElement,
            &LightSourceListElement::lightColorRequest, propertiesLightSource,
            &LightSource::lightColorRequested);
    connect(propertiesLightSource, &LightSource::lightColorRequestResponse,
            propertieslightSourceElement,
            &LightSourceListElement::lightColorRequestResponsed);

    // ### LightSourceListElement and LightSourece for active ###
    connect(propertieslightSourceElement,
            &LightSourceListElement::activeChanged, propertiesLightSource,
            &LightSource::activeChangedByGUI);
    connect(propertieslightSourceElement,
            &LightSourceListElement::activeRequest, propertiesLightSource,
            &LightSource::activeRequested);
    connect(propertiesLightSource, &LightSource::activeRequestResponse,
            propertieslightSourceElement,
            &LightSourceListElement::activeRequestResponsed);

    propertieslightSourceElement
        ->init();  // LightsourceListelement updates itself with Requests for
                   // LightSource
  }

  QPushButton* addButton = this->findChild<QPushButton*>("addButton");
  if (addButton) {
    addButton->setIcon(QIcon(":/icons/plus-small.png"));
  }
}

LightSourceProperties::~LightSourceProperties() { delete ui; }

void LightSourceProperties::setButtonColor(QPushButton* colButt, QColor color) {
  if (!colButt && !color.isValid()) return;

  QString colorButtonStyleSheet = "background-color:";
  colorButtonStyleSheet += (color.name());
  colorButtonStyleSheet += (" ; border: 1px solid black; ");
  colButt->setStyleSheet(colorButtonStyleSheet);
}

// #### Slots ####
void LightSourceProperties::positionRequestResponsed(QVector4D pos) {
  this->position = pos;
}

void LightSourceProperties::lightSourceColorRequestResponsed(QColor color) {
  this->lightSourceColor = color;
}

void LightSourceProperties::activeRequestResponsed(bool active) {
  this->active = active;
}

void LightSourceProperties::ambientLightRequested() {
  Q_EMIT ambientLightRequestResponse(this->ambientLightColor);
}

void LightSourceProperties::lightSourcesListRequested() {
  Q_EMIT lightSourcesListRequestResponse(&this->lightSourceList);
}

void LightSourceProperties::updateViewRequested() {
  Q_EMIT updateViewRequest();
}

// #### GUI-Interactions ####

void LightSourceProperties::on_addButton_clicked() {
  Q_EMIT positionRequest();
  Q_EMIT lightColorRequest();
  Q_EMIT activeRequest();

  LightSourceListElement* listElement = new LightSourceListElement(this);
  LightSource* light = new LightSource(listElement, this->position,
                                       this->lightSourceColor, this->active);

  // ### LightSourceListElement and LightSourece for position ###
  connect(listElement, &LightSourceListElement::positionChanged, light,
          &LightSource::positionChangedByGUI);
  connect(listElement, &LightSourceListElement::positionRequest, light,
          &LightSource::positionRequested);
  connect(light, &LightSource::positionRequestResponse, listElement,
          &LightSourceListElement::positionRequestResponsed);

  // ### LightSourceListElement and LightSourece for lightColor ###
  connect(listElement, &LightSourceListElement::lightColorChanged, light,
          &LightSource::lightColorChangedByGUI);
  connect(listElement, &LightSourceListElement::lightColorRequest, light,
          &LightSource::lightColorRequested);
  connect(light, &LightSource::lightColorRequestResponse, listElement,
          &LightSourceListElement::lightColorRequestResponsed);

  // ### LightSourceListElement and LightSourece for active ###
  connect(listElement, &LightSourceListElement::activeChanged, light,
          &LightSource::activeChangedByGUI);
  connect(listElement, &LightSourceListElement::activeRequest, light,
          &LightSource::activeRequested);
  connect(light, &LightSource::activeRequestResponse, listElement,
          &LightSourceListElement::activeRequestResponsed);

  listElement->init();  // LightsourceListelement updates itself with Requests
                        // for LightSource

  connect(listElement, &LightSourceListElement::updateViewRequest, this,
          &LightSourceProperties::updateViewRequested);

  this->lightSourceList.append(light);

  QHBoxLayout* newRow = new QHBoxLayout();
  newRow->addWidget(listElement);

  QPushButton* butt = new QPushButton(listElement);
  butt->setMaximumSize(23, 23);
  butt->setMinimumSize(23, 23);

  butt->setIcon(QIcon(":/icons/cross-small.png"));
  newRow->addWidget(butt);

  // Behavior of the delete button when clicked
  connect(butt, &QPushButton::clicked, [=] {
    this->lightSourceList.removeOne(light);
    listElement->deleteLater();
    if (butt) {
      butt->deleteLater();
    }
    Q_EMIT updateViewRequest();
  });

  QVBoxLayout* listLayout = this->findChild<QVBoxLayout*>("listLayout");

  if (listLayout) {
    listLayout->addLayout(newRow);
  }
  if (this->active) {
    Q_EMIT updateViewRequest();
  }
}

void LightSourceProperties::on_ambientColorButton_clicked() {
  if (ambientColorButton) {
    QColor tmpColor = QColorDialog::getColor(
        ambientColorButton->palette().window().color(), this);
    if (tmpColor.isValid()) {
      this->ambientLightColor = tmpColor;
      setButtonColor(ambientColorButton, this->ambientLightColor);
      Q_EMIT updateViewRequest();
    }
  }
}

void LightSourceProperties::on_ambientlightSlider_valueChanged(int value) {
  if (ambientlightLineEdit) {
    ambientlightLineEdit->setText(QString::number(value));
    Q_EMIT ambientlightScaleChanged(value);
  }
}

void LightSourceProperties::on_diffuselightSlider_valueChanged(int value) {
  if (diffuselightLineEdit) {
    diffuselightLineEdit->setText(QString::number(value));
    Q_EMIT lightsourcesScaleChanged(value);
  }
}

void LightSourceProperties::on_ambientlightLineEdit_editingFinished() {
  int value = 0;
  bool ok = false;

  if (ambientlightLineEdit && ambientlightSlider) {
    QString text = ambientlightLineEdit->text();
    value = text.toInt(&ok);

    if (ok) {
      if (value < 0) {
        value = 0;
        ambientlightLineEdit->setText(QString::number(value));
      }
      if (value > 100) {
        value = 100;
        ambientlightLineEdit->setText(QString::number(value));
      }
      ambientlightSlider->setValue(value);
      Q_EMIT ambientlightScaleChanged(value);
    } else {
      ambientlightLineEdit->setText(
          QString::number(ambientlightSlider->value()));
    }
  }
}

void LightSourceProperties::on_diffuselightLineEdit_editingFinished() {
  int value = 0;
  bool ok = false;

  if (diffuselightLineEdit && diffuselightSlider) {
    QString text = diffuselightLineEdit->text();
    value = text.toInt(&ok);

    if (ok) {
      if (value < 0) {
        value = 0;
        diffuselightLineEdit->setText(QString::number(value));
      }
      if (value > 100) {
        value = 100;
        diffuselightLineEdit->setText(QString::number(value));
      }
      diffuselightSlider->setValue(value);
      Q_EMIT lightsourcesScaleChanged(value);
    } else {
      diffuselightLineEdit->setText(
          QString::number(ambientlightSlider->value()));
    }
  }
}

void LightSourceProperties::on_useAbsShadingValueCheckBox_clicked(
    bool checked) {
  Q_EMIT useAbsShadingValueChanged(checked);
}
