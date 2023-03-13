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

#include "View3D.hpp"

#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <Voxie/SpNav/SpaceNavVisualizer.hpp>
#include <Voxie/Vis/MouseOperation.hpp>

#include <QtCore/QSharedPointer>
#include <QtCore/QTimer>

#include <QtGui/QMatrix4x4>
#include <QtGui/QVector3D>

using namespace vx::visualization;

View3D::View3D(QObject* parent,
               const QSharedPointer<MouseOperation> mouseOperation)
    : QObject(parent),
      // Note: For the 3D visualizer these initial values will be overwritten
      // with values from the properties
      fieldOfView(40 / 180.0 * M_PI),
      centerPoint(0, 0, 0),
      rotation(1, 0, 0, 0),
      mouseOperation(mouseOperation) {
  this->viewSizeStandard = 0.25;
  this->setStandardZoom(1);
}

void View3D::setStandardZoom(float diagonalSize, bool changeCurrentZoom,
                             float zoomMin, float zoomMax) {
  this->factor = 0.25 / diagonalSize;
  if (changeCurrentZoom) {
    this->zoom = factor;
  }
  this->zoomMin = zoomMin * factor;
  this->zoomMax = zoomMax * factor;
  Q_EMIT zoomChanged(this->zoom, this->zoomMin, this->zoomMax);
}

QMatrix4x4 View3D::viewMatrix() {
  QMatrix4x4 matView;
  matView.scale(zoom);
  matView.rotate(QQuaternion(rotation.scalar(), -rotation.vector()));
  matView.translate(-centerPoint);
  // qDebug() << "matView" << matView;
  return matView;
}

QMatrix4x4 View3D::projectionMatrix(float fWidth, float fHeight) {
  QMatrix4x4 matProj;

  // Near and far plane should be at most 1000 times the view size
  float near = -1000 * viewSizeStandard;
  float far = 1000 * viewSizeStandard;

  // Calculate perspective projection, but allow fieldOfView to be 0 (which
  // will be the same as orthographic projection)

  // TODO: handle negative fieldOfView values?

  float sine = std::sin(fieldOfView / 2.0f);
  float cosine = std::cos(fieldOfView / 2.0f);

  // Distance from centerPoint to camera
  float distanceCameraCenterSine = 0.5f * viewSize() * cosine;

  // near plane is at least 1/10 of camera <-> center distance
  // near = std::max(near, distanceCameraCenter * 0.1f -
  // distanceCameraCenter);
  // Because distanceCameraCenterSine should be positive this condition should
  // never be true if sine is 0
  if (near * sine < distanceCameraCenterSine * 0.1f - distanceCameraCenterSine)
    near = (distanceCameraCenterSine * 0.1f - distanceCameraCenterSine) / sine;

  // matProj.perspective(fieldOfView / M_PI * 180, fWidth / fHeight, near +
  // distanceCameraCenterSine/sine, far + distanceCameraCenterSine/sine);
  // matProj.translate(0, 0, -distanceCameraCenter);
  // qDebug() << "A" << matProj;
  float clip = far - near;
  // Same as matrix produced by QMatrix4x4::perspective(), but with a
  // translate by (0, 0, -distanceCameraCenter) before it and the entire
  // matrix multiplied with sine
  matProj(0, 0) = cosine / fWidth * fHeight;
  matProj(1, 1) = cosine;
  matProj(2, 2) =
      -(near * sine + far * sine + 2 * distanceCameraCenterSine) / clip;
  // matProj(2, 3) = -(2.0f * near + distanceCameraCenter * far +
  // distanceCameraCenter) / clip * sine;
  /*
  matProj(2, 3) = (-distanceCameraCenterSine) *
                      (-(near + far + 2 * distanceCameraCenter) / clip) +
                  -(2.0f * (near * sine + distanceCameraCenterSine) *
                    (far + distanceCameraCenter)) /
                      clip;
  */
  matProj(2, 3) = (-2.0f * near * far * sine - near * distanceCameraCenterSine -
                   far * distanceCameraCenterSine) /
                  clip;
  matProj(3, 2) = -1.0f * sine;
  // matProj(3, 3) = 0.0f * sine;
  matProj(3, 3) = (-distanceCameraCenterSine) * (-1.0f);
  // qDebug() << "B" << matProj;

  // matProj /= matProj.row(3).length();

  // qDebug() << near / viewSizeStandard << far / viewSizeStandard;
  // qDebug() << matProj;
  // qDebug() << matProj / matProj.row(3).length();
  return matProj;
}

QVector4D View3D::getCameraPosition() {
  // Distance from centerPoint to camera
  float sine = std::sin(fieldOfView / 2.0f);
  float cosine = std::cos(fieldOfView / 2.0f);
  float distanceCameraCenterSine = 0.5f * viewSize() * cosine;

  QVector3D cameraOffset =
      (rotation * QVector3D(0, 0, distanceCameraCenterSine));

  return QVector4D(centerPoint * (zoom * sine) + cameraOffset, (zoom * sine));
}

// Give a mouse position / window size, return a vector with x, y in [-1;1]
static QVector3D toVector(const QPoint& pos, const QSize& size) {
  return QVector3D(2.0 * (pos.x() + 0.5) / size.width() - 1.0,
                   1.0 - 2.0 * (pos.y() + 0.5) / size.height(), 0);
}

// Information about arcball:
// https://en.wikibooks.org/w/index.php?title=OpenGL_Programming/Modern_OpenGL_Tutorial_Arcball&oldid=2356903
static QVector3D getArcballVector(const QPoint& pos, const QSize& size) {
  QVector3D p = toVector(pos, size);
  if (p.lengthSquared() < 1)
    p.setZ(std::sqrt(1 - p.lengthSquared()));  // Pythagore
  else
    p.normalize();  // nearest point
  return p;
}

void View3D::mousePressEvent(const QPoint& mouseLast, QMouseEvent* event,
                             const QSize& windowSize) {
  Q_UNUSED(mouseLast);
  Q_UNUSED(event);
  Q_UNUSED(windowSize);
}
void View3D::mouseMoveEvent(const QPoint& mouseLast, QMouseEvent* event,
                            const QSize& windowSize) {
  float factor = 1.0f;
  if (event->modifiers().testFlag(Qt::ShiftModifier)) factor = 0.1f;

  int dxInt = event->x() - mouseLast.x();
  int dyInt = event->y() - mouseLast.y();

  if (dxInt == 0 && dyInt == 0) return;

  float dx = dxInt * factor;
  float dy = dyInt * factor;

  auto action = mouseOperation->decideAction(event);

  switch (action) {
    case MouseOperation::Action::RotateObject: {
      // Todo: Use arcball vector
      auto matView = viewMatrix();
      QQuaternion quatX = QQuaternion::fromAxisAndAngle(
          (QVector4D(0, 1, 0, 0) * matView).toVector3D(), dx * 0.15);
      QQuaternion quatY = QQuaternion::fromAxisAndAngle(
          (QVector4D(1, 0, 0, 0) * matView).toVector3D(), dy * 0.15);
      Q_EMIT objectRotationChangeRequested(quatX * quatY);
      break;
    }
    case MouseOperation::Action::RotateView: {
      QVector3D va = getArcballVector(mouseLast, windowSize);
      QVector3D vb = getArcballVector(event->pos(), windowSize);
      float angle = std::acos(std::min(1.0f, QVector3D::dotProduct(va, vb)));
      QVector3D axis = QVector3D::crossProduct(va, vb);
      angle = angle * factor;
      rotation *= QQuaternion::fromAxisAndAngle(axis, -angle / M_PI * 180);
      rotation.normalize();
      Q_EMIT changed();
      break;
    }
    case MouseOperation::Action::MoveView:
    case MouseOperation::Action::MoveObject: {
      QVector3D move(dx, -dy, 0);
      move *= pixelSize(windowSize);
      auto offset = rotation.rotatedVector(move);
      if (action == MouseOperation::Action::MoveObject) {
        Q_EMIT objectPositionChangeRequested(offset);
        break;
      }
      centerPoint -= offset;
      Q_EMIT changed();
      break;
    }

    default:
      break;
  }
}
void View3D::wheelEvent(QWheelEvent* event, const QSize& windowSize) {
  float mult = 2;
  if (event->modifiers().testFlag(Qt::ShiftModifier)) mult = 0.2;
  if (event->modifiers().testFlag(Qt::ControlModifier)) {
    if (mouseOperation->canMoveView()) {
      // QVector3D direction(0, 0, 1);
      // Choose direction so that object under the mouse stays the same
      QVector3D direction =
          -(projectionMatrix(windowSize.width(), windowSize.height())
                .inverted() *
            toVector(event->pos(), windowSize))
               .normalized();
      float distance = mult * 10.0f * event->angleDelta().y() / 120.0f;
      QVector3D move = direction * distance;
      move *= pixelSize(windowSize);
      centerPoint -= rotation.rotatedVector(move);
    }
  } else {
    float zoomNew =
        this->zoom * std::exp(mult * 0.1f * event->angleDelta().y() / 120.0f);
    zoomNew = std::min(zoomMax, std::max(zoomMin, zoomNew));

    // Change centerPoint so that the object under the mouse stays the same
    // while zooming
    if (true) {
      if (mouseOperation->canMoveView()) {
        QVector3D p(event->pos().x(), -event->pos().y(), 0);
        p -= QVector3D(windowSize.width(), -windowSize.height(), 0) / 2;
        p *= pixelSize(windowSize);
        centerPoint += rotation.rotatedVector(p * (1 - this->zoom / zoomNew));
      }
    }

    this->zoom = zoomNew;

    Q_EMIT zoomChanged(this->zoom, this->zoomMin, this->zoomMax);
  }
  Q_EMIT changed();
}

/**
 * @brief View3D::keyPressEvent
 * This method handles key presses for the 3d view window
 * Use 1 (Front/Back), 3 (Left/Right) and 7(Top/Bottom) + Ctrl key to set fixed
 * viewing angle Use 4/6 (left/right) and 2/8 (up/down) to turn view in 22.5
 * degree increments This works similar to Blender's viewing system
 */

void View3D::keyPressEvent(QKeyEvent* event, const QSize& windowSize) {
  Q_UNUSED(windowSize);

  // Front/Back View
  if (event->key() == Qt::Key_1) {
    // Use the Ctrl key to toggle between both sides (front and back)
    if (event->modifiers() & Qt::ControlModifier) {
      setFixedAngle("back");
    } else {
      setFixedAngle("front");
    }
  }

  // Right/Left View
  if (event->key() == Qt::Key_3) {
    if (event->modifiers() & Qt::ControlModifier) {
      setFixedAngle("left");
    } else {
      setFixedAngle("right");
    }
  }

  // Top/Bottom View
  if (event->key() == Qt::Key_7) {
    if (event->modifiers() & Qt::ControlModifier) {
      setFixedAngle("bottom");
    } else {
      setFixedAngle("top");
    }
  }

  // This rotates the view in 22.5 degree increments
  if (event->key() == Qt::Key_4) {
    QVector3D rotate = rotation.toEulerAngles();
    rotate.setY(rotate.y() - 22.5f);
    rotation = rotation.fromEulerAngles(rotate);
    Q_EMIT changed();
  } else if (event->key() == Qt::Key_6) {
    QVector3D rotate = rotation.toEulerAngles();
    rotate.setY(rotate.y() + 22.5f);
    rotation = rotation.fromEulerAngles(rotate);
    Q_EMIT changed();
  } else if (event->key() == Qt::Key_2) {
    QVector3D rotate = rotation.toEulerAngles();
    rotate.setX(rotate.x() + 22.5f);
    rotation = rotation.fromEulerAngles(rotate);
    Q_EMIT changed();
  } else if (event->key() == Qt::Key_8) {
    QVector3D rotate = rotation.toEulerAngles();
    rotate.setX(rotate.x() - 22.5f);
    rotation = rotation.fromEulerAngles(rotate);
    Q_EMIT changed();
  }

  if (event->key() == Qt::Key_O) {
    switchProjection();
  }
}

void View3D::setFixedAngle(QString direction) {
  if (direction == "front") {
    rotation = QQuaternion(1, 0, 0, 0);
  } else if (direction == "back") {
    rotation = QQuaternion(0, 0, 1, 0);
  } else if (direction == "right") {
    rotation = QQuaternion(0.707f, 0, 0.707f, 0);
  } else if (direction == "left") {
    rotation = QQuaternion(0.707f, 0, -0.707f, 0);
  } else if (direction == "top") {
    rotation = QQuaternion(0.707f, -0.707f, 0, 0);
  } else if (direction == "bottom") {
    rotation = QQuaternion(0.707f, 0.707f, 0, 0);
  }

  this->zoom = 0.5 * factor;
  centerPoint = QVector3D(0, 0, 0);
  Q_EMIT changed();
}

void View3D::switchProjection() {
  if (fieldOfView == 0)
    fieldOfView = 40 / 180.0 * M_PI;
  else
    fieldOfView = 0;
  Q_EMIT changed();
}

void View3D::move(QVector3D vectorViewSpace) {
  if (!mouseOperation->canMoveView()) return;

  // centerPoint -= rotation.rotatedVector(vectorViewSpace / this->zoom);
  centerPoint -= viewMatrix().inverted().mapVector(vectorViewSpace);

  Q_EMIT changed();
}

void View3D::rotate(QQuaternion rotation) {
  this->rotation =
      this->rotation * QQuaternion(rotation.scalar(), -rotation.vector());
  this->rotation.normalize();

  Q_EMIT changed();
}

void View3D::moveZoom(float value) {
  float zoomNew = this->zoom * std::exp(value);
  zoomNew = std::min(zoomMax, std::max(zoomMin, zoomNew));
  this->zoom = zoomNew;

  Q_EMIT changed();
}

void View3D::setZoom(float zoom) {
  this->zoom = zoom;
  Q_EMIT changed();
}

void View3D::resetView() {
  this->zoom = 1.0 * factor;
  centerPoint = QVector3D(0, 0, 0);
  rotation = QQuaternion(1, 0, 0, 0);
  Q_EMIT changed();
}

void View3D::registerSpaceNavVisualizer(vx::spnav::SpaceNavVisualizer* sn) {
  int interval = 20;

  auto zoomButton = createQSharedPointer<int>();
  *zoomButton = -1;

  auto zoomTimer = new QTimer(sn);
  connect(this, &QObject::destroyed, zoomTimer,
          [zoomTimer] { zoomTimer->deleteLater(); });
  zoomTimer->setInterval(interval);
  zoomTimer->setSingleShot(false);

  connect(zoomTimer, &QTimer::timeout, this, [this, interval, zoomButton] {
    // qDebug() << "timeout";
    float scale = 1;
    scale *= interval / 1000.0f;
    if (*zoomButton == 0)
      moveZoom(-scale);
    else if (*zoomButton == 1)
      moveZoom(scale);
  });

  connect(
      sn, &vx::spnav::SpaceNavVisualizer::motionEvent, this,
      [this, zoomButton](vx::spnav::SpaceNavMotionEvent* ev) {
        // qDebug() << "Event" << this << ev->translation() << ev->rotation();

        move(ev->translation() * 2e-5f);

        QVector3D rotation = ev->rotation() * 1e-4f;
        auto angle = rotation.length();
        if (angle > 1e-4)
          rotate(QQuaternion::fromAxisAndAngle(rotation, angle / M_PI * 180));
      });
  connect(sn, &vx::spnav::SpaceNavVisualizer::buttonPressEvent, this,
          [zoomButton, zoomTimer](vx::spnav::SpaceNavButtonPressEvent* ev) {
            // qDebug() << "Button Press Event" << this << ev->button();
            /*
                if (ev->button() == 0)
                    moveZoom(-1);
                else if (ev->button() == 1)
                    moveZoom(1);
                */
            if (ev->button() == 0) {
              *zoomButton = 0;
              zoomTimer->start();
            } else if (ev->button() == 1) {
              *zoomButton = 1;
              zoomTimer->start();
            }
          });
  connect(sn, &vx::spnav::SpaceNavVisualizer::buttonReleaseEvent, this,
          [zoomButton, zoomTimer](vx::spnav::SpaceNavButtonReleaseEvent* ev) {
            // qDebug() << "Button Release Event" << this << ev->button();
            if (ev->button() == *zoomButton) zoomTimer->stop();
          });
  connect(sn, &vx::spnav::SpaceNavVisualizer::looseFocus, this,
          [zoomButton, zoomTimer] {
            // qDebug() << "Loose Focus Event" << this;
            zoomTimer->stop();
          });
}

void View3D::setFieldOfView(float value) {
  this->fieldOfView = value;
  Q_EMIT changed();
}

void View3D::setViewSizeStandard(float value) {
  this->viewSizeStandard = value;
  Q_EMIT changed();
}

void View3D::setCenterPoint(const QVector3D& value) {
  this->centerPoint = value;
  Q_EMIT changed();
}

void View3D::cameraRotationRequested() {
  this->rotation.normalize();
  Q_EMIT cameraRotationChanged(this->rotation);
}

void View3D::zoomRequested() {
  Q_EMIT zoomChanged(this->zoom, this->zoomMin, this->zoomMax);
}

void View3D::setRotation(QQuaternion rot) {
  this->rotation = rot;
  Q_EMIT changed();
}
