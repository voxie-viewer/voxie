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

#include "PlanePropertiesUi.hpp"
#include "ui_PlanePropertiesUi.h"

PlanePropertiesUi::PlanePropertiesUi(Visualizer3DView* view, QWidget* parent)
    : QWidget(parent), ui(new Ui::PlanePropertiesUi), view(view) {
  ui->setupUi(this);
  update();
}

PlanePropertiesUi::~PlanePropertiesUi() { delete ui; }

void PlanePropertiesUi::update() {
  this->ignoreChanges = true;

  this->setEnabled(true);
  this->ui->hidePartsBox->setEnabled(true);
  this->ui->hidePartsSpinBox->setEnabled(true);

  int index = -1;
  switch (this->view->cuttingMode.mode()) {
    case CuttingMode::AtLeast:
      index = 0;
      break;
    case CuttingMode::AllBut:
      index = 1;
      break;
  }
  this->ui->hidePartsBox->setCurrentIndex(index);
  this->ui->hidePartsSpinBox->setValue(this->view->cuttingMode.limit());

  this->ignoreChanges = false;
}

void PlanePropertiesUi::on_hidePartsBox_currentIndexChanged(int index) {
  if (this->ignoreChanges) {
    return;
  }

  CuttingMode cuttingMode;
  switch (index) {
    case 1:
      cuttingMode = CuttingMode::AllBut;
      break;
    default:
      cuttingMode = CuttingMode::AtLeast;
      break;
  }
  this->view->cuttingMode.setMode(cuttingMode);
}

void PlanePropertiesUi::on_hidePartsSpinBox_valueChanged(int value) {
  if (this->ignoreChanges) {
    return;
  }

  this->view->cuttingMode.setLimit(value);
}
