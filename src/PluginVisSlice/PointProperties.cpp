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

#include "PointProperties.hpp"

#include "ui_PointProperties.h"

#include <QColorDialog>
#include <QDebug>

#include <Voxie/Gui/SliceFromPointsDialog.hpp>

#include <Voxie/Data/GeometricPrimitiveObject.hpp>
#include <VoxieBackend/Data/GeometricPrimitiveData.hpp>

#include <PluginVisSlice/Prototypes.hpp>

using namespace vx;

PointProperties::PointProperties(QWidget* parent, SliceVisualizer* sv)
    : QWidget(parent),
      ui(new Ui::PointProperties)

{
  ui->setupUi(this);
  this->sv = sv;

  this->ui->lineEdit->setText("1 mm");
}

PointProperties::~PointProperties() { delete ui; }

void PointProperties::on_lineEdit_returnPressed() {
  QString input = this->ui->lineEdit->text();
  QStringList elements = input.split(" ");
  bool worked = false;
  if (elements.size() != 2) {
    this->ui->lineEdit->setText(latestUnit);
    return;
  }
  float value = elements.at(0).toFloat(&worked);
  if (worked) {
    QString newUnit = elements.at(1);
    if (newUnit == "m") {
    } else if (newUnit == "dm") {
      value /= 10;
    } else if (newUnit == "cm") {
      value /= 100;
    } else if (newUnit == "mm") {
      value /= 1000;
    } else if (newUnit == "µm") {
      value /= 1000000;
    } else {
      this->ui->lineEdit->setText(latestUnit);
      return;
    }
  }
  if (value >= 0 && worked) {
    latestUnit = input;
    Q_EMIT this->newVisibility(value);
  } else {
    this->ui->lineEdit->setText(latestUnit);
  }
}

void PointProperties::on_pushButton_clicked() {
  auto slice = new vx::SliceFromPointsDialog(true);
  auto gpo = dynamic_cast<vx::GeometricPrimitiveNode*>(
      this->sv->properties->geometricPrimitive());
  auto data = gpo ? gpo->geometricPrimitiveData()
                  : QSharedPointer<vx::GeometricPrimitiveData>();
  slice->setPoints(data ? data : GeometricPrimitiveData::create());
  connect(slice, &SliceFromPointsDialog::createNewPlane, this,
          &PointProperties::newPlaneFromCoordinates);
  connect(slice, &SliceFromPointsDialog::changeSlice, this,
          &PointProperties::changePlane);
  slice->setModal(true);
  slice->show();
}

void PointProperties::on_pushButton_2_clicked() { this->sv->createPlaneNode(); }

void PointProperties::changePlane(QVector3D one, QVector3D two,
                                  QVector3D three) {
  auto newPlane = this->createNewPlaneFromCoordinates(one, two, three);
  this->sv->setOrigin(newPlane.origin);
  this->sv->setRotation(newPlane.rotation);
}

PlaneInfo PointProperties::createNewPlaneFromCoordinates(QVector3D one,
                                                         QVector3D two,
                                                         QVector3D three) {
  QVector3D firstVector = one - two;
  firstVector.normalize();
  QVector3D secondVector = one - three;
  secondVector.normalize();
  if (firstVector.length() == 0) {
    if (std::abs(secondVector.x()) != 1)
      firstVector = QVector3D(1, 0, 0);
    else
      firstVector = QVector3D(0, 1, 0);
  }
  if (secondVector.length() == 0) {
    if (std::abs(firstVector.x()) != 1)
      secondVector = QVector3D(1, 0, 0);
    else
      secondVector = QVector3D(0, 1, 0);
  }
  if (firstVector == secondVector || secondVector == -firstVector) {
    if (std::abs(firstVector.x()) == 1) {
      secondVector = QVector3D(0, 1, 0);
    } else {
      secondVector = QVector3D(1, 0, 0);
    }
  }

  QVector3D newNormal = QVector3D::crossProduct(firstVector, secondVector);
  newNormal.normalize();
  newNormal = newNormal * -1;
  QQuaternion rot = QQuaternion::rotationTo(QVector3D(0, 0, 1), newNormal);

  return vx::PlaneInfo(one, rot);
}

void PointProperties::newPlaneFromCoordinates(QVector3D one, QVector3D two,
                                              QVector3D three) {
  auto newPlane = createNewPlaneFromCoordinates(one, two, three);
  auto planeProperty = createNode<PlaneNode>();
  // TODO: Pass properties to createNode() instead?
  planeProperty->setOrigin(newPlane.origin);
  planeProperty->setRotation(newPlane.rotation);
}
