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

#ifndef LIGHTSOURCE_H
#define LIGHTSOURCE_H

#include <QColor>
#include <QObject>
#include <QVector4D>

class LightSource : public QObject {
  Q_OBJECT
 private:
  QVector4D position;
  QColor lightColor;
  bool active;

  bool ignoreUpdates;

 public:
  explicit LightSource(QObject* parent = 0,
                       QVector4D position = QVector4D(1.0, 1.0, 1.0, 0.0),
                       QColor lightColor = QColor(255, 255, 105),
                       bool active = false);

  QVector4D getPosition();
  QColor getLightColor();
  bool isActive();

 Q_SIGNALS:

  void positionRequestResponse(QVector4D pos);
  void lightColorRequestResponse(QColor col);
  void activeRequestResponse(bool active);

 public Q_SLOTS:
  void positionRequested();
  void lightColorRequested();
  void activeRequested();

  void positionChangedByGUI(QVector4D position);
  void lightColorChangedByGUI(QColor col);
  void activeChangedByGUI(bool active);
};

#endif  // LIGHTSOURCE_H
