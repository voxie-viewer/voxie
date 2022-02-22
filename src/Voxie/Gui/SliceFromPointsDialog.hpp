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

#include <QtCore/QStringListModel>

#include <QtWidgets/QDialog>
#include <QtWidgets/QErrorMessage>

namespace Ui {
class SliceFromPointsDialog;
}

namespace vx {
class GeometricPrimitive;
class GeometricPrimitiveData;

/**
 * @brief The SliceFromPointsDialog class This class contains the implementation
 * of the slicefrompoints dialog (see slicefrompointsdialog.ui)
 */
class VOXIECORESHARED_EXPORT SliceFromPointsDialog : public QDialog {
  Q_OBJECT

 public:
  explicit SliceFromPointsDialog(bool haveChangeSliceButton = false);
  ~SliceFromPointsDialog();

  void setPoints(const QSharedPointer<GeometricPrimitiveData>& points);

 private:
  /**
   * @brief fillComboBoxes populates the ComboBoxes with the names in points
   */
  void fillComboBoxes();

 private Q_SLOTS:
  /**
   * @brief on_comboBoxFirst_currentIndexChanged sets first to index. Called
   * when index of the first comboBox changes
   * @param index int
   */
  void on_comboBoxFirst_currentIndexChanged(int index);

  /**
   * @brief on_comboBoxSecond_currentIndexChanged sets second to index. Called
   * when index of the second comboBox changes
   * @param index int
   */
  void on_comboBoxSecond_currentIndexChanged(int index);

  /**
   * @brief on_comboBoxThird_currentIndexChanged sets third to index. Called
   * when index of the third comboBox changes
   * @param index int
   */
  void on_comboBoxThird_currentIndexChanged(int index);

  /**
   * @brief on_newSliceButton_clicked emits signal createNewSlice if first,
   * second and third are all different
   */
  void on_newSliceButton_clicked();

  void on_thisSliceButton_clicked();

  /**
   * @brief on_cancelButton_clicked closes the dialog
   */
  void on_cancelButton_clicked();

 Q_SIGNALS:
  /**
   * @brief createNewPlane emited when first, second and third are all different
   * and the newPlane button was clicked
   */
  void createNewPlane(const QVector3D& one, const QVector3D& two,
                      const QVector3D& three);
  void changeSlice(const QVector3D& one, const QVector3D& two,
                   const QVector3D& three);

 private:
  Ui::SliceFromPointsDialog* ui;
  QMap<quint64, QSharedPointer<GeometricPrimitive>> points;
  quint64 first, second, third;
  QErrorMessage error;
  QList<quint64> idList;
};
}  // namespace vx
