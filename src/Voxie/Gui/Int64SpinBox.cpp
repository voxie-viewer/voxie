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

#include "Int64SpinBox.hpp"

#include <QtCore/QDebug>

#include <QtWidgets/QLineEdit>

#include <cmath>

using namespace vx;

Int64SpinBox::Int64SpinBox(QWidget* parent)
    : QAbstractSpinBox(parent), minimum_(0), maximum_(99), value_(0) {
  this->lineEdit()->setText("0");

  connect(this->lineEdit(), &QLineEdit::textEdited, this,
          &Int64SpinBox::onEditFinished);
}
Int64SpinBox::~Int64SpinBox() {}

void Int64SpinBox::setRange(qint64 minimum, qint64 maximum) {
  this->minimum_ = minimum;
  this->maximum_ = maximum;
}

void Int64SpinBox::setValue(qint64 value) {
  // qDebug() << "Int64SpinBox::setValue" << value;

  if (value_ != value) {
    this->lineEdit()->setText(textFromValue(value));
    value_ = value;
    Q_EMIT valueChanged(value_);
  }
}

void Int64SpinBox::onEditFinished(const QString& value) {
  // qDebug() << "Int64SpinBox::onEditFinished" << value;

  QString input = value;

  int pos = 0;
  if (QValidator::Acceptable == validate(input, pos)) {
    value_ = valueFromText(input);
    Q_EMIT valueChanged(value_);
  }
}

QValidator::State Int64SpinBox::validate(QString& text, int& pos) const {
  // qDebug() << "Int64SpinBox::validate" << text << pos;

  (void)pos;

  if (text == "" || (minimum_ < 0 && text == "-"))
    return QValidator::Intermediate;

  bool ok;
  qlonglong val = text.toLongLong(&ok);
  if (!ok) return QValidator::Invalid;

  if (val < minimum_ || val > maximum_) return QValidator::Invalid;

  return QValidator::Acceptable;
}

QString Int64SpinBox::textFromValue(qint64 value) const {
  return QString::number(value);
}

qint64 Int64SpinBox::valueFromText(const QString& text) const {
  return text.toLongLong();
}

QAbstractSpinBox::StepEnabled Int64SpinBox::stepEnabled() const {
  auto res = QAbstractSpinBox::StepEnabled();
  if (value() > minimum_) res |= StepDownEnabled;
  if (value() < maximum_) res |= StepUpEnabled;
  return res;
}

void Int64SpinBox::stepBy(int steps) {
  // qDebug() << "Int64SpinBox::stepBy" << steps;

  auto newValue = this->value();
  if (steps < 0 && std::numeric_limits<qint64>::min() - steps > newValue) {
    newValue = std::numeric_limits<qint64>::min();
  } else if (steps > 0 &&
             std::numeric_limits<qint64>::max() - steps < newValue) {
    newValue = std::numeric_limits<qint64>::max();
  } else {
    newValue += steps;
  }

  if (newValue > maximum_) newValue = maximum_;
  if (newValue < minimum_) newValue = minimum_;

  setValue(newValue);
}
