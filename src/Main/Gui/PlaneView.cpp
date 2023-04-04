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

#include "PlaneView.hpp"

#include <cmath>

#include <QtGui/QMatrix4x4>
#include <QtGui/QMouseEvent>

#include <QtOpenGL/QGLFormat>

using namespace vx::gui;
using namespace vx;

PlaneView::PlaneView(Slice* slice, QWidget* parent)
    : OpenGLDrawWidget(parent),
      slice(slice),
      view3d(new vx::visualization::View3D(
          this, vx::visualization::View3DProperty::Orientation |
                    vx::visualization::View3DProperty::ZoomLog)) {
  // TODO: Handle changes of the bounding box
  this->view3d->setBoundingBox(this->slice->getDataset()->boundingBox());
  this->view3d->resetView();
  // TODO: Values?
  this->view3d->setZoomLogMinMax(this->view3d->zoomLog() - 2,
                                 this->view3d->zoomLog() + 1);
  this->setMinimumHeight(150);
  QMetaObject::Connection conni =
      connect(this->slice, &QObject::destroyed,
              [this]() -> void { this->slice = nullptr; });
  connect(this, &QObject::destroyed,
          [=]() -> void { this->disconnect(conni); });

  connect(view3d, &vx::visualization::View3D::changed, this,
          [this] { this->update(); });
}

void PlaneView::mousePressEvent(QMouseEvent* event) {
  view3d->mousePressEvent(mouseLast, event, size());
  this->mouseLast = event->pos();
}

void PlaneView::mouseMoveEvent(QMouseEvent* event) {
  int dx = event->x() - this->mouseLast.x();
  int dy = event->y() - this->mouseLast.y();

  if ((event->buttons() & Qt::RightButton) && (this->slice != nullptr)) {
    float ax = 0.12f * dx;
    float ay = 0.12f * dy;

    QQuaternion src = this->slice->rotation();

    QMatrix4x4 matView = toQMatrix4x4(
        matrixCastNarrow<float>(view3d->viewMatrix().projectiveMatrix()));
    matView.setRow(3, QVector4D(0, 0, 0, 1));  // Remove translation

    QQuaternion quatX = QQuaternion::fromAxisAndAngle(
        (QVector4D(0, 1, 0, 0) * matView).toVector3D(), ax);
    QQuaternion quatY = QQuaternion::fromAxisAndAngle(
        (QVector4D(1, 0, 0, 0) * matView).toVector3D(), ay);

    QQuaternion quat = quatX * quatY;

    this->slice->setRotation(quat * src);

    this->update();
  } else {
    view3d->mouseMoveEvent(mouseLast, event, size());
  }
  this->mouseLast = event->pos();
}

void PlaneView::mouseReleaseEvent(QMouseEvent* event) {
  view3d->mouseReleaseEvent(mouseLast, event, size());
  this->mouseLast = event->pos();
}

void PlaneView::wheelEvent(QWheelEvent* event) {
  view3d->wheelEvent(event, size());
}

void PlaneView::paint() {
  glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
  glClearDepthf(1.0f);

  if (this->slice == nullptr) return;

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  QVector3D extends = this->slice->getDataset()->size();
  QVector3D origin = this->slice->getDataset()->origin();

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  PrimitiveBuffer buffer;

  QMatrix4x4 matViewProj = toQMatrix4x4(matrixCastNarrow<float>(
      (view3d->projectionMatrix(this->width(), this->height()) *
       view3d->viewMatrix())
          .projectiveMatrix()));

  QMatrix4x4 transformVolumeObject = matViewProj;
  transformVolumeObject.translate(origin);

  ///////////////////////////////////////////////////////////////////////////////////
  /// Render data set cuboid
  ///////////////////////////////////////////////////////////////////////////////////

  buffer.clear();

  // Front
  buffer.addQuad(QVector3D(0.8f, 0.8f, 0.8f), QVector3D(0, 0, extends.z()),
                 QVector3D(extends.x(), 0, extends.z()),
                 QVector3D(extends.x(), extends.y(), extends.z()),
                 QVector3D(0, extends.y(), extends.z()));

  // Back
  buffer.addQuad(QVector3D(0.4f, 0.4f, 0.4f), QVector3D(0, 0, 0),
                 QVector3D(extends.x(), 0, 0),
                 QVector3D(extends.x(), extends.y(), 0),
                 QVector3D(0, extends.y(), 0));

  // Left
  buffer.addQuad(QVector3D(0.6f, 0.6f, 0.6f), QVector3D(extends.x(), 0, 0),
                 QVector3D(extends.x(), 0, extends.z()),
                 QVector3D(extends.x(), extends.y(), extends.z()),
                 QVector3D(extends.x(), extends.y(), 0));

  // Right
  buffer.addQuad(QVector3D(0.5f, 0.5f, 0.5f), QVector3D(0, 0, 0),
                 QVector3D(0, 0, extends.z()),
                 QVector3D(0, extends.y(), extends.z()),
                 QVector3D(0, extends.y(), 0));

  // Top
  buffer.addQuad(QVector3D(0.7f, 0.7f, 0.7f), QVector3D(0, extends.y(), 0),
                 QVector3D(0, extends.y(), extends.z()),
                 QVector3D(extends.x(), extends.y(), extends.z()),
                 QVector3D(extends.x(), extends.y(), 0));

  // Bottom
  buffer.addQuad(QVector3D(0.3f, 0.3f, 0.3f), QVector3D(0, 0, 0),
                 QVector3D(0, 0, extends.z()),
                 QVector3D(extends.x(), 0, extends.z()),
                 QVector3D(extends.x(), 0, 0));

  draw(buffer, transformVolumeObject);

  ///////////////////////////////////////////////////////////////////////////////////
  /// Render coordinate axis
  ///////////////////////////////////////////////////////////////////////////////////

  buffer.clear();

  buffer.addLine(QVector3D(1.0f, 1.0f, 1.0f), QVector3D(0.0f, 0.0f, 0.0f),
                 QVector3D(2.0f * extends.x(), 0.0f, 0.0f));

  buffer.addLine(QVector3D(1.0f, 1.0f, 1.0f), QVector3D(0.0f, 0.0f, 0.0f),
                 QVector3D(0.0f, 2.0f * extends.y(), 0.0f));

  buffer.addLine(QVector3D(1.0f, 1.0f, 1.0f), QVector3D(0.0f, 0.0f, 0.0f),
                 QVector3D(0.0f, 0.0f, 2.0f * extends.z()));

  draw(buffer, matViewProj);

  ///////////////////////////////////////////////////////////////////////////////////
  /// Render plane
  ///////////////////////////////////////////////////////////////////////////////////

  QMatrix4x4 transformPlane = matViewProj;

  // Adjust origin
  // transformPlane.translate(origin);

  // Slice transformation
  transformPlane.translate(this->slice->origin());
  transformPlane.rotate(this->slice->rotation());

  buffer.clear();

  buffer.addQuad(QVector4D(1.0f, 0.0f, 0.0f, 0.4f),
                 QVector3D(-extends.length(), -extends.length(), 0),
                 QVector3D(-extends.length(), extends.length(), 0),
                 QVector3D(extends.length(), extends.length(), 0),
                 QVector3D(extends.length(), -extends.length(), 0));

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDepthMask(GL_FALSE);

  draw(buffer, transformPlane);

  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);

  buffer.clear();

  buffer.addLine(QVector3D(0.0f, 1.0f, 0.0f), QVector3D(0.0f, 0.0f, 0.0f),
                 QVector3D(extends.length(), 0.0f, 0.0f));

  buffer.addLine(QVector3D(0.0f, 0.0f, 1.0f), QVector3D(0.0f, 0.0f, 0.0f),
                 QVector3D(0.0f, extends.length(), 0.0f));

  buffer.addLine(QVector3D(1.0f, 0.0f, 0.0f), QVector3D(0.0f, 0.0f, 0.0f),
                 QVector3D(0.0f, 0.0f, extends.length()));

  draw(buffer, transformPlane);
}
