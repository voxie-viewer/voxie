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

#include "PropertySection.hpp"

using namespace vx;

PropertySection::PropertySection(QString title) : QWidget() {
  this->setWindowTitle(title);
  this->layout = new QVBoxLayout();
  this->layout->setContentsMargins(0, 0, 0, 0);
  this->setLayout(layout);
}

void PropertySection::addProperty(QWidget* property) {
  this->layout->addWidget(property);
}

QInputDialog* PropertySection::addTextProperty(QString label) {
  QInputDialog* inputDialog = new QInputDialog();
  inputDialog->setInputMode(QInputDialog::TextInput);
  inputDialog->setLabelText(label + ":");
  inputDialog->setOption(QInputDialog::NoButtons);

  this->addProperty(inputDialog);
  return inputDialog;
}

QInputDialog* PropertySection::addNumericProperty(QString label, bool isFloat,
                                                  double min, double max,
                                                  double stepSize) {
  QInputDialog* inputDialog = new QInputDialog();
  if (isFloat) {
    inputDialog->setInputMode(QInputDialog::DoubleInput);
    inputDialog->setDoubleRange(min, max);
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    // Introduced in Qt 5.10:
    // http://doc.qt.io/qt-5/qinputdialog.html#doubleStep-prop
    inputDialog->setDoubleStep(stepSize);
#endif
  } else {
    inputDialog->setInputMode(QInputDialog::IntInput);
    inputDialog->setIntRange(min, max);
    inputDialog->setIntStep(stepSize);
  }
  inputDialog->setLabelText(label + ":");
  inputDialog->setOption(QInputDialog::NoButtons);

  this->addProperty(inputDialog);
  return inputDialog;
}

QComboBox* PropertySection::addComboBoxProperty(const QStringList& items) {
  QComboBox* inputDialog = new QComboBox();
  inputDialog->addItems(items);

  this->addProperty(inputDialog);
  return inputDialog;
}

void PropertySection::addHBoxLayout(QHBoxLayout* layout) {
  this->layout->addLayout(layout);
}
