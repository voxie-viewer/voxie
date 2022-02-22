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

#include <Voxie/Voxie.hpp>

#include <QComboBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QObject>

namespace vx {

/**
 * @brief The LabelConstraint class defines a horizontal layout with the three
 * inputs: attribute, operator and value.
 * This inputs defines the constraints for the label selection.
 */
class VOXIECORESHARED_EXPORT LabelConstraint : public QHBoxLayout {
  Q_OBJECT

 public:
  LabelConstraint(int id);
  int getID() { return this->id; }

  QComboBox* attributeBox;
  QComboBox* operatorBox;
  double value;

 private:
  int id;
  QInputDialog* valueInput;

  /**
   * @brief removeConstraint signals that this constraint will be removed and
   * removed this contstraint afterwards.
   */
  void removeConstraint();
  void setValue(double value);

 Q_SIGNALS:
  void constrainDeleted(int id);
};
}  // namespace vx
