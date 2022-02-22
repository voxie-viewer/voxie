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

#include <QDoubleValidator>
#include <QLabel>
#include <QLineEdit>
#include <QVector3D>

namespace vx {
namespace internal {

class DoubleLineEdit : public QLineEdit {
 public:
  DoubleLineEdit(QWidget* parent = 0) : QLineEdit(parent) {
    this->setValidator(new QDoubleValidator(this));
  }
};
}  // namespace internal

/**
 * @brief The QuaternionWidget class can be used to display three qlineedits in
 * one row.
 */
class QuaternionWidget : public QWidget {
  Q_OBJECT
 public:
  explicit QuaternionWidget(QLabel* label, QWidget* parent = 0);

  /**
   * @brief Returns a QVector3D with the entered values.
   * @return
   */
  QVector3D getValues();

  /**
   * @brief Returns true, when all three fields are not filled, otherwise false.
   * @return
   */
  bool allEmpty();

  /**
   * @brief Clears all three fields.
   */
  void clearAll();

  /**
   * @brief Updates the values of all three fields to the given value.
   * @param values
   */
  void updateAllValues(QVector3D values);

 Q_SIGNALS:
  void changed();

 private:
  internal::DoubleLineEdit* x = new internal::DoubleLineEdit;
  internal::DoubleLineEdit* y = new internal::DoubleLineEdit;
  internal::DoubleLineEdit* z = new internal::DoubleLineEdit;
};
}  // namespace vx
