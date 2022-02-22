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

#include <Voxie/Voxie.hpp>

#include <Voxie/OldFilterMask/ellipse.hpp>
#include <Voxie/OldFilterMask/ellipseData.hpp>
#include <Voxie/OldFilterMask/polygon.hpp>
#include <Voxie/OldFilterMask/polygonData.hpp>
#include <Voxie/OldFilterMask/polygonPoint.hpp>
#include <Voxie/OldFilterMask/rectangle.hpp>
#include <Voxie/OldFilterMask/rectangleData.hpp>
#include <Voxie/OldFilterMask/shapes.hpp>

#include <VoxieBackend/OpenCL/CLInstance.hpp>

#include <QtCore/QMutex>
#include <QtCore/QObject>

#include <QtGui/QPainterPath>

namespace vx {
namespace filter {
/**
 * Selection2DMask is class which every 2DFilter contains. This is for 2D masks.
 * This class provides several methods for creating a 2D Selection mask in the
 * forms, Rectangle, Ellipse and Polygon. This Forms can also be deleted. By
 * changing the origin, this mask will be retained when the slice before and the
 * new one are equal. For these there are the slots translateOrigin whene origin
 * is moved and rotate when origin is rotated.
 * @brief The Selection2DMask class is for all masks on 2D. This classes offers
 * methods for creating masks, deleting them and manipulating them when
 * something changed from the origin. It also contains the drawing of the masks.
 */
class VOXIECORESHARED_EXPORT Selection2DMask : public QObject {
  Q_OBJECT
 private:
  QVector<Rectangle*> rectangles;
  QVector<Ellipse*> ellipses;
  QVector<Polygon*> polygons;
  QMutex mutex;

 public:
  QMutex& getLock() { return mutex; }

  /**
   * @brief isEmpty looks if a shape exists.
   * @return true if no shapes inside the mask, else it returns false when at
   * least one shape exist.
   */
  bool isEmpty() {
    return rectangles.isEmpty() && ellipses.isEmpty() && polygons.isEmpty();
  }

  Selection2DMask(QObject* parent = nullptr)
      : QObject(parent), mutex(QMutex::Recursive) {}

  //-----------CPU---------------------------------
  QPainterPath getPath();
  /**
   * @brief Returns all ellipse transformation matrixs where are created on this
   * mask
   * @return A QVector which contains the ellipseData
   */
  QVector<ellipseData> getEllipseCoords();

  /**
   * @brief Returns all rectangle transformation matrixs where are created on
   * this mask
   * @return A QVector which contains the rectangleData
   */
  QVector<rectangleData> getRectangleCoords();

  /**
   * @brief Returns all polygon Coordinates where are created on this mask
   * @return A QVector which contains the polygonData
   */
  QVector<polygonData> getPolygonCoords();

  //------------GPU---------------------------------

  /**
   * @brief getEllipseBuffer creates cl Buffers for all ellipses
   * @param instance the instance for creating the buffers.
   * @return the cl Buffer full with ellipses.
   */
  cl::Buffer getEllipseBuffer(vx::opencl::CLInstance* instance);

  /**
   * @brief getRectangleBuffer creates cl Buffers for all rectangle.
   * @param instance the instance for creating the buffers.
   * @return the cl Buffers full with rectangles.
   */
  cl::Buffer getRectangleBuffer(vx::opencl::CLInstance* instance);

  /**
   * @brief getPolygonBuffer creates cl Buffers for all polygons.
   * @param instance the instance for creating the buffers.
   * @return the cl Buffers full with polygons
   */
  cl::Buffer getPolygonBuffer(vx::opencl::CLInstance* instance);

  /**
   * @brief getPolygonBufferOffset creates cl Buffer with integers which
   * describes the offset for the polygon buffer.
   * @param instance the instance for creating the buffers.
   * @return cl Buffer as a offset.
   */
  cl::Buffer getPolygonBufferOffset(vx::opencl::CLInstance* instance);

  /**
   * @brief getRectangleSize
   * @return the number of all existing rectangles of this mask.
   */
  int getRectangleSize() { return rectangles.size(); }

  /**
   * @brief getEllipseSize
   * @return the number of all existing ellipses of this mask.
   */
  int getEllipseSize() { return ellipses.size(); }

  /**
   * @brief getPolygonSize
   * @return the number of all existing polygons of this mask.
   */
  int getPolygonSize() { return polygons.size(); }

  //------------------------------------------------
  /**
   * @brief addEllipse adds a new ellipse shape
   * @param midPointX the midpoint of the x axis of the ellipse
   * @param midPointY the midpoint of the y axis of the ellipse
   * @param radiusX the radius of the x axis of the ellipse
   * @param radiusY the radius of the y axis of the ellipse
   */
  void addEllipse(qreal midPointX, qreal midPointY, qreal radiusX,
                  qreal radiusY);

  /**
   * @brief addRectangle adds a new rectangle shape
   * @param startX the start point of the x direction
   * @param startY the start point of the y direction
   * @param endX the end position of the x direction
   * @param endY the end position of the y direction
   */
  void addRectangle(qreal startX, qreal startY, qreal endX, qreal endY);

  /**
   * @brief addPolygon adds a new polygon shape
   * @param polygonCoords a QVector which contains points.
   */
  void addPolygon(QVector<QPointF> polygonCoords);

  /**
   * @brief deleteMask delets the mask, it deletes every content of the shapes.
   */
  void clearMask();

  void addMaskFromJson(const QJsonObject& json);
  QJsonObject getMaskJson();

  /**
   * @brief Checks a given Point of Floats, if the point is inside the mask or
   * outside. It only works on CPU
   * @param coords which contains the given Point to check.
   * @return A boolean with true, if the Point is inside the Mask, else false.
   */
  bool isPointIn(QPointF coord);
 public Q_SLOTS:
  /**
   * @brief translateOrigin translates the masks in dircetin of origin
   * translation.
   * @param x the amount of translation in x direction.
   * @param y the amount of translation in y direction.
   */
  void translateOrigin(qreal x, qreal y);

  /**
   * @brief rotate rotates the masks along the direction which the images is
   * rotated.
   * @param angel the rotation angle
   */
  void rotate(qreal angel);
 Q_SIGNALS:
  void changed();
};
}  // namespace filter
}  // namespace vx
