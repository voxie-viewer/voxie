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

#include <PluginVis3D/Data/PlaneData.hpp>
#include <PluginVis3D/Visualizer3DView.hpp>

#include <QWidget>

#include <QVBoxLayout>

namespace Ui {
class PlanePropertiesUi;
}

// TODO: remove
/**
 * @brief This class holds the GUI elements of the plane property section and
 * the logicial part of the 3D Visualizer
 */
class PlanePropertiesUi : public QWidget {
  Q_OBJECT

 public:
  explicit PlanePropertiesUi(Visualizer3DView* view, QWidget* parent = 0);
  ~PlanePropertiesUi();

  /**
   * @brief update Updates the View depending on the currently selected
   * PlaneData and the VolumeNode List.
   */
  void update();

 private Q_SLOTS:
  void on_hidePartsBox_currentIndexChanged(int index);

  void on_hidePartsSpinBox_valueChanged(int value);

 private:
  Ui::PlanePropertiesUi* ui;
  Visualizer3DView* view;

  bool ignoreChanges = false;
};
