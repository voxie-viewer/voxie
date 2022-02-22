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

#include "SliceFromPointsDialog.hpp"

#include "ui_SliceFromPointsDialog.h"

#include <VoxieBackend/Data/GeometricPrimitive.hpp>
#include <VoxieBackend/Data/GeometricPrimitiveData.hpp>

using namespace vx;

SliceFromPointsDialog::SliceFromPointsDialog(bool haveChangeSliceButton)
    : QDialog(), ui(new Ui::SliceFromPointsDialog) {
  ui->setupUi(this);

  if (!haveChangeSliceButton) this->ui->thisSliceButton->setVisible(false);
}

SliceFromPointsDialog::~SliceFromPointsDialog() { delete ui; }

void SliceFromPointsDialog::on_comboBoxFirst_currentIndexChanged(int index) {
  if (index < 0 || index >= idList.size()) {
    first = 0;
  } else {
    first = idList[index];
  }
}

void SliceFromPointsDialog::on_comboBoxSecond_currentIndexChanged(int index) {
  if (index < 0 || index >= idList.size()) {
    second = 0;
  } else {
    second = idList[index];
  }
}

void SliceFromPointsDialog::on_comboBoxThird_currentIndexChanged(int index) {
  if (index < 0 || index >= idList.size()) {
    third = 0;
  } else {
    third = idList[index];
  }
}

void SliceFromPointsDialog::on_newSliceButton_clicked() {
  if (!first || !second || !third) {
    error.setModal(true);
    error.showMessage("All three points have to be selected");
  } else if (first == second || second == third || first == third) {
    error.setModal(true);
    error.showMessage("All three points have to be different");
  } else {
    auto firstP =
        qSharedPointerDynamicCast<GeometricPrimitivePoint>(points[first]);
    auto secondP =
        qSharedPointerDynamicCast<GeometricPrimitivePoint>(points[second]);
    auto thirdP =
        qSharedPointerDynamicCast<GeometricPrimitivePoint>(points[third]);
    if (!firstP || !secondP || !thirdP)  // Should never happen
      return;
    Q_EMIT this->createNewPlane(firstP->position(), secondP->position(),
                                thirdP->position());
    this->close();
  }
}

void SliceFromPointsDialog::on_thisSliceButton_clicked() {
  if (!first || !second || !third) {
    error.setModal(true);
    error.showMessage("All three points have to be selected");
  } else if (first == second || second == third || first == third) {
    error.setModal(true);
    error.showMessage("All three points have to be different");
  } else {
    auto firstP =
        qSharedPointerDynamicCast<GeometricPrimitivePoint>(points[first]);
    auto secondP =
        qSharedPointerDynamicCast<GeometricPrimitivePoint>(points[second]);
    auto thirdP =
        qSharedPointerDynamicCast<GeometricPrimitivePoint>(points[third]);
    if (!firstP || !secondP || !thirdP)  // Should never happen
      return;
    Q_EMIT this->changeSlice(firstP->position(), secondP->position(),
                             thirdP->position());
    this->close();
  }
}

void SliceFromPointsDialog::on_cancelButton_clicked() { this->close(); }

void SliceFromPointsDialog::fillComboBoxes() {
  QStringListModel* modelCOne = new QStringListModel(this);
  QStringListModel* modelCTwo = new QStringListModel(this);
  QStringListModel* modelCThree = new QStringListModel(this);
  QStringList nameList;
  idList.clear();
  for (const auto& id : points.keys()) {
    auto point = qSharedPointerDynamicCast<GeometricPrimitivePoint>(points[id]);
    if (!point) continue;
    idList << id;
    nameList << point->name();
  }
  ui->comboBoxFirst->clear();
  ui->comboBoxSecond->clear();
  ui->comboBoxThird->clear();
  modelCOne->setStringList(nameList);
  modelCTwo->setStringList(nameList);
  modelCThree->setStringList(nameList);
  ui->comboBoxFirst->setModel(modelCOne);
  ui->comboBoxSecond->setModel(modelCTwo);
  ui->comboBoxThird->setModel(modelCThree);
}

void SliceFromPointsDialog::setPoints(
    const QSharedPointer<GeometricPrimitiveData>& points) {
  this->points = points->primitives();
  fillComboBoxes();
}
