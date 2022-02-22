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
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

namespace vx {

/**
 * The PropertySection Class implements the PropertySections of
 * FilterObjects. It implements a Prototype to create Default Property Items.
 * @brief The PropertySection class
 */
class VOXIECORESHARED_EXPORT PropertySection : public QWidget {
  Q_OBJECT
 private:
  QVBoxLayout* layout;

 public:
  explicit PropertySection(QString title = "Properties");

  /**
   * @param property the Property Item is added to the the Property Section. Can
   * be an QWidget.
   */
  void addProperty(QWidget* property);

  /**
   * @brief Adds an Text Value Property.
   * @param label Text to display in the GUI, describing the input field
   * @return Returns the generated QInputDialog. &QInputDialog::textValueChanged
   * is the signal for changed input.
   */
  QInputDialog* addTextProperty(QString label);

  /**
   * @brief Adds an Numeric Value Property. This can be Float or Integer.
   * @param label Text to display in the GUI, describing the input field
   * @param isFloat Defines wheter the input field can be a float
   * @return Returns the generated QInputDialog. &QInputDialog::intValueChanged
   * is the signal for changed input.
   */
  QInputDialog* addNumericProperty(QString label, bool isFloat, double min,
                                   double max, double stepSize);

  /**
   * @brief Adds an ComboBox Value Property.
   * @param items List of items to display in the GUI
   * @return Returns the generated QComboBox. &QComboBox::currentTextChanged is
   * the signal for changed input.
   */
  QComboBox* addComboBoxProperty(const QStringList& items);

  /**
   * @brief addHBoxLayout a QHBoxLayout to the layout
   * @param layout the QHBoxLayout that is added to the layout
   */
  void addHBoxLayout(QHBoxLayout* layout);
};

}  // namespace vx
