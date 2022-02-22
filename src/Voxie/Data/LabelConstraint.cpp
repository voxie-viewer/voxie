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

#include <Voxie/Data/LabelAttributes.hpp>
#include <Voxie/Data/LabelConstraint.hpp>

#include <QLabel>
#include <QPushButton>
#include <QStandardItem>
#include <QStandardItemModel>

using namespace vx;

LabelConstraint::LabelConstraint(int id) : QHBoxLayout() {
  this->id = id;
  this->attributeBox = new QComboBox();
  int index = 0;
  bool disabled = false;
  for (QString attributeTitel : LabelAttributes::getAttributeTitles()) {
    this->attributeBox->addItem(attributeTitel);
    QString itemText = this->attributeBox->itemText(index);
    QStandardItemModel* model =
        qobject_cast<QStandardItemModel*>(this->attributeBox->model());
    disabled = (itemText == "Bounding box" || itemText == "Center of mass" ||
                itemText == "Weighted center of mass");
    QStandardItem* item = model->item(index);
    item->setFlags(disabled ? item->flags() & ~Qt::ItemIsEnabled
                            : item->flags() | Qt::ItemIsEnabled);
    index += 1;
  }
  this->addWidget(this->attributeBox);

  this->operatorBox = new QComboBox();
  this->operatorBox->addItems({"=", "<", ">", "<=", ">="});
  this->addWidget(this->operatorBox);

  this->valueInput = new QInputDialog();
  this->valueInput->setInputMode(QInputDialog::DoubleInput);
  this->valueInput->setOption(QInputDialog::NoButtons);
  this->valueInput->setDoubleMinimum(0);
  this->valueInput->setDoubleMaximum(std::numeric_limits<double>::max());
  connect(valueInput, &QInputDialog::doubleValueChanged, this,
          &LabelConstraint::setValue);
  this->addWidget(this->valueInput);

  QPushButton* deleteButton = new QPushButton();
  deleteButton->setIcon(QIcon(":/icons/cross.png"));
  connect(deleteButton, &QPushButton::released, this,
          &LabelConstraint::removeConstraint);
  this->addWidget(deleteButton);
}

void LabelConstraint::removeConstraint() {
  Q_EMIT this->constrainDeleted(this->id);
  this->~LabelConstraint();
}

void LabelConstraint::setValue(double value) { this->value = value; }
