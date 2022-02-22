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

#ifndef LIGHTSOURCEPROPERTIES_HPP
#define LIGHTSOURCEPROPERTIES_HPP

#include <QQuaternion>
#include <QVector3D>
#include <QWidget>

#include <QLineEdit>
#include <QSlider>

#include <PluginVis3D/LightSource.hpp>
#include <PluginVis3D/LightSourceListElement.hpp>

namespace Ui {
class LightSourceProperties;
}

/**
 * @brief This class mediates between the GUI of the light source properties and
 * the rest of the logic. Visualizers that use this class must match the signals
 * and slots accordingly.
 */
class LightSourceProperties : public QWidget {
  Q_OBJECT

 public:
  explicit LightSourceProperties(QWidget* parent = 0);
  ~LightSourceProperties();

 Q_SIGNALS:
  void positionRequest();
  void lightColorRequest();
  void activeRequest();

  void ambientlightScaleChanged(int value);
  void lightsourcesScaleChanged(int value);
  void useAbsShadingValueChanged(bool value);

  void ambientLightRequestResponse(QColor ambientLight);
  void lightSourcesListRequestResponse(QList<LightSource*>* lightSourcesList);

  void updateViewRequest();

 private Q_SLOTS:

  void positionRequestResponsed(QVector4D pos);
  void lightSourceColorRequestResponsed(QColor color);
  void activeRequestResponsed(bool active);

  void on_ambientlightSlider_valueChanged(int value);
  void on_diffuselightSlider_valueChanged(int value);

  void on_ambientlightLineEdit_editingFinished();

  void on_diffuselightLineEdit_editingFinished();

  void on_useAbsShadingValueCheckBox_clicked(bool checked);

 public Q_SLOTS:
  void ambientLightRequested();
  void lightSourcesListRequested();

  void updateViewRequested();

  // #### GUI-Interactions ####
  void on_addButton_clicked();
  void on_ambientColorButton_clicked();

 private:
  // data
  Ui::LightSourceProperties* ui;
  QList<LightSource*> lightSourceList;

  static void setButtonColor(QPushButton* colButt, QColor color);

  QVector4D position;
  QColor lightSourceColor;
  bool active = false;

  QColor ambientLightColor = QColor(255, 255, 105);
  QPushButton* ambientColorButton = nullptr;

  QSlider* ambientlightSlider = nullptr;
  QLineEdit* ambientlightLineEdit = nullptr;

  QSlider* diffuselightSlider = nullptr;
  QLineEdit* diffuselightLineEdit = nullptr;
  QCheckBox* useAbsShadingValueCheckBox = nullptr;

  LightSourceListElement* propertieslightSourceElement = nullptr;
  LightSource* propertiesLightSource = nullptr;
};

#endif  // LIGHTSOURCEPROPERTIES_HPP
