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

#include <VoxieBackend/Data/PlaneInfo.hpp>

#include <QtWidgets/QComboBox>
#include <QtWidgets/QWidget>

namespace Ui {
class PointList;
}

namespace vx {
class GeometricPrimitiveNode;

/**
 * @brief The PointList class manages the PointList widget (see
 * pointlist3d.ui), which allows for geometric analysis in the Isosurfaceview.
 */
class VOXIECORESHARED_EXPORT PointList : public QWidget {
  Q_OBJECT

  GeometricPrimitiveNode* gpo;
  QStringList pointNameList;
  QList<quint64> pointIdList;
  bool suppressMeasurementPointChanged;

 public:
  explicit PointList(GeometricPrimitiveNode* gpo);
  ~PointList();

  enum Unit { metres, decimetres, centimetres, millimetres, micrometres };

 private:
  /**
   * @brief fillUI populates the Widget with the data (i.e. saved points) and
   * the geometric analysis data (distances, etc.)
   */
  void fillUI();
  /**
   * @brief fillUnitCombobox populates the ComboBox containing available units
   * to measure in. Only call this once in constructor!
   */
  void fillUnitCombobox();
  /**
   * @brief fillListWidget populates the listWidget with saved points
   */
  void fillListWidget();
  /**
   * @brief calculateDistance creates String with the distance between the two
   * selected points and the unit and displays it in the Widget.
   */
  void calculateDistance();
  /**
   * @brief getPointNameList generates a QList containing all the names of
   * points
   * @param nameList list to write into
   */
  void getPointNameList(QStringList& nameList, QList<quint64>& ids);

 public Q_SLOTS:
  /**
   * @brief reloadUIData repopulates the entire Widget, call this when
   * datapoints changes
   */
  void reloadUIData();
  /**
   * @brief newPlaneFromCoordinates creates a new slice from the three Points
   * with coordinates one, two and three. make sure, one, two and three are
   * different before calling this
   * @param one int
   * @param two int
   * @param three int
   */
  static void newPlaneFromCoordinates(QVector3D one, QVector3D two,
                                      QVector3D three);

  /**
   * @brief createNewPlaneFromCoordinates creates a new slice from the three
   * Points with coordinates one, two and three. make sure, one, two and three
   * are different before calling this.
   * @param one int
   * @param two int
   * @param three int
   * @return the newly created plane
   */
  static PlaneInfo createNewPlaneFromCoordinates(QVector3D one, QVector3D two,
                                                 QVector3D three);

 private Q_SLOTS:
  void on_lineLabel_textChanged(const QString& arg1);

  void on_deleteButton_pressed();

  void on_pointBox1_currentIndexChanged(int index);

  void on_pointBox2_currentIndexChanged(int index);

  void on_pushButton_clicked();

  void on_unitComboBox_currentIndexChanged(const QString& arg1);

  void on_lineFixed_returnPressed();

  void on_listWidget_clicked(const QModelIndex& index);

 private:
  Ui::PointList* ui;
  // for settings inside the widget
  Unit unit;
  int roundTo;
};
}  // namespace vx
