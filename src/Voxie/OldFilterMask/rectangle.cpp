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

#include "rectangle.hpp"

#include <math.h>

#include <QtCore/QDebug>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>

using namespace vx::filter;

// constructor

/*
 * m11 m12 0
 * m21 m22 0
 * dx  dy  1
 *
 * m11 = horizontale Skalierung
 * m22 = vertikale Skalierung
 *
 * m21 = horizontale drehung
 * m12 = vertikale drehung
 *
 * dx = translation x
 * dy = translation y
 */
Rectangle::Rectangle(qreal startX, qreal startY, qreal endX, qreal endY) {
  // Set startPoints.

  // Constructor vertikale Skalierung, vertikale Drehung, horizontale Drehung,
  // horizontale Skalierung, translate X, translate Y

  this->transformationRotate = QTransform(1, 0, 0, 1, 0, 0);
  this->transformationScale =
      QTransform(fabs(endX - startX), 0, 0, fabs(endY - startY), 0, 0);
  // falls startpunkt unten links endpunkt oben rechts
  if (startX < endX && startY > endY) {
    // startX = endX - fabs(endX - startX);
    startY = endY;
  }
  // falls startpunkt oben rechts und endpunkt unten links
  else if (startX > endX && startY < endY) {
    startX = endX;
    // startY = startY - fabs(startY - endY);
  }
  // falls startpunkt unten rechts und endpunkt oben links
  else if (startX > endX && startY > endY) {
    int temp = startX;
    startX = endX;
    endX = temp;
    temp = startY;
    startY = endY;
    endY = temp;
  }
  this->transformationTranslate = QTransform(1, 0, 0, 1, startX, startY);
  this->transformationMatrix =
      this->transformationScale * this->transformationRotate;
  this->transformationMatrix *= this->transformationTranslate;

  if (!this->transformationMatrix.isInvertible()) {
    // qDebug() << "Notinvertible";
  }
  QTransform inverted = this->transformationMatrix.inverted();

  data.scaleX = (cl_float)inverted.m11();
  data.scaleY = (cl_float)inverted.m22();
  data.angleX = (cl_float)inverted.m21();
  data.angleY = (cl_float)inverted.m12();
  data.dx = (cl_float)inverted.dx();
  data.dy = (cl_float)inverted.dy();
}

// TODO: This should not store all this (redundant) data
Rectangle::Rectangle(const QJsonObject& json) {
  leftUp = qPointFFromJson(json["leftUp"]);
  leftDown = qPointFFromJson(json["leftDown"]);
  rightUp = qPointFFromJson(json["rightUp"]);
  rightDown = qPointFFromJson(json["rightDown"]);
  data.scaleX = json["data"].toObject()["scaleX"].toDouble();
  data.scaleY = json["data"].toObject()["scaleY"].toDouble();
  data.dx = json["data"].toObject()["dx"].toDouble();
  data.dy = json["data"].toObject()["dy"].toDouble();
  data.angleX = json["data"].toObject()["angleX"].toDouble();
  data.angleY = json["data"].toObject()["angleY"].toDouble();
  transformationMatrix = qTransformFromJson(json["transformationMatrix"]);
  transformationRotate = qTransformFromJson(json["transformationRotate"]);
  transformationScale = qTransformFromJson(json["transformationScale"]);
  transformationTranslate = qTransformFromJson(json["transformationTranslate"]);
}
QJsonObject Rectangle::getJson() {
  return QJsonObject{
      {"leftUp", qPointFToJson(leftUp)},
      {"leftDown", qPointFToJson(leftDown)},
      {"rightUp", qPointFToJson(rightUp)},
      {"rightDown", qPointFToJson(rightDown)},
      {
          "data",
          QJsonObject{
              {"scaleX", data.scaleX},
              {"scaleY", data.scaleY},
              {"dx", data.dx},
              {"dy", data.dy},
              {"angleX", data.angleX},
              {"angleY", data.angleY},
          },
      },
      {"transformationMatrix", qTransformToJson(transformationMatrix)},
      {"transformationRotate", qTransformToJson(transformationRotate)},
      {"transformationScale", qTransformToJson(transformationScale)},
      {"transformationTranslate", qTransformToJson(transformationTranslate)},
  };
}

void Rectangle::translateOrigin(qreal x, qreal y) {
  // translate the coordinates in rectangle struct

  // test translation

  this->transformationTranslate = this->transformationTranslate.translate(x, y);

  this->transformationMatrix = this->transformationScale *
                               this->transformationRotate *
                               this->transformationTranslate;

  if (!this->transformationMatrix.isInvertible()) {
    // qDebug() << "Not invertible";
  }
  QTransform inverted = this->transformationMatrix.inverted();
  data.scaleX = (cl_float)inverted.m11();
  data.scaleY = (cl_float)inverted.m22();
  data.angleX = (cl_float)inverted.m21();
  data.angleY = (cl_float)inverted.m12();
  data.dx = (cl_float)inverted.dx();
  data.dy = (cl_float)inverted.dy();
}

void Rectangle::rotate(qreal angle) {
  this->transformationRotate = this->transformationRotate.rotate(angle);

  QPointF translatePoint(this->transformationTranslate.dx(),
                         this->transformationTranslate.dy());

  QTransform tempRotate(1, 0, 0, 1, 0, 0);
  tempRotate.rotate(angle);
  QPointF rotatedPoint = tempRotate.map(translatePoint);
  this->transformationTranslate =
      QTransform(1, 0, 0, 1, rotatedPoint.x(), rotatedPoint.y());

  this->transformationMatrix = this->transformationScale *
                               this->transformationRotate *
                               this->transformationTranslate;

  if (!this->transformationMatrix.isInvertible()) {
    // qDebug() << "Not invertible";
  }
  QTransform inverted = this->transformationMatrix.inverted();

  data.scaleX = (cl_float)inverted.m11();
  data.scaleY = (cl_float)inverted.m22();
  data.angleX = (cl_float)inverted.m21();
  data.angleY = (cl_float)inverted.m12();
  data.dx = (cl_float)inverted.dx();
  data.dy = (cl_float)inverted.dy();
}

QPainterPath Rectangle::path() {
  QPainterPath path;
  path.addRect(QRectF(0, 0, 1, 1));
  path = this->transformationMatrix.map(path);
  return path;
}

void Rectangle::isPointIn(QPointF point) {
  Q_UNUSED(point);
  /*    QPointF resultLeftUp = this->transformationMatrix.map(this->leftUp);
          QPointF resultLeftDown =
    this->transformationMatrix.map(this->leftDown); QPointF resultRightUp =
    this->transformationMatrix.map(this->rightUp); QPointF resultRightDown =
    this->transformationMatrix.map(this->rightDown);

      QTransform inverted = this->transformationMatrix.inverted();
          QPointF result = inverted.map(point);
    //  qDebug() << this->transformationMatrix.isInvertible();*/
}

rectangleData Rectangle::getTransformationMatrix() { return this->data; }
