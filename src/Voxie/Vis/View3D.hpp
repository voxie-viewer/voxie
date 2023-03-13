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

#include <Voxie/Vis/MouseOperation.hpp>
#include <Voxie/Voxie.hpp>
#include <cmath>

#include <QtCore/QObject>

#include <QSharedPointer>

#include <QtGui/QMouseEvent>
#include <QtGui/QQuaternion>

namespace vx {

namespace spnav {
class SpaceNavVisualizer;
}

namespace visualization {

class VOXIECORESHARED_EXPORT View3D : public QObject {
  Q_OBJECT
 private:
  /**
   * @brief minimal and maximal values for zoom factor
   */
  float zoomMin, zoomMax;

  /**
   * @brief Field of view in y direction in rad.
   *
   * Note that 0 indicates an orthographic projection.
   */
  float fieldOfView;

  /**
   * @brief Size of region (in y direction) which can be seen in the centerPoint
   * plane on standard zoom
   */
  float viewSizeStandard;

  /**
   * @brief Zoom factor
   */
  float zoom;

  /**
   * @brief Coordiantes (in world space) around which rotations will happen
   */
  QVector3D centerPoint;

  /**
   * @brief Rotation which will transform coordinates from eye space to world
   * space (i.e. the rotation of the camera)
   */
  QQuaternion rotation;

  float factor;

  QSharedPointer<MouseOperation> mouseOperation;

 public:
  /**
   * @brief constructor of View3D
   * @param QObject* parent
   * @param QSharedPointer<MouseOperation> mouseOperation
   * @param float viewSizeStandard
   * @param float zoomMin
   * @param float zoomMax
   */
  View3D(QObject* parent, QSharedPointer<MouseOperation> mouseOperation);

  // Note: The view matrix transforms into a coordinate system which is centered
  // on the center point, not on the eye point. Moving the origin to the eye
  // point is done in the projection matrix.
  QMatrix4x4 viewMatrix();
  QMatrix4x4 projectionMatrix(float fWidth, float fHeight);

  void mousePressEvent(const QPoint& mouseLast, QMouseEvent* event,
                       const QSize& windowSize);
  void mouseMoveEvent(const QPoint& mouseLast, QMouseEvent* event,
                      const QSize& windowSize);
  void wheelEvent(QWheelEvent* event, const QSize& windowSize);
  void keyPressEvent(QKeyEvent* event, const QSize& windowSize);
  void setFixedAngle(QString direction);
  void switchProjection();
  void moveZoom(float value);

  void move(QVector3D vectorViewSpace);
  void resetView();

  void registerSpaceNavVisualizer(vx::spnav::SpaceNavVisualizer* sn);

  const QVector3D& getCenterPoint() const { return centerPoint; }

  float viewSize() const { return viewSizeStandard; }

  float getZoomMin() const { return zoomMin; }
  float getZoomMax() const { return zoomMax; }
  float getFieldOfView() const { return fieldOfView; }
  float getViewSizeStandard() const { return viewSizeStandard; }
  float getZoom() const { return zoom; }
  QQuaternion getRotation() const { return rotation; }
  QVector4D getCameraPosition();

  void setFieldOfView(float value);
  void setViewSizeStandard(float value);
  void setCenterPoint(const QVector3D& value);

  /**
   * @brief Size of 1 pixel at the centerPoint in m
   * @param const QSize& windowSize
   * @return float
   */
  float pixelSize(const QSize& windowSize) const {
    return viewSize() / windowSize.height() / this->zoom;
  }

  /**
   * @brief setStandardZoom Sets a new ViewSizeStandard and calculates depending
   * variables.
   * @param diagonalSize Diagonale of the displayed DataObject
   * @param changeCurrentZoom Defines if the current Zoom Level should be
   * changed
   */
  void setStandardZoom(float diagonalSize, bool changeCurrentZoom = true,
                       float zoomMin = 1e-2, float zoomMax = 1e2);

 Q_SIGNALS:
  void changed();
  /**
   * @brief objectPositionChangeRequested is signalled if a other class request
   * the object position.
   * @param QVector3D offset
   */
  void objectPositionChangeRequested(QVector3D offset);

  /**
   * @brief objectRotationChangeRequested is signalled if a other class request
   * the object rotation.
   * @param QQuaternion angle
   */
  void objectRotationChangeRequested(QQuaternion angle);

  /**
   * @brief cameraRotationChanged is signalled if the camera rotation changed.
   * @param QQuaternion angle
   */
  void cameraRotationChanged(QQuaternion angel);

  /**
   * @brief zoomChanged is signalled if the zoom changed.
   * @param float zoom (current zoom factor)
   * @param float zoomMin
   * @param float zoomMax
   */
  void zoomChanged(float zoom, float zoomMin, float zoomMax);

 public Q_SLOTS:

  /**
   * @brief cameraRotationRequested emit cameraRotationChanged signal to answer
   * this request.
   */
  void cameraRotationRequested();

  /**
   * @brief zoomRequested emit zoomChanged signal to answer this request.
   */
  void zoomRequested();

  /**
   * @brief rotate the camera to the given rotation
   * @param QQuaternion rotation
   */
  void rotate(QQuaternion rotation);

  /**
   * @brief setZoom sets the camera zoom.
   * @param float zoom
   */
  void setZoom(float zoom);

  void setRotation(QQuaternion rot);
};
}  // namespace visualization
}  // namespace vx
