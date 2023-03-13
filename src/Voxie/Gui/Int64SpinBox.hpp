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

#pragma once

#include <QtWidgets/QAbstractSpinBox>

// TODO: Add unsigned version?

namespace vx {
class Int64SpinBox : public QAbstractSpinBox {
  Q_OBJECT

  qint64 minimum_;
  qint64 maximum_;
  qint64 value_;

  void onEditFinished(const QString& s);

 public:
  Int64SpinBox(QWidget* parent = nullptr);
  ~Int64SpinBox();

  void setRange(qint64 minimum, qint64 maximum);
  void setValue(qint64 value);

  qint64 value() const { return value_; }

  void stepBy(int steps) override;

 protected:
  QValidator::State validate(QString& input, int& pos) const override;

  QAbstractSpinBox::StepEnabled stepEnabled() const override;

  virtual qlonglong valueFromText(const QString& text) const;

  virtual QString textFromValue(qlonglong val) const;

 Q_SIGNALS:
  void valueChanged(qint64 i);
  /*
  QValidator::State validate(QString& text, int& pos) const override;
  void fixup(QString& input) const override;

  QString textFromValue(double value) const override;
  double valueFromText(const QString& text) const override;

  void stepBy(int steps) override;
  */
};
}  // namespace vx
