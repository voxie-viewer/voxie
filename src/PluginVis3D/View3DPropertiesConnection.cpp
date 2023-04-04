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

#include <PluginVis3D/Prototypes.hpp>

using namespace vx;
using namespace vx::vis3d;
using namespace vx::visualization;

View3DPropertiesConnection::View3DPropertiesConnection(
    View3DProperties* properties, View3D* view3d)
    : properties(properties), view3d(view3d) {
  QObject::connect(properties, &QObject::destroyed, this,
                   &QObject::deleteLater);
  QObject::connect(view3d, &QObject::destroyed, this, &QObject::deleteLater);

  QObject::connect(this->view3d, &View3D::changed, this,
                   &View3DPropertiesConnection::updateFromView);

  QObject::connect(
      this->properties, &View3DProperties::fieldOfViewChanged, this,
      [this](float value) {
        if (!this->view3d || !this->properties || suppressForwardToView) return;
        suppressForwardFromView = true;
        this->view3d->setFieldOfView(value);
        suppressForwardFromView = false;
      });

  QObject::connect(
      this->properties, &View3DProperties::viewSizeUnzoomedChanged, this,
      [this](float value) {
        if (!this->view3d || !this->properties || suppressForwardToView) return;
        suppressForwardFromView = true;
        this->view3d->setViewSizeUnzoomed(value);
        suppressForwardFromView = false;
      });

  QObject::connect(
      this->properties, &View3DProperties::zoomLogChanged, this,
      [this](float value) {
        if (!this->view3d || !this->properties || suppressForwardToView) return;
        suppressForwardFromView = true;
        this->view3d->setZoomLog(value);
        suppressForwardFromView = false;
      });

  QObject::connect(
      this->properties, &View3DProperties::lookAtChanged, this,
      [this](const QVector3D& value) {
        if (!this->view3d || !this->properties || suppressForwardToView) return;
        suppressForwardFromView = true;
        this->view3d->setLookAt(vectorCast<double>(toVector(value)));
        suppressForwardFromView = false;
      });

  QObject::connect(
      this->properties, &View3DProperties::orientationChanged, this,
      [this](const QQuaternion& value) {
        if (!this->view3d || !this->properties || suppressForwardToView) return;
        suppressForwardFromView = true;
        this->view3d->setOrientation(rotationCast<double>(toRotation(value)));
        suppressForwardFromView = false;
      });

  // Copy initial values
  suppressForwardFromView = true;
  this->view3d->setFieldOfView(this->properties->fieldOfView());
  this->view3d->setViewSizeUnzoomed(this->properties->viewSizeUnzoomed());
  this->view3d->setZoomLog(this->properties->zoomLog());
  this->view3d->setLookAt(
      vectorCast<double>(toVector(this->properties->lookAt())));
  this->view3d->setOrientation(
      rotationCast<double>(toRotation(this->properties->orientation())));
  suppressForwardFromView = false;
}
View3DPropertiesConnection::~View3DPropertiesConnection() {}

void View3DPropertiesConnection::updateFromView() {
  if (!view3d || !properties) return;

  if (suppressForwardFromView) return;

  // TODO: set property values initially?

  suppressForwardToView = true;
  {
    auto val = view3d->fieldOfView();
    auto valOld = properties->fieldOfView();
    if (val != valOld) properties->setFieldOfView(val);
  }
  {
    auto val = view3d->viewSizeUnzoomed();
    auto valOld = properties->viewSizeUnzoomed();
    if (val != valOld) properties->setViewSizeUnzoomed(val);
  }
  {
    auto val = view3d->zoomLog();
    auto valOld = properties->zoomLog();
    if (val != valOld) properties->setZoomLog(val);
  }
  {
    auto val = toQVector(vectorCastNarrow<float>(view3d->lookAt()));
    auto valOld = properties->lookAt();
    if (val != valOld) properties->setLookAt(val);
  }
  {
    auto val = toQQuaternion(rotationCastNarrow<float>(view3d->orientation()));
    auto valOld = properties->orientation();
    if (val != valOld) properties->setOrientation(val);
  }
  suppressForwardToView = false;
}
