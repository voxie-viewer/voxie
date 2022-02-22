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

#include <QQuaternion>
#include <QVector3D>
#include <QWidget>

#include <QLineEdit>

#include <Voxie/Data/PositionInterface.hpp>

namespace Ui {
class ObjectProperties;
}

/**
 * @brief This class mediates between the GUI of the object properties and the
 * rest of the logic. Visualizers that use this class must match the signals and
 * slots accordingly.
 */
class VOXIECORESHARED_EXPORT ObjectProperties : public QWidget {
  Q_OBJECT
  typedef vx::PositionInterface PositionInterface;

 public:
  /**
   * @brief This is the standard constructor for the class ObjectProperties.
   * @param parent
   */
  explicit ObjectProperties(QWidget* parent = 0, bool showPosition = true,
                            bool showRotation = true);

  /**
   * @brief This is the standard destructor for the class ObjectProperties.
   * @param parent
   */
  ~ObjectProperties();

  /**
   * @brief This method emit request signals for the initiation of this class.
   */
  void init();

  /**
   * @brief Standart getter for the object position.
   * @return a QVector3D which represents the object position.
   */
  QVector3D getPosition();

  /**
   * @brief Standart getter for the object rotation.
   * @return a QQuaternion which represents the object rotation.
   */
  QQuaternion getRotation();

  /**
   * @brief Set the position to this value
   */
  void setPosition(const QVector3D& position);

  /**
   * @brief Set the rotation to this value
   */
  void setRotation(const QQuaternion& position);

 public Q_SLOTS:

  /**
   * @brief Standard setter for the object position which can be connectet as a
   * Slot.
   * @param dataSet is unused. Needed for connect reasons.
   * @param pos is an QVector3D which represents the new position of the current
   * object.
   */
  void setPosition(PositionInterface* selectedPositionInterface);

  /**
   * @brief Standard setter for the object rotation which can be connectet as a
   * Slot.
   * @param dataSet is unused. Needed for connect reasons.
   * @param rot is an QQuaternion which represents the new rotation of the
   * current object.
   */
  void setRotation(PositionInterface* selectedPositionInterface);

 private Q_SLOTS:

  /**
   * @brief on_rotationModeComboBox_editingFinished is an listener for the UI
   * element rotationModeComboBox and is called if editing of the LineEdit is
   * finished.
   */
  void on_rotationModeComboBox_currentTextChanged(const QString& mode);

  void on_positionXLineEdit_textChanged(const QString& text);

  void on_positionYLineEdit_textChanged(const QString& text);

  void on_positionZLineEdit_textChanged(const QString& text);

  void on_rotationWLineEdit_textChanged(const QString& text);

  void on_rotationXLineEdit_textChanged(const QString& text);

  void on_rotationYLineEdit_textChanged(const QString& text);

  void on_rotationZLineEdit_textChanged(const QString& text);

 Q_SIGNALS:

  /**
   * @brief positionRequest is a signal which is called if the position need to
   * be request.
   */
  void positionRequest();

  /**
   * @brief rotationRequest is a signal which is called if the rotation need to
   * be request.
   */
  void rotationRequest();

  /**
   * @brief positionChanged is a signal which is called if the object position
   * has changed.
   */
  void positionChanged(QVector3D, bool);

  /**
   * @brief rotationChanged is a signal which is called if the object rozation
   * has changed.
   */
  void rotationChanged(QQuaternion, bool);

 private:
  // data
  bool ignoreUpdates = false;
  Ui::ObjectProperties* ui;
  QString mode;
  QQuaternion objectRotation;
  QVector3D objectPosition;

  /**
   * @brief isStringValidFloat tests if the given QLineEdit box contains a valid
   * float and returns the value on success.
   * @param exitBox The QLineEdit box that holds the value.
   * @param value Holds the value if the content is a valid float.
   * @return true if the value was successfully converted, false otherwise.
   */
  bool getFloat(const QString text, float* value);

  /**
   * @brief getXPosition extract the current content of the UI element
   * positionXLineEdit.
   * @return a float which represent the current content of the UI element
   * positionXLineEdit.
   */
  float getXPosition();

  /**
   * @brief getYPosition extract the current content of the UI element
   * positionYLineEdit.
   * @return a float which represent the current content of the UI element
   * positionYLineEdit.
   */
  float getYPosition();

  /**
   * @brief geZPosition extract the current content of the UI element
   * positionZLineEdit.
   * @return a float which represent the current content of the UI element
   * positionZLineEdit.
   */
  float getZPosition();

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
};
