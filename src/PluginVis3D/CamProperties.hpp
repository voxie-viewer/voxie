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

#include <QPair>
#include <QQuaternion>
#include <QWidget>
#include <cmath>

#include <Voxie/Data/VolumeNode.hpp>

namespace Ui {
class CamProperties;
}

/**
 * @brief This class mediates between the GUI of the camera and the rest of the
 * logic. Visualizers that use this class must match the signals and slots
 * accordingly.
 */
class CamProperties : public QWidget {
  Q_OBJECT

 public:
  /**
   * @brief Constructor of CamProperties
   * @param QWidget* parent = 0
   */
  explicit CamProperties(QWidget* parent = 0);

  /**
   * @brief Destructor of CamProperties
   */
  ~CamProperties();

  /**
   * @brief This method emit request signals for the initiation of this class.
   */
  void init();

 public Q_SLOTS:

  /**
   * @brief Standard setter for the object rotation which can be connectet as a
   * Slot.
   * @param QQuaternion rot
   */
  void setRotation(QQuaternion rot);

  /**
   * @brief setZoom is a setter for the zoom.
   * @param float zoomMin
   * @param float zoomMax
   */
  void setZoom(float, float zoomMin, float zoomMax);

 private Q_SLOTS:

  /**
   * @brief on_rotationModeComboBox_currentTextChanged is an listener for the UI
   * element rotationModeComboBox and is called if current mode changed
   */
  void on_rotationModeComboBox_currentTextChanged(const QString& mode);

  // Textfield trigger
  void on_rotationWLineEdit_textChanged(const QString& text);
  void on_rotationXLineEdit_textChanged(const QString& text);
  void on_rotationYLineEdit_textChanged(const QString& text);
  void on_rotationZLineEdit_textChanged(const QString& text);
  void on_zoomLineEdit_editingFinished();

  void on_copyButton_pressed();

  void on_pasteButton_pressed();

 Q_SIGNALS:

  /**
   * @brief zoomRequest is a signal which is called if the zoom need to be
   * request.
   */
  void zoomRequest();

  /**
   * @brief rotationRequest is a signal which is called if the rotation need to
   * be request.
   */
  void rotationRequest();

  /**
   * @brief zoomChanged is a signal which is called if the camera zoom has
   * changed.
   */
  void zoomChanged(float);

  /**
   * @brief rotationChanged is a signal which is called if the object rozation
   * has changed.
   */
  void rotationChanged(QQuaternion);

 private:
  // data
  Ui::CamProperties* ui;
  bool ignoreUpdates = false;
  float zoomMin, zoomMax, zoomCurrent;
  QString mode;
  QQuaternion cameraRotation;

  /**
   * @brief isStringValidFloat tests if an QString contains an valis float
   * number.
   * @param text is an QString which is to be tested.
   * @return true if the given QString represents an valid float number.
   * Otherwise its false.
   */
  bool getFloat(const QString text, float* value);
  /**
   * @brief getWRotation extract the current content of the UI element
   * rotationWLineEdit.
   * @return a float which represent the current content of the UI element
   * rotationWLineEdit.
   */
  float getWRotation();

  /**
   * @brief getXRotation extract the current content of the UI element
   * rotationXLineEdit.
   * @return a float which represent the current content of the UI element
   * rotationXLineEdit.
   */
  float getXRotation();

  /**
   * @brief getYRotation extract the current content of the UI element
   * rotationYLineEdit.
   * @return a float which represent the current content of the UI element
   * rotationYLineEdit.
   */
  float getYRotation();

  /**
   * @brief getZRotation extract the current content of the UI element
   * rotationZLineEdit.
   * @return a float which represent the current content of the UI element
   * rotationZLineEdit.
   */
  float getZRotation();

  /**
   * @brief updateRotationLineEdits updates the RotationLineEdits with the
   * current this->objectRotation.
   */
  void updateLineEdits();
};
