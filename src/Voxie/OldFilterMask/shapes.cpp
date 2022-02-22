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

#include "shapes.hpp"

#include <Voxie/OldFilterMask/ellipse.hpp>
#include <Voxie/OldFilterMask/polygon.hpp>
#include <Voxie/OldFilterMask/rectangle.hpp>

#include <VoxieClient/Exception.hpp>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>

#include <QtGui/QTransform>

using namespace vx::filter;

Shape::~Shape() {}

Shape* Shape::fromJson(const QJsonObject& json) {
  QString type = json["Type"].toString();

#define DEF(ty)                                      \
  if (type == "de.uni_stuttgart.Voxie.Shape2D." #ty) \
    return new ty(json["Data"].toObject());
  DEF(Polygon)
  DEF(Ellipse)
  DEF(Rectangle)
#undef DEF

  throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                      "Unknown Shape2D: " + type);
}

QJsonObject Shape::getJsonFull() {
  QJsonObject result;
  result["Type"] = getType();
  result["Data"] = getJson();
  return result;
}

QJsonValue Shape::qMatrixToJson(const QMatrix& matrix) {
  return QJsonObject{
      {"m11", matrix.m11()}, {"m12", matrix.m12()}, {"m21", matrix.m21()},
      {"m22", matrix.m22()}, {"dx", matrix.dx()},   {"dy", matrix.dy()},
  };
}
QMatrix Shape::qMatrixFromJson(const QJsonValue& json) {
  auto jsonO = json.toObject();
  return QMatrix(jsonO["m11"].toDouble(), jsonO["m12"].toDouble(),
                 jsonO["m21"].toDouble(), jsonO["m22"].toDouble(),
                 jsonO["dx"].toDouble(), jsonO["dy"].toDouble());
}
QJsonValue Shape::qTransformToJson(const QTransform& transform) {
  return QJsonObject{
      {"m11", transform.m11()}, {"m12", transform.m12()},
      {"m13", transform.m13()}, {"m21", transform.m21()},
      {"m22", transform.m22()}, {"m23", transform.m23()},
      {"m31", transform.m31()}, {"m32", transform.m32()},
      {"m33", transform.m33()},
  };
}
QTransform Shape::qTransformFromJson(const QJsonValue& json) {
  auto jsonO = json.toObject();
  return QTransform(jsonO["m11"].toDouble(), jsonO["m12"].toDouble(),
                    jsonO["m13"].toDouble(), jsonO["m21"].toDouble(),
                    jsonO["m22"].toDouble(), jsonO["m23"].toDouble(),
                    jsonO["m31"].toDouble(), jsonO["m32"].toDouble(),
                    jsonO["m33"].toDouble());
}
QJsonValue Shape::qPointFToJson(const QPointF& pointF) {
  return QJsonArray{pointF.x(), pointF.y()};
}
QPointF Shape::qPointFFromJson(const QJsonValue& json) {
  return QPointF(json.toArray()[0].toDouble(), json.toArray()[1].toDouble());
}
