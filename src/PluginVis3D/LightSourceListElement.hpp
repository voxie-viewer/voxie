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

#ifndef LIGHTSOURCELISTELEMENT_HPP
#define LIGHTSOURCELISTELEMENT_HPP

#include <QCheckBox>
#include <QColor>
#include <QColorDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QVector4D>
#include <QWidget>

namespace Ui {
class LightSourceListElement;
}

class LightSourceListElement : public QWidget {
  Q_OBJECT

 public:
  explicit LightSourceListElement(QWidget* parent = 0);
  ~LightSourceListElement();

  void init();

 private:
  Ui::LightSourceListElement* ui;

  //#### GUI-Elements ####
  QLineEdit* xPosBox;
  QLineEdit* yPosBox;
  QLineEdit* zPosBox;
  QLineEdit* wDistanceBox;
  QPushButton* colButt;
  QCheckBox* checkBox;

  QVector4D position;
  QColor lightColor;
  bool active;

  bool ignoreUpdates;

  void removeGUI_Element_Pointer();
  void setButtonColor(QPushButton* colButt, QColor color);

 Q_SIGNALS:
  void positionRequest();
  void lightColorRequest();
  void activeRequest();

  void positionChanged(QVector4D pos);
  void lightColorChanged(QColor col);
  void activeChanged(bool active);

  void updateViewRequest();

 public Q_SLOTS:
  void positionRequestResponsed(QVector4D pos);
  void lightColorRequestResponsed(QColor color);
  void activeRequestResponsed(bool active);

 private Q_SLOTS:
  void on_positionXLineEdit_editingFinished();
  void on_positionYLineEdit_editingFinished();
  void on_positionZLineEdit_editingFinished();
  void on_distanceLineEdit_editingFinished();
  void on_colorButton_clicked();
  void on_lightSourceCheckBox_clicked(bool checked);
};

#endif  // LIGHTSOURCELISTELEMENT_HPP
