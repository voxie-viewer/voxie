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

#include <VoxieClient/Map.hpp>
#include <VoxieClient/Rotation.hpp>
#include <VoxieClient/Vector.hpp>

#include <Voxie/Data/BoundingBox3D.hpp>

#include <Voxie/Voxie.hpp>

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>

#include <QtGui/QMouseEvent>

#include <cmath>

namespace vx {
namespace spnav {
class SpaceNavVisualizer;
}

namespace visualization {
class MouseAction;

// TODO: Move getMousePositionQt() and getMousePosition() somewhere else?
VOXIECORESHARED_EXPORT Vector<double, 2> getMousePositionQt(QWidget* widget,
                                                            QMouseEvent* event);
VOXIECORESHARED_EXPORT Vector<double, 2> getMousePosition(QWidget* widget,
                                                          QMouseEvent* event);
VOXIECORESHARED_EXPORT Vector<double, 2> getMousePositionQt(QWidget* widget,
                                                            QWheelEvent* event);
VOXIECORESHARED_EXPORT Vector<double, 2> getMousePosition(QWidget* widget,
                                                          QWheelEvent* event);

enum class View3DProperty : quint32 {
  LookAt = 1 << 0,
  Orientation = 1 << 1,
  ZoomLog = 1 << 2,
  ViewSizeUnzoomed = 1 << 3,
  FieldOfView = 1 << 4,

  None = 0,
  All = LookAt | Orientation | ZoomLog | ViewSizeUnzoomed | FieldOfView,
};

// TODO: How should this be done properly?
static inline View3DProperty operator|(View3DProperty a, View3DProperty b) {
  return static_cast<View3DProperty>(static_cast<quint32>(a) |
                                     static_cast<quint32>(b));
}
static inline View3DProperty operator&(View3DProperty a, View3DProperty b) {
  return static_cast<View3DProperty>(static_cast<quint32>(a) &
                                     static_cast<quint32>(b));
}
static inline View3DProperty operator^(View3DProperty a, View3DProperty b) {
  return static_cast<View3DProperty>(static_cast<quint32>(a) ^
                                     static_cast<quint32>(b));
}
static inline View3DProperty operator~(View3DProperty a) {
  return static_cast<View3DProperty>(~static_cast<quint32>(a));
}
static inline View3DProperty& operator|=(View3DProperty& a, View3DProperty b) {
  return a = a | b;
}
static inline View3DProperty& operator&=(View3DProperty& a, View3DProperty b) {
  return a = a & b;
}

class VOXIECORESHARED_EXPORT View3DValues {
 public:
  View3DValues();

  View3DProperty valid;

  /**
   * @brief Coordiantes (in world space) around which rotations will happen
   */
  vx::Vector<double, 3> lookAt;

  /**
   * @brief Rotation which will transform coordinates from eye space to world
   * space (i.e. the rotation of the camera)
   */
  vx::Rotation<double, 3> orientation;

  /**
   * @brief Zoom value
   */
  double zoomLog;

  /**
   * @brief Size of region (in y direction) which can be seen in the lookAt
   * plane on standard zoom
   */
  double viewSizeUnzoomed;

  /**
   * @brief Field of view in y direction in rad.
   *
   * Note that 0 indicates an orthographic projection.
   */
  double fieldOfView;

  // Note: The set*() methods will also set the field to valid
  void setLookAt(const vx::Vector<double, 3>& value);
  void setOrientation(const vx::Rotation<double, 3>& value);
  void setZoomLog(double value);
  void setViewSizeUnzoomed(double value);
  void setFieldOfView(double value);

  void updateFrom(const View3DValues& values, View3DProperty toCopy);

  // Updates only properties which are valid in values
  void updateFrom(const View3DValues& values);
};

class VOXIECORESHARED_EXPORT View3D : public QObject {
  Q_OBJECT

 private:
  View3DValues values_;

  View3DProperty changable_;

  BoundingBox3D boundingBox_;

  double zoomLogMin_, zoomLogMax_;

  QSharedPointer<MouseAction> currentMouseAction_;

 public:
  /**
   * @brief constructor of View3D
   */
  View3D(QObject* parent, View3DProperty changable,
         const View3DValues& initial = View3DValues());

  // Note: The view matrix transforms into a coordinate system which is centered
  // on the center point, not on the eye point. Moving the origin to the eye
  // point is done in the projection matrix.
  vx::AffineMap<double, 3> viewMatrix() const;
  vx::ProjectiveMap<double, 3> projectionMatrix(double fWidth,
                                                double fHeight) const;

  void mousePressEvent(const vx::Vector<double, 2>& mousePosLast,
                       const vx::Vector<double, 2>& mousePosNow,
                       QMouseEvent* event, const QSize& windowSize);
  void mouseMoveEvent(const vx::Vector<double, 2>& mousePosLast,
                      const vx::Vector<double, 2>& mousePosNow,
                      QMouseEvent* event, const QSize& windowSize);
  void mouseReleaseEvent(const vx::Vector<double, 2>& mousePosLast,
                         const vx::Vector<double, 2>& mousePosNow,
                         QMouseEvent* event, const QSize& windowSize);
  void wheelEvent(const vx::Vector<double, 2>& mousePosNow, QWheelEvent* event,
                  const QSize& windowSize);
  void keyPressEvent(QKeyEvent* event, const QSize& windowSize);
  void setFixedAngle(QString direction);
  void switchProjection();
  void moveZoom(double value);

  void move(const vx::Vector<double, 3>& vectorViewSpace);
  void resetView(View3DProperty toReset = View3DProperty::LookAt |
                                          View3DProperty::ZoomLog |
                                          View3DProperty::Orientation);

  void registerSpaceNavVisualizer(vx::spnav::SpaceNavVisualizer* sn);

  const View3DValues& values() const { return values_; }

  View3DProperty changable() const { return changable_; }

  void update(const View3DValues& values);

  const BoundingBox3D& boundingBox() const { return boundingBox_; }
  double zoomLogMin() const { return zoomLogMin_; }
  double zoomLogMax() const { return zoomLogMax_; }

  const vx::Vector<double, 3>& lookAt() const { return values().lookAt; }
  vx::Rotation<double, 3> orientation() const { return values().orientation; }
  double zoomLog() const { return values().zoomLog; }
  double viewSizeUnzoomed() const { return values().viewSizeUnzoomed; }
  double fieldOfView() const { return values().fieldOfView; }

  double viewSizeZoomed() const {
    return viewSizeUnzoomed() / std::exp(zoomLog());
  }

  /**
   * Return the zoom value limited by some minimum and maximum value.
   */
  double limitZoomLog(double zoomLog) const;

  vx::HmgVector<double, 3> getCameraPosition();

  void setFieldOfView(double value);
  void setViewSizeUnzoomed(double value);
  void setLookAt(const vx::Vector<double, 3>& value);

  /**
   * @brief Size of 1 pixel at the lookAt in m
   * @param const QSize& windowSize
   * @return double
   */
  double pixelSize(const QSize& windowSize) const {
    return viewSizeUnzoomed() / windowSize.height() / std::exp(this->zoomLog());
  }

  void setBoundingBox(const BoundingBox3D& boundingBox);
  // Note: This will not update the zoom, call
  // view3d->setZoomLog(view3d->limitZoomLog(view3d->zoomLog())) if this is
  // desired
  void setZoomLogMinMax(double zoomLogMin, double zoomLogMax);
  void setZoomLogUnlimited();

 Q_SIGNALS:
  void changed(const View3DValues& update);

 public:
  /**
   * @brief rotate the camera to the given rotation
   * @param vx::Rotation<double, 3> rotation
   */
  void rotate(vx::Rotation<double, 3> rotation);

  /**
   * @brief setZoom sets the camera zoom.
   * @param double zoom
   */
  void setZoomLog(double zoomLog);

  void setOrientation(const vx::Rotation<double, 3>& rot);

  // TODO: Create proper accessors for these?

  // When zooming using Ctrl+middle mouse button and moving vertically over the
  // entire window, the log zoom will be modified by this amount
  double mouseActionZoomFactor = 2.0;

  // When rotating the mouse wheel once, the log zoom will be modified by this
  // amount
  double wheelZoomFactor = 3.0;

  // When rotating the mouse wheel once, the view will pan into Z direction by
  // this times the (zoomed) vertical view size
  double wheelPanFactor = 3.0;

  // Amount of rotation by keys
  double keyRotateAmountDeg = 22.5;

  // Amount of zoom (log) by keys
  double keyZoomAmount = 0.125;

  // When doing panning with keys, each key press will pan keyPanFactor times
  // the (zoomed) vertical view size
  double keyPanFactor = 0.125;

  // Keep the view Z value if possible when resetting the view
  bool resetKeepViewZ = false;
};
}  // namespace visualization
}  // namespace vx
