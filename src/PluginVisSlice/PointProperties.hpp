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

#include <QWidget>

#include <PluginVisSlice/SliceVisualizer.hpp>

#include <Voxie/IVoxie.hpp>

#include <QStringListModel>

namespace vx {
struct PlaneInfo;
}

namespace Ui {
class PointProperties;
}

class PointProperties : public QWidget {
  Q_OBJECT
 public:
  explicit PointProperties(QWidget* parent = 0, SliceVisualizer* sv = 0);
  ~PointProperties();

 private Q_SLOTS:
  void on_lineEdit_returnPressed();

  void on_pushButton_clicked();

  void on_pushButton_2_clicked();

 public Q_SLOTS:
  /**
   * @brief changePlane is used when a the current plane is to be changed to
   * contain the three points with indices one, two and three in the current
   * Pointlist
   * @param one
   * @param two
   * @param three
   */
  void changePlane(QVector3D one, QVector3D two, QVector3D three);

 Q_SIGNALS:
  /**
   * @brief newVisibility emited when a valid visibility is received from the
   * user
   * @param visibility the new visibility
   */
  void newVisibility(float visibility);

 private:
  Ui::PointProperties* ui;
  SliceVisualizer* sv;

  // for settings
  QString latestUnit;

  static void newPlaneFromCoordinates(QVector3D one, QVector3D two,
                                      QVector3D three);
  static vx::PlaneInfo createNewPlaneFromCoordinates(QVector3D one,
                                                     QVector3D two,
                                                     QVector3D three);
};
