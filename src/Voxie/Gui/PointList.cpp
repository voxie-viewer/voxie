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

#include "PointList.hpp"

#include "ui_PointList.h"

#include <Voxie/IVoxie.hpp>

#include <Voxie/Gui/SliceFromPointsDialog.hpp>

#include <Voxie/Data/GeometricPrimitiveObject.hpp>
#include <Voxie/Data/Prototypes.hpp>
#include <VoxieBackend/Data/GeometricPrimitive.hpp>
#include <VoxieBackend/Data/GeometricPrimitiveData.hpp>

#include <Voxie/PropertyObjects/PlaneNode.hpp>

#include <QtCore/QStringListModel>

#include <QtGui/QVector4D>

using namespace vx;

// TODO: Get rid of combo boxes etc. (because they are also available as
// properties) and move things like distance measurement to the properties?

PointList::PointList(GeometricPrimitiveNode* gpo)
    : QWidget(), gpo(gpo), ui(new Ui::PointList) {
  suppressMeasurementPointChanged = false;

  ui->setupUi(this);
  unit = metres;
  roundTo = 3;

  this->fillUI();
  this->fillUnitCombobox();
  this->ui->lineFixed->setText(QString::number(roundTo));
  this->ui->laengenEingabe->setText("");

  QObject::connect(gpo->properties,
                   &GeometricPrimitiveProperties::measurementPrimitive1Changed,
                   this, &PointList::calculateDistance);
  QObject::connect(gpo->properties,
                   &GeometricPrimitiveProperties::measurementPrimitive2Changed,
                   this, &PointList::calculateDistance);

  QObject::connect(gpo->properties,
                   &GeometricPrimitiveProperties::selectedPrimitiveChanged,
                   this, [this](quint64 currentPoint) {
                     QString name = "";
                     auto data = this->gpo->geometricPrimitiveData();
                     if (data) {
                       auto point = data->getPrimitiveOrNull(currentPoint);
                       if (point) name = point->name();
                     }

                     this->ui->lineLabel->setText(name);
                   });
}

PointList::~PointList() { delete ui; }

void PointList::on_lineLabel_textChanged(const QString& arg1) {
  auto currentPoint = this->gpo->properties->selectedPrimitive();
  if (!currentPoint) return;

  auto data = gpo->geometricPrimitiveData();
  if (!data) return;
  auto oldPoint = data->getPrimitiveOrNull(currentPoint);
  if (!oldPoint) return;
  // qDebug() << "NameChange" << oldPoint->name() << arg1;
  if (oldPoint->name() == arg1) return;
  auto update = data->createUpdate();
  data->addOrReplacePrimitive(update, currentPoint, oldPoint->withName(arg1));
  update->finish(QJsonObject());
}

void PointList::on_deleteButton_pressed() {
  auto currentPoint = this->gpo->properties->selectedPrimitive();
  if (!currentPoint) return;

  if (currentPoint == this->gpo->properties->measurementPrimitive1())
    this->gpo->properties->setMeasurementPrimitive1(0);
  if (currentPoint == this->gpo->properties->measurementPrimitive2())
    this->gpo->properties->setMeasurementPrimitive2(0);

  auto data = gpo->geometricPrimitiveData();
  if (data) {
    auto update = data->createUpdate();
    data->addOrReplacePrimitive(update, currentPoint,
                                QSharedPointer<GeometricPrimitive>());
    update->finish(QJsonObject());
  }

  this->gpo->properties->setSelectedPrimitive(0);
}

void PointList::on_pointBox1_currentIndexChanged(int index) {
  if (suppressMeasurementPointChanged) return;
  // qDebug() << "on_pointBox1_currentIndexChanged" << index;
  if (index < 0 || index >= pointIdList.size()) {
    this->gpo->properties->setMeasurementPrimitive1(0);
  } else {
    this->gpo->properties->setMeasurementPrimitive1(pointIdList[index]);
  }
}

void PointList::on_pointBox2_currentIndexChanged(int index) {
  if (suppressMeasurementPointChanged) return;
  // qDebug() << "on_pointBox1_currentIndexChanged" << index;
  if (index < 0 || index >= pointIdList.size()) {
    this->gpo->properties->setMeasurementPrimitive2(0);
  } else {
    this->gpo->properties->setMeasurementPrimitive2(pointIdList[index]);
  }
}

void PointList::on_pushButton_clicked() {
  auto slice = new SliceFromPointsDialog();
  auto data = gpo->geometricPrimitiveData();
  slice->setPoints(data ? data : GeometricPrimitiveData::create());
  connect(slice, &SliceFromPointsDialog::createNewPlane, this,
          &PointList::newPlaneFromCoordinates);
  slice->setModal(true);
  slice->show();
}

void PointList::on_unitComboBox_currentIndexChanged(const QString& arg1) {
  if (arg1 == "metre") {
    unit = metres;
  } else if (arg1 == "decimetre") {
    unit = decimetres;
  } else if (arg1 == "centimetre") {
    unit = centimetres;
  } else if (arg1 == "millimetre") {
    unit = millimetres;
  } else if (arg1 == "micrometre") {
    unit = micrometres;
  }
  calculateDistance();
}

void PointList::on_lineFixed_returnPressed() {
  QString vis = this->ui->lineFixed->text();
  bool worked = true;
  int newRound = vis.toInt(&worked);
  if (newRound >= 0 && newRound <= 7) {
    roundTo = newRound;
    calculateDistance();
  } else {
    this->ui->lineFixed->setText(QString::number(roundTo));
  }
}

void PointList::fillUI() {
  getPointNameList(pointNameList, pointIdList);

  fillListWidget();

  QStringListModel* modelCO = new QStringListModel(this);
  QStringListModel* modelCT = new QStringListModel(this);
  modelCO->setStringList(pointNameList);
  modelCT->setStringList(pointNameList);

  auto currentIdOne = this->gpo->properties->measurementPrimitive1();
  auto currentIdTwo = this->gpo->properties->measurementPrimitive2();

  int indexOne = -1;
  for (int i = 0; i < pointIdList.size(); i++) {
    if (pointIdList[i] == currentIdOne) {
      indexOne = i;
      break;
    }
  }
  int indexTwo = -1;
  for (int i = 0; i < pointIdList.size(); i++) {
    if (pointIdList[i] == currentIdTwo) {
      indexTwo = i;
      break;
    }
  }

  suppressMeasurementPointChanged = true;

  ui->pointBox1->setModel(modelCO);
  ui->pointBox2->setModel(modelCT);

  // qDebug() << "setCurrentIndex" << indexOne << indexTwo;
  ui->pointBox1->setCurrentIndex(indexOne);
  ui->pointBox2->setCurrentIndex(indexTwo);

  suppressMeasurementPointChanged = false;

  int unitIndex = unit == metres        ? 0
                  : unit == decimetres  ? 1
                  : unit == centimetres ? 2
                  : unit == millimetres ? 3
                                        : 4;
  ui->unitComboBox->setCurrentIndex(unitIndex);
}

void PointList::fillListWidget() {
  ui->listWidget->clear();
  ui->listWidget->addItems(pointNameList);

  auto currentPoint = this->gpo->properties->selectedPrimitive();

  int index = -1;
  for (int i = 0; i < pointIdList.size(); i++) {
    if (pointIdList[i] == currentPoint) {
      index = i;
      break;
    }
  }
  ui->listWidget->setCurrentRow(index);
}

void PointList::fillUnitCombobox() {
  QStringListModel* unitModel = new QStringListModel(this);
  QStringList unitList;
  unitList.append("metre");
  unitList.append("decimetre");
  unitList.append("centimetre");
  unitList.append("millimetre");
  unitList.append("micrometre");
  unitModel->setStringList(unitList);
  ui->unitComboBox->setModel(unitModel);
}

void PointList::calculateDistance() {
  auto currentIdOne = this->gpo->properties->measurementPrimitive1();
  auto currentIdTwo = this->gpo->properties->measurementPrimitive2();

  auto data = gpo->geometricPrimitiveData();
  // qDebug() << "PointList::calculateDistance" << currentIdOne << currentIdTwo;
  if (!data || !currentIdOne || !currentIdTwo) {
    this->ui->laengenEingabe->setText("");
    return;
  }

  auto point1 = qSharedPointerDynamicCast<GeometricPrimitivePoint>(
      data->getPrimitiveOrNull(currentIdOne));
  auto point2 = qSharedPointerDynamicCast<GeometricPrimitivePoint>(
      data->getPrimitiveOrNull(currentIdTwo));
  if (!point1 || !point2) {
    this->ui->laengenEingabe->setText("");
    return;
  }

  QString text;
  auto totalDistance = point1->position().distanceToPoint(point2->position());
  if (unit == metres) {
    text += QString::number(
        round(totalDistance * pow(10, roundTo)) / pow(10, roundTo), 'f',
        roundTo);
    text += " metres";
  } else if (unit == decimetres) {
    totalDistance *= 10;
    text += QString::number(
        round(totalDistance * pow(10, roundTo)) / pow(10, roundTo), 'f',
        roundTo);
    text += " decimetres";
  } else if (unit == centimetres) {
    totalDistance *= 100;
    text += QString::number(
        round(totalDistance * pow(10, roundTo)) / pow(10, roundTo), 'f',
        roundTo);
    text += " centimetres";
  } else if (unit == millimetres) {
    totalDistance *= 1000;
    text += QString::number(
        round(totalDistance * pow(10, roundTo)) / pow(10, roundTo), 'f',
        roundTo);
    text += " millimetres";
  } else if (unit == micrometres) {
    totalDistance *= 1000000;
    text += QString::number(
        round(totalDistance * pow(10, roundTo)) / pow(10, roundTo), 'f',
        roundTo);
    text += " micrometres";
  }

  this->ui->laengenEingabe->setText(text);
}

void PointList::getPointNameList(QStringList& nameList, QList<quint64>& ids) {
  nameList.clear();
  ids.clear();

  auto data = gpo->geometricPrimitiveData();
  if (!data) return;

  auto allIds = data->getPrimitiveIDs();
  for (auto id : allIds) {
    auto p = data->getPrimitive(id);
    // Only consider points
    if (!qSharedPointerDynamicCast<GeometricPrimitivePoint>(p)) continue;

    nameList << p->name();
    ids << id;
  }
}

PlaneInfo PointList::createNewPlaneFromCoordinates(QVector3D one, QVector3D two,
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

void PointList::newPlaneFromCoordinates(QVector3D one, QVector3D two,
                                        QVector3D three) {
  auto newPlane = createNewPlaneFromCoordinates(one, two, three);
  auto planeProperty = createNode<PlaneNode>();
  // TODO: Pass properties to createNode() instead?
  planeProperty->setOrigin(newPlane.origin);
  planeProperty->setRotation(newPlane.rotation);
}

void PointList::reloadUIData() {
  auto currentPoint = this->gpo->properties->selectedPrimitive();

  fillUI();
  if (!currentPoint) {
    this->ui->lineLabel->setText("");
  } else {
    QString name = "";
    auto data = gpo->geometricPrimitiveData();
    if (data) {
      auto point = data->getPrimitiveOrNull(currentPoint);
      if (point) name = point->name();
    }

    this->ui->lineLabel->setText(name);
  }
}

// TODO: this is the wrong event, this is not triggered when the selected item
// is changed using the keyboard
void PointList::on_listWidget_clicked(const QModelIndex& index) {
  int idx = index.row();
  if (idx >= pointIdList.size()) return;
  this->gpo->properties->setSelectedPrimitive(pointIdList[idx]);
}
