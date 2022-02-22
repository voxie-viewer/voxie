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

#include "QuaternionWidget.hpp"

#include <QHBoxLayout>

using namespace vx;

QuaternionWidget::QuaternionWidget(QLabel* label, QWidget* parent)
    : QWidget(parent) {
  auto layout = new QHBoxLayout();
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(label);
  layout->addWidget(x);
  layout->addWidget(y);
  layout->addWidget(z);
  this->setLayout(layout);

  this->x->connect(x, &QLineEdit::textChanged, this, [&](QString param) {
    (void)param;
    Q_EMIT this->changed();
  });
  this->y->connect(y, &QLineEdit::textChanged, this, [&](QString param) {
    (void)param;
    Q_EMIT this->changed();
  });
  this->z->connect(z, &QLineEdit::textChanged, this, [&](QString param) {
    (void)param;
    Q_EMIT this->changed();
  });
}

QVector3D QuaternionWidget::getValues() {
  return QVector3D(this->x->text().toFloat(), this->y->text().toFloat(),
                   this->z->text().toFloat());
}

bool QuaternionWidget::allEmpty() {
  return this->x->text().isEmpty() && this->y->text().isEmpty() &&
         this->z->text().isEmpty();
}

void QuaternionWidget::clearAll() {
  this->x->clear();
  this->y->clear();
  this->z->clear();
}

void QuaternionWidget::updateAllValues(QVector3D values) {
  this->x->setText(QString::number(values.x()));
  this->y->setText(QString::number(values.y()));
  this->z->setText(QString::number(values.z()));
}
