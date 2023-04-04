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

#include "View3DPropertiesConnection.hpp"

#include <Voxie/Vis/View3D.hpp>

#include <Voxie/MathQt.hpp>

#include <PluginVisSlice/Prototypes.hpp>
#include <PluginVisSlice/SliceVisualizer.hpp>

using namespace vx;
using namespace vx::vis_slice;
using namespace vx::visualization;

View3DPropertiesConnection::View3DPropertiesConnection(
    SliceVisualizer* sv, SliceProperties* properties, View3D* view3d)
    : sv(sv), properties(properties), view3d(view3d) {
  QObject::connect(properties, &QObject::destroyed, this,
                   &QObject::deleteLater);
  QObject::connect(view3d, &QObject::destroyed, this, &QObject::deleteLater);

  QObject::connect(this->view3d, &View3D::changed, this,
                   &View3DPropertiesConnection::updateFromView);

  // TODO: Clean up?

  QObject::connect(this->properties, &SliceProperties::originChanged, this,
                   [this](const QVector3D& value) {
                     if (!this->view3d || !this->sv || !this->properties ||
                         suppressForwardToView)
                       return;
                     suppressForwardFromView = true;

                     View3DValues upd;
                     auto center = this->properties->centerPoint();
                     QVector3D center3D(center.x(), center.y(), 0);
                     upd.setLookAt(vectorCast<double>(toVector(
                         value + this->properties->orientation() * center3D)));
                     this->view3d->update(upd);

                     suppressForwardFromView = false;
                   });

  QObject::connect(
      this->properties, &SliceProperties::orientationChanged, this,
      [this](const QQuaternion& value) {
        if (!this->view3d || !this->sv || !this->properties ||
            suppressForwardToView)
          return;
        suppressForwardFromView = true;

        View3DValues upd;
        auto center = this->properties->centerPoint();
        QVector3D center3D(center.x(), center.y(), 0);
        upd.setLookAt(vectorCast<double>(
            toVector(this->properties->origin() + value * center3D)));
        upd.setOrientation(rotationCast<double>(toRotation(value)));
        this->view3d->update(upd);

        suppressForwardFromView = false;
      });

  QObject::connect(this->properties, &SliceProperties::verticalSizeChanged,
                   this, [this](float value) {
                     if (!this->view3d || !this->sv || !this->properties ||
                         suppressForwardToView)
                       return;
                     suppressForwardFromView = true;
                     this->view3d->setZoomLog(-std::log(value));
                     suppressForwardFromView = false;
                   });

  QObject::connect(this->properties, &SliceProperties::centerPointChanged, this,
                   [this](QPointF value) {
                     if (!this->view3d || !this->sv || !this->properties ||
                         suppressForwardToView)
                       return;
                     suppressForwardFromView = true;

                     View3DValues upd;
                     auto center = value;
                     QVector3D center3D(center.x(), center.y(), 0);
                     upd.setLookAt(vectorCast<double>(
                         toVector(this->properties->origin() +
                                  this->properties->orientation() * center3D)));
                     this->view3d->update(upd);

                     suppressForwardFromView = false;
                   });

  // Copy initial values
  suppressForwardFromView = true;
  View3DValues upd;
  upd.setZoomLog(-std::log(this->properties->verticalSize()));
  auto center = this->properties->centerPoint();
  QVector3D center3D(center.x(), center.y(), 0);
  upd.setLookAt(
      vectorCast<double>(toVector(this->properties->origin() +
                                  this->properties->orientation() * center3D)));
  upd.setOrientation(
      rotationCast<double>(toRotation(this->properties->orientation())));
  this->view3d->update(upd);
  suppressForwardFromView = false;
}
View3DPropertiesConnection::~View3DPropertiesConnection() {}

void View3DPropertiesConnection::updateFromView() {
  if (!view3d || !sv || !properties) return;

  if (suppressForwardFromView) return;

  suppressForwardToView = true;

  properties->setVerticalSize(std::exp(-view3d->zoomLog()));

  auto newOrientation = view3d->orientation();
  sv->setRotation(toQQuaternion(rotationCastNarrow<float>(newOrientation)));

  // Project old origin onto new plane
  auto oldOrigin = vectorCast<double>(toVector(properties->origin()));
  auto lookAt = view3d->lookAt();
  auto lookAtPlane = inverse(newOrientation).map(lookAt);
  auto newOriginPlane = inverse(newOrientation).map(oldOrigin);
  newOriginPlane[2] = lookAtPlane[2];
  auto newOrigin = newOrientation.map(newOriginPlane);
  sv->setOrigin(toQVector(vectorCastNarrow<float>(newOrigin)));

  // Calculate new center point
  auto newCenterPoint3D = inverse(newOrientation).map(lookAt - newOrigin);
  // Note: z coordinate should be (almost) 0
  // qDebug() << newCenterPoint3D;
  auto newCenterPoint3DFloat = vectorCastNarrow<float>(newCenterPoint3D);
  properties->setCenterPoint(
      QPointF(newCenterPoint3DFloat[0], newCenterPoint3DFloat[1]));

  suppressForwardToView = false;
}
