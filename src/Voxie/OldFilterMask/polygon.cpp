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

#include "polygon.hpp"

#include <QtCore/QDebug>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>

using namespace vx::filter;

Polygon::Polygon(QVector<QPointF> polygonCoords) {
  this->polygonCoords = polygonCoords;
  data.cpuCoords = polygonCoords;
  data.gpuCoords = this->pointFVectorToPoly(data.cpuCoords);
  this->transformationRotate = QMatrix(1, 0, 0, 1, 0, 0);
  this->transformationScale = QMatrix(1, 0, 0, 1, 0, 0);
  this->transformationTranslate = QMatrix(1, 0, 0, 1, 0, 0);
  this->transformationMatrix = this->transformationScale *
                               this->transformationRotate *
                               this->transformationTranslate;
}

// TODO: This should not store all this (redundant) data
Polygon::Polygon(const QJsonObject& json) {
  polygonCoords.clear();
  for (const auto& entry : json["polygonCoords"].toArray())
    polygonCoords.push_back(qPointFFromJson(entry));

  transformationMatrix = qMatrixFromJson(json["transformationMatrix"]);
  transformationRotate = qMatrixFromJson(json["transformationRotate"]);
  transformationScale = qMatrixFromJson(json["transformationScale"]);
  transformationTranslate = qMatrixFromJson(json["transformationTranslate"]);

  for (int i = 0; i < polygonCoords.size(); i++) {
    QPointF result = this->transformationMatrix.map(polygonCoords.at(i));
    data.cpuCoords.append(result);
  }
  data.gpuCoords = this->pointFVectorToPoly(data.cpuCoords);
}
QJsonObject Polygon::getJson() {
  QJsonArray polygonCoords;
  for (const auto& entry : this->polygonCoords)
    polygonCoords << qPointFToJson(entry);

  return QJsonObject{
      {"polygonCoords", polygonCoords},
      {"transformationMatrix", qMatrixToJson(transformationMatrix)},
      {"transformationRotate", qMatrixToJson(transformationRotate)},
      {"transformationScale", qMatrixToJson(transformationScale)},
      {"transformationTranslate", qMatrixToJson(transformationTranslate)},
  };
}

void Polygon::translateOrigin(qreal x, qreal y) {
  data.cpuCoords.clear();
  data.gpuCoords.clear();

  this->transformationTranslate = this->transformationTranslate.translate(x, y);

  this->transformationMatrix = this->transformationScale *
                               this->transformationRotate *
                               this->transformationTranslate;

  for (int i = 0; i < polygonCoords.size(); i++) {
    QPointF result = this->transformationMatrix.map(polygonCoords.at(i));
    data.cpuCoords.append(result);
  }
  data.gpuCoords = this->pointFVectorToPoly(data.cpuCoords);
}

void Polygon::rotate(qreal angle) {
  data.cpuCoords.clear();
  data.gpuCoords.clear();

  this->transformationRotate = this->transformationRotate.rotate(angle);

  QPointF translatePoint(this->transformationTranslate.dx(),
                         this->transformationTranslate.dy());

  QMatrix tempRotate(1, 0, 0, 1, 0, 0);
  tempRotate.rotate(angle);
  QPointF rotatedPoint = tempRotate.map(translatePoint);
  this->transformationTranslate.setMatrix(1, 0, 0, 1, rotatedPoint.x(),
                                          rotatedPoint.y());

  // qDebug() << this->transformationTranslate;
  this->transformationMatrix = this->transformationScale *
                               this->transformationRotate *
                               this->transformationTranslate;

  for (int i = 0; i < polygonCoords.size(); i++) {
    QPointF result = this->transformationMatrix.map(polygonCoords.at(i));
    data.cpuCoords.append(result);
  }

  data.gpuCoords = this->pointFVectorToPoly(data.cpuCoords);
}

QPainterPath Polygon::path() {
  QPainterPath path;
  path.addPolygon(QPolygonF(this->polygonCoords));
  path = this->transformationMatrix.map(path);
  return path;
}

polygonData Polygon::getTransformatedPolyCoords() { return data; }
// from cpuCoords to gpuCoords
QVector<polygonPoint> Polygon::pointFVectorToPoly(QVector<QPointF> cpu) {
  QVector<polygonPoint> point;
  for (int i = 0; i < cpu.size(); i++) {
    polygonPoint addPoint;
    addPoint.x = (cl_float)cpu.at(i).x();
    addPoint.y = (cl_float)cpu.at(i).y();
    point.append(addPoint);
  }
  return point;
}
