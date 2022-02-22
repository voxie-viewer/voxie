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

#include <PluginVis3D/Grid3D.hpp>
#include <QWidget>

namespace Ui {
class Grid3DWidget;
}

/**
 * @brief This class handle all requests from and for the GUI of the grid.
 */
class Grid3DWidget : public QWidget {
  Q_OBJECT

 public:
  explicit Grid3DWidget(QWidget* parent = 0, Grid3D* grid = 0);
  ~Grid3DWidget();

  /**
   * @brief Writes the current size and unit of length into the LineEdit
   */
  void updateLineEdit();

 private Q_SLOTS:
  /**
   * @brief on_checkBox_clicked is an listener for the UI element checkBox and
   * is called if clicked on it.
   * @param bool checked
   */
  void on_checkBox_clicked(bool checked);

  /**
   * @brief on_colorButton_clicked is an listener for the UI element colorButton
   * and is called if clicked on it.
   */
  void on_colorButton_clicked();

  /**
   * @brief on_comboBox_currentTextChanged is an listener for the UI element
   * comboBox and is called if the current mode has changed.
   * @param  const QString &mode
   */
  void on_comboBox_currentTextChanged(const QString& mode);

  /**
   * @brief on_gridMeshWidth_editingFinished is an listener for the UI element
   * gridMeshWidth and is called if editing the LineEdit is finished.
   */
  void on_gridMeshWidth_editingFinished();

  /**
   * @brief on_opacitySlider_valueChanged is an listener for the UI element
   * opacitySlider and is called if the value of the slider has changed.
   * @param value of the slider
   */
  void on_opacitySlider_valueChanged(int value);

  /**
   * @brief on_XY_clicked is an listener for checkbox XY and is called if XY is
   * clicked.
   * @param bool checked
   */
  void on_XY_clicked(bool checked);

  /**
   * @brief on_XZ_clicked is an listener for checkbox XZ and is called if XZ is
   * clicked.
   * @param bool checked
   */
  void on_XZ_clicked(bool checked);

  /**
   * @brief on_YZ_clicked is an listener for checkbox YZ and is called if YZ is
   * clicked.
   * @param bool checked
   */
  void on_YZ_clicked(bool checked);

  /**
   * @brief Is called if the unit of entered grid Size has changed. This method
   * fetches the current grid mesh size and its length unit and writes it into
   * the text field of the GUI.
   * @param arg1
   */
  void stringToUnit(const QString& unit);

 private:
  // data
  Ui::Grid3DWidget* ui;
  Grid3D* grid;
};
