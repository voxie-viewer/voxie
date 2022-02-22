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

#include "ellipse.hpp"

#include <QtCore/QDebug>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>

#include <QtGui/QPainterPath>

using namespace vx::filter;

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
Ellipse::Ellipse(qreal midPointX, qreal midPointY, qreal radiusX,
                 qreal radiusY) {
  // Constructor vertikale Skalierung, vertikale Drehung, horizontale Drehung,
  // horizontale Skalierung, translate X, translate Y

  this->transformationRotate = QTransform(1, 0, 0, 1, 0, 0);
  this->transformationScale = QTransform(radiusX, 0, 0, radiusY, 0, 0);

  this->transformationTranslate = QTransform(1, 0, 0, 1, midPointX, midPointY);
  this->transformationMatrix = this->transformationScale *
                               this->transformationRotate *
                               this->transformationTranslate;

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
Ellipse::Ellipse(const QJsonObject& json) {
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
QJsonObject Ellipse::getJson() {
  return QJsonObject{
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

QPainterPath Ellipse::path() {
  QPainterPath path;
  path.addEllipse(QPointF(0, 0), 1, 1);
  path = this->transformationMatrix.map(path);
  return path;
}

void Ellipse::translateOrigin(qreal x, qreal y) {
  this->transformationTranslate = this->transformationTranslate.translate(x, y);
  this->transformationMatrix = this->transformationScale *
                               this->transformationRotate *
                               this->transformationTranslate;

  this->transformationMatrix =
      transformationScale * transformationRotate * transformationTranslate;

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

void Ellipse::rotate(qreal angle) {
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

ellipseData Ellipse::getTransformationMatrix() { return data; }
