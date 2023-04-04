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

#include "GeometricPrimitiveLayer.hpp"

#include <Voxie/Data/GeometricPrimitiveObject.hpp>
#include <Voxie/Data/Prototypes.hpp>
#include <VoxieBackend/Data/GeometricPrimitive.hpp>
#include <VoxieBackend/Data/GeometricPrimitiveData.hpp>

#include <Voxie/Node/ParameterCopy.hpp>

#include <PluginVisSlice/Prototypes.hpp>
#include <PluginVisSlice/SliceVisualizer.hpp>

#include <QtGui/QGuiApplication>
#include <QtGui/QPainter>

using namespace vx;

// TODO: Seems like sometimes the color of the points is not properly updated

// TODO: Use alpha of color values?

GeometricPrimitiveLayer::GeometricPrimitiveLayer(SliceVisualizer* sv) : sv(sv) {
  this->visibility = 10;

  // Redraw when the slice changes
  QObject::connect(sv->properties, &SliceProperties::orientationChanged, this,
                   &Layer::triggerRedraw);
  QObject::connect(sv->properties, &SliceProperties::originChanged, this,
                   &Layer::triggerRedraw);

  // Redraw when shown part of slice changes
  connect(sv->properties, &SliceProperties::centerPointChanged, this,
          &Layer::triggerRedraw);
  connect(sv->properties, &SliceProperties::verticalSizeChanged, this,
          &Layer::triggerRedraw);

  // Redraw when data changes
  /*
  connect(this, &SliceVisualizer::gpoDataChangedFinished, this,
          [ga]() { qDebug() << "SliceVisualizer::gpoDataChangedFinished"; });
  connect(this, &SliceVisualizer::currentPointChanged, this,
          [this]() { qDebug() << "SliceVisualizer::currentPointChanged"; });
  */
  connect(sv, &SliceVisualizer::gpoDataChangedFinished, this,
          &Layer::triggerRedraw);
  connect(sv, &SliceVisualizer::currentPointChanged, this,
          &Layer::triggerRedraw);
  connect(sv, &SliceVisualizer::newMeasurement, this, &Layer::triggerRedraw);

  // Redraw when colors change
  QObject::connect(this->sv->properties,
                   &SliceProperties::geometricPrimitiveColorOnSliceChanged,
                   this, &GeometricPrimitiveLayer::triggerRedraw);
  QObject::connect(
      this->sv->properties,
      &SliceProperties::geometricPrimitiveColorInFrontOfSliceChanged, this,
      &GeometricPrimitiveLayer::triggerRedraw);
  QObject::connect(this->sv->properties,
                   &SliceProperties::geometricPrimitiveColorBehindSliceChanged,
                   this, &GeometricPrimitiveLayer::triggerRedraw);
}

/*
void GeometricPrimitiveLayer::drawLine(const QPoint& startPos,
                                      const QPoint& endPos) {
  float distance =
      this->sv->sliceImage().distanceInMeter(startPos, endPos);
  QPainter painter(&tempImg);
  painter.setRenderHint(QPainter::Antialiasing, true);
  QPen pen(QColor(0, 0, 0, 140));
  pen.setWidth(2);
  painter.setPen(pen);
  painter.drawLine(startPos, endPos);
  pen.setColor(QColor(255, 255, 0));
  pen.setWidth(1);
  pen.setStyle(Qt::DashLine);
  painter.setPen(pen);
  painter.drawLine(startPos, endPos);

  // draw the arrows
  QPoint point = endPos;
  int orthVecX = ((startPos.y() - endPos.y()) / 20);
  int orthVecY = ((startPos.x() - endPos.x()) / 20);

  QPoint basePoint = startPos;
  basePoint.setX(startPos.x() - (startPos.x() - endPos.x()) / 20);
  basePoint.setY(startPos.y() - (startPos.y() - endPos.y()) / 20);
  point.setX(basePoint.x() - orthVecX);
  point.setY(basePoint.y() + orthVecY);
  painter.drawLine(startPos, point);
  point.setX(basePoint.x() + orthVecX);
  point.setY(basePoint.y() - orthVecY);
  painter.drawLine(startPos, point);
  basePoint.setX(endPos.x() + (startPos.x() - endPos.x()) / 20);
  basePoint.setY(endPos.y() + (startPos.y() - endPos.y()) / 20);
  point.setX(basePoint.x() - orthVecX);
  point.setY(basePoint.y() + orthVecY);
  painter.drawLine(endPos, point);
  point.setX(basePoint.x() + orthVecX);
  point.setY(basePoint.y() - orthVecY);
  painter.drawLine(endPos, point);

  QString txt = QString::number(distance);
  QPoint txtPos = startPos + (endPos - startPos) * 0.5f;
  QRect txtRect = QRect(
      txtPos, QFontMetrics(QFont()).boundingRect(txt).size() + QSize(3, 0));
  painter.fillRect(txtRect, QColor(0, 0, 0, 140));
  pen.setStyle(Qt::SolidLine);
  painter.setPen(pen);
  painter.drawText(txtRect, Qt::TextSingleLine, txt);
}
*/

void GeometricPrimitiveLayer::drawPoint(QImage& outputImage,
                                       SlicePropertiesBase* properties,
                                       float visibility, QPointF point,
                                       float distance, std::string name,
                                       QVector3D vector, bool isSelected) {
  QPainter painter(&outputImage);
  QPen pen;
  pen.setStyle(Qt::SolidLine);
  QColor color;
  if (distance < -visibility / 100) {
    color = properties->geometricPrimitiveColorBehindSlice().asQColor();
    color.setAlpha(255 + 255 * distance / visibility < 0
                       ? 0
                       : 255 + 255 * distance / visibility);
  } else if (distance > visibility / 100) {
    color = properties->geometricPrimitiveColorInFrontOfSlice().asQColor();
    color.setAlpha(255 - 255 * distance / visibility < 0
                       ? 0
                       : 255 - 255 * distance / visibility);
  } else
    color = properties->geometricPrimitiveColorOnSlice().asQColor();
  if (isSelected) color = QColor(0, 0, 255, color.alpha());  // TODO
  pen.setColor(color);
  painter.setPen(pen);
  QFont font;  // applications default
  painter.setFont(font);
  QPoint upperleft(point.x() - 5, point.y() - 5);
  QPoint lowerright(point.x() + 5, point.y() + 5);
  QPoint upperright(point.x() + 5, point.y() - 5);
  QPoint lowerleft(point.x() - 5, point.y() + 5);
  painter.drawLine(upperleft, lowerright);
  painter.drawLine(upperright, lowerleft);

  if (distance > -visibility && distance < visibility) {
    std::string info = "";
    info += name;
    info += " ( ";
    info += std::to_string(vector.x());
    info += ", ";
    info += std::to_string(vector.y());
    info += ", ";
    info += std::to_string(vector.z());
    info += ")";
    QString qName = QString::fromStdString(info);
    QRect txtRect =
        QRect(upperright + QPoint(0, -10),
              QFontMetrics(font).boundingRect(qName).size() + QSize(10, 0));
    painter.fillRect(txtRect, QColor(0, 0, 0, 140));
    painter.drawText(txtRect, Qt::TextSingleLine, qName);
  }
}

void GeometricPrimitiveLayer::redrawPoints(
    QImage& outputImage, SlicePropertiesBase* properties, float visibility,
    const QSharedPointer<GeometricPrimitiveData>& gpd, quint64 currentPointID) {
  if (!gpd) return;

  PlaneInfo plane(properties->origin(), properties->orientation());
  auto planeArea =
      SliceVisualizer::getCurrentPlaneArea(properties, outputImage.size());

  auto primitives = gpd->primitives();
  for (const auto& id : primitives.keys()) {
    const auto& primitive = primitives[id];
    auto pointPrimitive =
        qSharedPointerDynamicCast<GeometricPrimitivePoint>(primitive);
    if (!pointPrimitive) continue;
    QVector3D point = pointPrimitive->position();
    float distance = plane.distance(point);
    QPointF onPlane = plane.get2DPlanePoint(point);
    QPointF onSlice = SliceVisualizer::planePointToPixel(outputImage.size(),
                                                         planeArea, onPlane);
    drawPoint(outputImage, properties, visibility, onSlice, distance,
              pointPrimitive->name().toStdString(), point,
              id == currentPointID);
  }
}

void GeometricPrimitiveLayer::drawLine(QImage& outputImage,
                                      SlicePropertiesBase* properties,
                                      float visibility, QVector3D p1,
                                      QVector3D p2) {
  PlaneInfo plane(properties->origin(), properties->orientation());
  auto planeArea =
      SliceVisualizer::getCurrentPlaneArea(properties, outputImage.size());

  float distanceP1 = plane.distance(p1);
  float distanceP2 = plane.distance(p2);

  QPainter painter(&outputImage);

  QPointF p1OnPixel = SliceVisualizer::planePointToPixel(
      outputImage.size(), planeArea, plane.get2DPlanePoint(p1));
  QPointF p2OnPixel = SliceVisualizer::planePointToPixel(
      outputImage.size(), planeArea, plane.get2DPlanePoint(p2));

  // TODO: don't compare floats with ==
  if (distanceP1 == 0 && distanceP2 == 0) {  // both on Slice
    QPen pen;
    pen.setColor(properties->geometricPrimitiveColorOnSlice().asQColor());
    painter.setPen(pen);
    painter.drawLine(p1OnPixel, p2OnPixel);
  } else if (distanceP1 * distanceP2 == 0) {  // exactly one Point on Plane
    QPointF offPlane;
    QPointF onPlane;

    onPlane = distanceP1 == 0 ? p1OnPixel : p2OnPixel;
    offPlane = distanceP1 == 0 ? p2OnPixel : p1OnPixel;
    float distanceOff = distanceP1 == 0 ? distanceP2 : distanceP1;
    QPointF furthestVisible;
    if (distanceOff > visibility || distanceOff < -visibility) {
      furthestVisible.setX(onPlane.x() +
                           (offPlane.x() - onPlane.x()) *
                               (visibility / std::abs(distanceOff)));
      furthestVisible.setY(onPlane.y() +
                           (offPlane.y() - onPlane.y()) *
                               (visibility / std::abs(distanceOff)));
    } else {
      furthestVisible = offPlane;
    }
    QLinearGradient gradient(onPlane, furthestVisible);
    QColor colorFar =
        distanceOff > 0
            ? properties->geometricPrimitiveColorInFrontOfSlice().asQColor()
            : properties->geometricPrimitiveColorBehindSlice().asQColor();

    gradient.setColorAt(0, colorFar);

    colorFar.setAlpha(std::abs(distanceOff) > visibility
                          ? 0
                          : 255 - 255 * std::abs(distanceOff) / visibility);
    gradient.setColorAt(1, colorFar);

    QBrush brush(gradient);
    QPen pen;
    pen.setBrush(brush);
    painter.setPen(pen);
    painter.drawLine(onPlane, offPlane);
  } else if (distanceP1 * distanceP2 > 0) {  // both on same side of the Plane
    QPointF closerToPlane;
    QPointF furtherFromPlane;
    float distanceClose;
    float distanceFar;
    if (distanceP1 > 0) {
      closerToPlane = distanceP1 < distanceP2 ? p1OnPixel : p2OnPixel;
      furtherFromPlane = distanceP1 < distanceP2 ? p2OnPixel : p1OnPixel;
      distanceClose = distanceP1 < distanceP2 ? distanceP1 : distanceP2;
      distanceFar = distanceP1 < distanceP2 ? distanceP2 : distanceP1;
    } else {
      closerToPlane = distanceP1 > distanceP2 ? p1OnPixel : p2OnPixel;
      furtherFromPlane = distanceP1 > distanceP2 ? p2OnPixel : p1OnPixel;
      distanceClose = distanceP1 > distanceP2 ? distanceP1 : distanceP2;
      distanceFar = distanceP1 > distanceP2 ? distanceP2 : distanceP1;
    }
    if (distanceClose < visibility &&
        distanceClose > -visibility) {  // closer Point within visible range
      QPoint furthestVisible;
      if (distanceFar > -visibility) {
        furthestVisible.setX(closerToPlane.x() +
                             (furtherFromPlane.x() - closerToPlane.x()) *
                                 (visibility - distanceClose) /
                                 (distanceFar - distanceClose));
        furthestVisible.setY(closerToPlane.y() +
                             (furtherFromPlane.y() - closerToPlane.y()) *
                                 (visibility - distanceClose) /
                                 (distanceFar - distanceClose));
      } else {
        furthestVisible.setX(closerToPlane.x() +
                             (furtherFromPlane.x() - closerToPlane.x()) *
                                 (visibility - distanceClose) /
                                 (-distanceFar - distanceClose));
        furthestVisible.setY(closerToPlane.y() +
                             (furtherFromPlane.y() - closerToPlane.y()) *
                                 (visibility - distanceClose) /
                                 (-distanceFar - distanceClose));
      }

      QLinearGradient gradient(
          closerToPlane, distanceFar < visibility && distanceFar > -visibility
                             ? furtherFromPlane
                             : furthestVisible);
      QColor color =
          distanceClose < 0
              ? properties->geometricPrimitiveColorBehindSlice().asQColor()
              : properties->geometricPrimitiveColorInFrontOfSlice().asQColor();
      //(distanceClose < 0 ? 200 : 0, 0, distanceClose < 0 ? 0 : 200);
      color.setAlpha(distanceClose < 0
                         ? 255 + 255 * distanceClose / visibility
                         : 255 - 255 * distanceClose / visibility);
      gradient.setColorAt(0, color);
      if (distanceFar < visibility && distanceFar > -visibility) {
        color.setAlpha(distanceFar < 0 ? 255 + 255 * distanceFar / visibility
                                       : 255 - 255 * distanceFar / visibility);
      } else {
        color.setAlpha(0);
      }
      gradient.setColorAt(1, color);

      QBrush brush(gradient);
      QPen pen;
      pen.setBrush(brush);
      painter.setPen(pen);
      painter.drawLine(closerToPlane,
                       distanceFar < visibility && distanceFar > -visibility
                           ? furtherFromPlane
                           : furthestVisible);
    }
  } else {  // one point on either side of the Plane
    QPointF cutsPlaneAt;
    QPointF pointBelowPlane = distanceP1 < 0 ? p1OnPixel : p2OnPixel;
    QPointF pointAbovePlane = distanceP1 > 0 ? p1OnPixel : p2OnPixel;
    float distanceAbovePlane = distanceP1 > 0 ? distanceP1 : distanceP2;
    float distanceBelowPlane = distanceP1 < 0 ? distanceP1 : distanceP2;

    cutsPlaneAt.setX(p1OnPixel.x() + (p2OnPixel.x() - p1OnPixel.x()) *
                                         distanceP1 /
                                         (distanceP1 - distanceP2));
    cutsPlaneAt.setY(p1OnPixel.y() + (p2OnPixel.y() - p1OnPixel.y()) *
                                         distanceP1 /
                                         (distanceP1 - distanceP2));

    QPoint visibilityAbovePlane;
    visibilityAbovePlane.setX(
        distanceAbovePlane < visibility
            ? pointAbovePlane.x()
            : cutsPlaneAt.x() - (cutsPlaneAt.x() - pointAbovePlane.x()) *
                                    visibility / (distanceAbovePlane));
    visibilityAbovePlane.setY(
        distanceAbovePlane < visibility
            ? pointAbovePlane.y()
            : cutsPlaneAt.y() - (cutsPlaneAt.y() - pointAbovePlane.y()) *
                                    visibility / (distanceAbovePlane));

    QPoint visibilityBelowPlane;
    visibilityBelowPlane.setX(
        distanceBelowPlane > -visibility
            ? pointBelowPlane.x()
            : cutsPlaneAt.x() + (cutsPlaneAt.x() - pointBelowPlane.x()) *
                                    visibility / (distanceBelowPlane));
    visibilityBelowPlane.setY(
        distanceBelowPlane > -visibility
            ? pointBelowPlane.y()
            : cutsPlaneAt.y() + (cutsPlaneAt.y() - pointBelowPlane.y()) *
                                    visibility / (distanceBelowPlane));

    QLinearGradient gradient(cutsPlaneAt, visibilityBelowPlane);
    QColor colorRed =
        properties->geometricPrimitiveColorBehindSlice().asQColor();
    QColor colorBlue =
        properties->geometricPrimitiveColorInFrontOfSlice().asQColor();
    QColor colorGreen = properties->geometricPrimitiveColorOnSlice().asQColor();

    colorRed.setAlpha(255);
    gradient.setColorAt(0, colorRed);
    colorRed.setAlpha(distanceBelowPlane < -visibility
                          ? 0
                          : 255 + 255 * distanceBelowPlane / visibility);
    gradient.setColorAt(1, colorRed);

    QBrush brush(gradient);
    QPen pen;
    pen.setBrush(brush);
    painter.setPen(pen);
    painter.drawLine(cutsPlaneAt, visibilityBelowPlane);

    QLinearGradient gradientBlue(cutsPlaneAt, visibilityAbovePlane);
    colorBlue.setAlpha(255);
    gradientBlue.setColorAt(0, colorBlue);
    colorBlue.setAlpha(distanceAbovePlane > visibility
                           ? 0
                           : 255 - 255 * distanceAbovePlane / visibility);
    gradientBlue.setColorAt(1, colorBlue);

    QBrush brushBlue(gradientBlue);
    QPen penBlue;
    penBlue.setBrush(brushBlue);
    painter.setPen(penBlue);
    painter.drawLine(cutsPlaneAt, visibilityAbovePlane);

    QPen penGreen;
    penGreen.setColor(colorGreen);
    painter.setPen(penGreen);
    painter.drawLine(QPoint(cutsPlaneAt.x() - 3, cutsPlaneAt.y() - 3),
                     QPoint(cutsPlaneAt.x() + 3, cutsPlaneAt.y() + 3));
    painter.drawLine(QPoint(cutsPlaneAt.x() - 3, cutsPlaneAt.y() + 3),
                     QPoint(cutsPlaneAt.x() + 3, cutsPlaneAt.y() - 3));
  }
}

void GeometricPrimitiveLayer::newVisibility(float newVis) {
  this->visibility = newVis;
  triggerRedraw();
}

void GeometricPrimitiveLayer::render(
    QImage& outputImage, const QSharedPointer<vx::ParameterCopy>& parameters) {
  SlicePropertiesCopy properties(
      parameters->properties()[parameters->mainNodePath()]);

  // TODO: This is not multithreading safe, clean up
  auto visibility = this->visibility;

  auto gpoPath = properties.geometricPrimitiveRaw();
  if (gpoPath == QDBusObjectPath("/")) return;
  GeometricPrimitivePropertiesCopy gpoProperties(
      parameters->properties()[gpoPath]);

  auto gpd = qSharedPointerDynamicCast<GeometricPrimitiveData>(
      parameters->getData(gpoPath).data());
  if (!gpd) return;

  auto currentPointID = gpoProperties.selectedPrimitive();
  auto measurement =
      GeometricPrimitiveNode::currentMeasurementPoints(gpd, &gpoProperties);

  redrawPoints(outputImage, &properties, visibility, gpd, currentPointID);

  if (std::get<0>(measurement))
    drawLine(outputImage, &properties, visibility, std::get<1>(measurement),
             std::get<2>(measurement));
}
