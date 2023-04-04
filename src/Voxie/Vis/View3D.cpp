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

#include <Voxie/DebugOptions.hpp>
#include <Voxie/MathQt.hpp>

#include <QtCore/QSharedPointer>
#include <QtCore/QTimer>

using namespace vx::visualization;

// Give a mouse position / window size, return a vector with x, y in [-1;1]
static vx::Vector<double, 3> toVector(const QPoint& pos, const QSize& size) {
  return vx::Vector<double, 3>(2.0 * (pos.x() + 0.5) / size.width() - 1.0,
                               1.0 - 2.0 * (pos.y() + 0.5) / size.height(), 0);
}

// Information about arcball:
// https://en.wikibooks.org/w/index.php?title=OpenGL_Programming/Modern_OpenGL_Tutorial_Arcball&oldid=2356903
static vx::Vector<double, 3> getArcballVector(const QPoint& pos,
                                              const QSize& size) {
  vx::Vector<double, 3> p = toVector(pos, size);
  if (squaredNorm(p) < 1)
    p.access<2>() = std::sqrt(1 - squaredNorm(p));  // Pythagore
  else
    p = normalize(p);  // nearest point
  return p;
}

namespace vx {
namespace visualization {
View3DValues::View3DValues()
    : valid(View3DProperty::None),
      lookAt({std::numeric_limits<double>::quiet_NaN(),
              std::numeric_limits<double>::quiet_NaN(),
              std::numeric_limits<double>::quiet_NaN()}),
      orientation({1, 0, 0, 0}),
      zoomLog(std::numeric_limits<double>::quiet_NaN()),
      viewSizeUnzoomed(std::numeric_limits<double>::quiet_NaN()),
      fieldOfView(std::numeric_limits<double>::quiet_NaN()) {}

void View3DValues::setLookAt(const vx::Vector<double, 3>& value) {
  valid |= View3DProperty::LookAt;
  this->lookAt = value;
}
void View3DValues::setOrientation(const vx::Rotation<double, 3>& value) {
  valid |= View3DProperty::Orientation;
  this->orientation = value;
}
void View3DValues::setZoomLog(double value) {
  valid |= View3DProperty::ZoomLog;
  this->zoomLog = value;
}
void View3DValues::setViewSizeUnzoomed(double value) {
  valid |= View3DProperty::ViewSizeUnzoomed;
  this->viewSizeUnzoomed = value;
}
void View3DValues::setFieldOfView(double value) {
  valid |= View3DProperty::FieldOfView;
  this->fieldOfView = value;
}

void View3DValues::updateFrom(const View3DValues& values,
                              View3DProperty toCopy) {
  if ((~values.valid & toCopy) != View3DProperty::None) {
    qWarning() << "View3DValues::updateFrom: Copying invalid values"
               << (uint32_t)values.valid << (uint32_t)toCopy;
  }

  if ((toCopy & View3DProperty::LookAt) == View3DProperty::LookAt)
    this->setLookAt(values.lookAt);
  if ((toCopy & View3DProperty::Orientation) == View3DProperty::Orientation)
    this->setOrientation(values.orientation);
  if ((toCopy & View3DProperty::ZoomLog) == View3DProperty::ZoomLog)
    this->setZoomLog(values.zoomLog);
  if ((toCopy & View3DProperty::ViewSizeUnzoomed) ==
      View3DProperty::ViewSizeUnzoomed)
    this->setViewSizeUnzoomed(values.viewSizeUnzoomed);
  if ((toCopy & View3DProperty::FieldOfView) == View3DProperty::FieldOfView)
    this->setFieldOfView(values.fieldOfView);
}
void View3DValues::updateFrom(const View3DValues& values) {
  this->updateFrom(values, values.valid);
}

class MouseAction {
  View3D* view_;
  QPoint pressEventPos_;
  QSize pressWindowSize_;

  View3DValues origValues;

 public:
  MouseAction(View3D* view, QMouseEvent* pressEvent,
              const QSize& pressWindowSize)
      : view_(view),
        pressEventPos_(pressEvent->pos()),
        pressWindowSize_(pressWindowSize) {}
  virtual ~MouseAction() {}

  static QSharedPointer<MouseAction> create(View3D* view,
                                            QMouseEvent* pressEvent,
                                            const QSize& pressWindowSize);

  const View3D* view() { return view_; }
  const QPoint& pressEventPos() { return pressEventPos_; }
  const QSize& pressWindowSize() { return pressWindowSize_; }

  void finish() {}

  void revert() {
    // qDebug() << "MouseAction::revert()";
    view_->update(origValues);
  }

  virtual void mouseMoveEvent(const QPoint& mouseLast, QMouseEvent* event,
                              const QSize& windowSize) = 0;

 protected:
  // TODO: Check whether the action was finished / reverted?
  void update(const View3DValues& values) {
    auto toBackup = values.valid & ~origValues.valid;
    origValues.updateFrom(view_->values(), toBackup);

    view_->update(values);
  }

  // Convinience functions
  void setLookAt(const vx::Vector<double, 3>& value) {
    View3DValues upd;
    upd.setLookAt(value);
    this->update(upd);
  }
  void setOrientation(const vx::Rotation<double, 3>& value) {
    View3DValues upd;
    upd.setOrientation(value);
    this->update(upd);
  }
  void setZoomLog(double value) {
    View3DValues upd;
    upd.setZoomLog(value);
    this->update(upd);
  }
};

class MouseActionPan : public MouseAction {
 public:
  MouseActionPan(View3D* view, QMouseEvent* pressEvent,
                 const QSize& pressWindowSize)
      : MouseAction(view, pressEvent, pressWindowSize) {}

  void mouseMoveEvent(const QPoint& mouseLast, QMouseEvent* event,
                      const QSize& windowSize) override {
    // qDebug() << "Pan mouseMoveEvent" << mouseLast << event;

    double factor = 1.0;
    if (event->modifiers().testFlag(Qt::AltModifier)) factor = 0.1;

    int dxInt = event->x() - mouseLast.x();
    int dyInt = event->y() - mouseLast.y();

    if (dxInt == 0 && dyInt == 0) return;

    double dx = dxInt * factor;
    double dy = dyInt * factor;

    vx::Vector<double, 3> move(dx, -dy, 0);
    move *= view()->pixelSize(windowSize);
    auto rotation = view()->orientation();
    auto offset = rotation.map(move);
    auto lookAt = view()->lookAt();
    lookAt -= offset;
    this->setLookAt(lookAt);
  }
  };  // namespace vx

class MouseActionPanZ : public MouseAction {
 public:
  MouseActionPanZ(View3D* view, QMouseEvent* pressEvent,
                  const QSize& pressWindowSize)
      : MouseAction(view, pressEvent, pressWindowSize) {}

  // TODO: Combine code with wheel pan code?
  void mouseMoveEvent(const QPoint& mouseLast, QMouseEvent* event,
                      const QSize& windowSize) override {
    // qDebug() << "Pan mouseMoveEvent" << mouseLast << event;

    double factor = 1.0;
    if (event->modifiers().testFlag(Qt::AltModifier)) factor = 0.1;

    int dyInt = event->y() - mouseLast.y();

    if (dyInt == 0) return;

    double dy = dyInt * factor;

    // Choose direction so that object under the mouse stays the same
    // Calculate projection*view matrix
    vx::ProjectiveMap<double, 3> projectionView =
        view()->projectionMatrix(pressWindowSize().width(),
                                 pressWindowSize().height()) *
        view()->viewMatrix();
    // Invert matrix
    vx::ProjectiveMap<double, 3> projectionViewInv = inverse(projectionView);
    // Invert vector from mouse pos into the direction -1 in Z direction
    vx::Vector<double, 3> mousePos =
        toVector(pressEventPos(), pressWindowSize());
    vx::Vector<double, 3> mousePos2 =
        mousePos + vx::Vector<double, 3>(0, 0, -1);
    vx::Vector<double, 3> direction = normalize(
        projectionViewInv.map(mousePos2) - projectionViewInv.map(mousePos));
    vx::Vector<double, 3> move = direction * -dy;

    move *= view()->pixelSize(windowSize);
    auto lookAt = view()->lookAt();
    lookAt -= move;
    this->setLookAt(lookAt);
  }
};

class MouseActionRotate : public MouseAction {
 public:
  MouseActionRotate(View3D* view, QMouseEvent* pressEvent,
                    const QSize& pressWindowSize)
      : MouseAction(view, pressEvent, pressWindowSize) {}

  void mouseMoveEvent(const QPoint& mouseLast, QMouseEvent* event,
                      const QSize& windowSize) override {
    // qDebug() << "Rotate mouseMoveEvent" << mouseLast << event;

    double factor = 1.0;
    if (event->modifiers().testFlag(Qt::AltModifier)) factor = 0.1;

    vx::Vector<double, 3> va = getArcballVector(mouseLast, windowSize);
    vx::Vector<double, 3> vb = getArcballVector(event->pos(), windowSize);
    double angle = std::acos(std::min(1.0, dotProduct(va, vb)));
    vx::Vector<double, 3> axis = crossProduct(va, vb);
    angle = angle * factor;
    auto rotation = view()->orientation();
    rotation *= vx::rotationFromAxisAngle(axis, -angle);
    this->setOrientation(rotation);
  }
};

class MouseActionZoom : public MouseAction {
 public:
  MouseActionZoom(View3D* view, QMouseEvent* pressEvent,
                  const QSize& pressWindowSize)
      : MouseAction(view, pressEvent, pressWindowSize) {}

  // TODO: Combine code with wheel zoom code?
  void mouseMoveEvent(const QPoint& mouseLast, QMouseEvent* event,
                      const QSize& windowSize) override {
    // qDebug() << "Rotate mouseMoveEvent" << mouseLast << event;

    double factor = 1.0;
    if (event->modifiers().testFlag(Qt::AltModifier)) factor = 0.1;

    int dyInt = event->y() - mouseLast.y();

    if (dyInt == 0) return;

    double dy = dyInt * factor;

    double zoomLogNew =
        view()->limitZoomLog(view()->zoomLog() + view()->mouseActionZoomFactor *
                                                     -dy / windowSize.height());

    View3DValues upd;
    // Change lookAt so that the object under the mouse when the button was
    // pressed stays the same while zooming
    if (true) {
      vx::Vector<double, 3> p(pressEventPos().x(), -pressEventPos().y(), 0);
      p -= vx::Vector<double, 3>(pressWindowSize().width(),
                                 -pressWindowSize().height(), 0) /
           2;
      p *= view()->pixelSize(pressWindowSize());
      auto lookAt = view()->lookAt();
      lookAt += view()->orientation().map(
          p * (1 - std::exp(view()->zoomLog() - zoomLogNew)));
      upd.setLookAt(lookAt);
    }

    upd.setZoomLog(zoomLogNew);
    this->update(upd);
  }
};

QSharedPointer<MouseAction> MouseAction::create(View3D* view,
                                                QMouseEvent* pressEvent,
                                                const QSize& pressWindowSize) {
  auto modifiers =
      pressEvent->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier);

  if (modifiers == Qt::NoModifier)
    return createQSharedPointer<MouseActionPan>(view, pressEvent,
                                                pressWindowSize);
  else if (modifiers == Qt::ShiftModifier)
    return createQSharedPointer<MouseActionRotate>(view, pressEvent,
                                                   pressWindowSize);
  else if (modifiers == Qt::ControlModifier)
    return createQSharedPointer<MouseActionZoom>(view, pressEvent,
                                                 pressWindowSize);
  else if (modifiers == (Qt::ShiftModifier | Qt::ControlModifier))
    return createQSharedPointer<MouseActionPanZ>(view, pressEvent,
                                                 pressWindowSize);
  return QSharedPointer<MouseAction>();
}
}  // namespace visualization
}  // namespace vx

View3D::View3D(QObject* parent, View3DProperty changable,
               const View3DValues& initial)
    : QObject(parent), changable_(changable) {
  this->setZoomLogUnlimited();

  // Note: For the 3D visualizer these initial values will be overwritten
  // with values from the properties
  values_.valid = View3DProperty::All;
  values_.fieldOfView = 40 / 180.0 * M_PI;
  values_.lookAt = {0, 0, 0};
  values_.orientation = vx::identityRotation<double>();
  values_.viewSizeUnzoomed = 0.25;
  values_.zoomLog = 0;

  // TODO: What should be the default here?
  this->setBoundingBox(BoundingBox3D::point({-1, -1, -1}) +
                       BoundingBox3D::point({1, 1, 1}));

  // Note: This will also overwrite values which are not changable
  values_.updateFrom(initial);
}

vx::AffineMap<double, 3> View3D::viewMatrix() const {
  auto matView = vx::createScaling<3>(std::exp(this->zoomLog())) *
                 inverse(this->orientation()) *
                 vx::createTranslation(-this->lookAt());
  // qDebug() << "matView" << matView;
  return matView;
}

vx::ProjectiveMap<double, 3> View3D::projectionMatrix(double fWidth,
                                                      double fHeight) const {
  auto matProj = vx::Matrix<double, 4>::zero();

  // Near and far plane should be at most 1000 times the view size
  double near = -1000 * this->viewSizeUnzoomed();
  double far = 1000 * this->viewSizeUnzoomed();

  // Calculate perspective projection, but allow fieldOfView to be 0 (which
  // will be the same as orthographic projection)

  // TODO: handle negative fieldOfView values?

  double sine = std::sin(this->fieldOfView() / 2.0);
  double cosine = std::cos(this->fieldOfView() / 2.0);

  // Distance from lookAt to camera
  double distanceCameraCenterSine = 0.5 * viewSizeUnzoomed() * cosine;

  // near plane is at least 1/10 of camera <-> center distance
  // near = std::max(near, distanceCameraCenter * 0.1 -
  // distanceCameraCenter);
  // Because distanceCameraCenterSine should be positive this condition should
  // never be true if sine is 0
  if (near * sine < distanceCameraCenterSine * 0.1 - distanceCameraCenterSine)
    near = (distanceCameraCenterSine * 0.1 - distanceCameraCenterSine) / sine;

  // matProj.perspective(this->fieldOfView() / M_PI * 180, fWidth / fHeight,
  // near + distanceCameraCenterSine/sine, far + distanceCameraCenterSine/sine);
  // matProj.translate(0, 0, -distanceCameraCenter);
  // qDebug() << "A" << matProj;
  double clip = far - near;
  // Same as matrix produced by QMatrix4x4::perspective(), but with a
  // translate by (0, 0, -distanceCameraCenter) before it and the entire
  // matrix multiplied with sine
  matProj(0, 0) = cosine / fWidth * fHeight;
  matProj(1, 1) = cosine;
  matProj(2, 2) =
      -(near * sine + far * sine + 2 * distanceCameraCenterSine) / clip;
  // matProj(2, 3) = -(2.0 * near + distanceCameraCenter * far +
  // distanceCameraCenter) / clip * sine;
  /*
  matProj(2, 3) = (-distanceCameraCenterSine) *
                      (-(near + far + 2 * distanceCameraCenter) / clip) +
                  -(2.0 * (near * sine + distanceCameraCenterSine) *
                    (far + distanceCameraCenter)) /
                      clip;
  */
  matProj(2, 3) = (-2.0 * near * far * sine - near * distanceCameraCenterSine -
                   far * distanceCameraCenterSine) /
                  clip;
  matProj(3, 2) = -1.0 * sine;
  // matProj(3, 3) = 0.0 * sine;
  matProj(3, 3) = (-distanceCameraCenterSine) * (-1.0);
  // qDebug() << "B" << matProj;

  // matProj /= matProj.row(3).length();

  // qDebug() << near / this->viewSizeUnzoomed() << far /
  // this->viewSizeUnzoomed();
  // qDebug() << matProj;
  // qDebug() << matProj / matProj.row(3).length();
  return createProjectiveMap(matProj);
}

double View3D::limitZoomLog(double zoomLog) const {
  // TODO: Set zoomLogMin based on bounding box? (This would mean that the zoom
  // might be updated when the bounding box is changed)

  return std::min(this->zoomLogMax(), std::max(this->zoomLogMin(), zoomLog));
}

vx::HmgVector<double, 3> View3D::getCameraPosition() {
  // Distance from lookAt to camera
  double sine = std::sin(this->fieldOfView() / 2.0);
  double cosine = std::cos(this->fieldOfView() / 2.0);
  double distanceCameraCenterSine = 0.5 * viewSizeUnzoomed() * cosine;

  vx::Vector<double, 3> cameraOffset = (this->orientation().map(
      vx::Vector<double, 3>(0, 0, distanceCameraCenterSine)));

  return vx::HmgVector<double, 3>(
      this->lookAt() * (std::exp(this->zoomLog()) * sine) + cameraOffset,
      (std::exp(this->zoomLog()) * sine));
}

void View3D::mousePressEvent(const QPoint& mouseLast, QMouseEvent* event,
                             const QSize& windowSize) {
  Q_UNUSED(mouseLast);
  Q_UNUSED(windowSize);

  // qDebug() << "mousePressEvent" << event;

  // Ignore all non-middle button presses
  if (event->button() != Qt::MiddleButton) return;

  // Ignore all button presses if another button is already pressed
  if ((event->buttons() & (Qt::LeftButton | Qt::RightButton)) != Qt::NoButton)
    return;

  // qDebug() << "Got mouse press" << modifiers;

  if (currentMouseAction_) {
    // Should not happen
    qWarning() << "Got mouse action replacing current mouse action";
    currentMouseAction_->finish();
    currentMouseAction_.reset();
  }
  currentMouseAction_ = MouseAction::create(this, event, windowSize);
}

void View3D::mouseMoveEvent(const QPoint& mouseLast, QMouseEvent* event,
                            const QSize& windowSize) {
  // qDebug() << "mouseMoveEvent" << event;

  if (currentMouseAction_) {
    currentMouseAction_->mouseMoveEvent(mouseLast, event, windowSize);
  }
}
void View3D::mouseReleaseEvent(const QPoint& mouseLast, QMouseEvent* event,
                               const QSize& windowSize) {
  Q_UNUSED(mouseLast);
  Q_UNUSED(windowSize);

  if (currentMouseAction_ && event->button() == Qt::MiddleButton) {
    currentMouseAction_->finish();
    currentMouseAction_.reset();
  }
}

void View3D::wheelEvent(QWheelEvent* event, const QSize& windowSize) {
  // Ignore wheel events while the middle mouse button is pressed
  if ((event->buttons() & Qt::MiddleButton) != 0) {
    return;
  }

  double wheelAngle;
  double mult = 1;

  if (event->modifiers().testFlag(Qt::AltModifier)) {
    mult = 0.1;
    wheelAngle = event->angleDelta().x() / 8.0;
  } else {
    wheelAngle = event->angleDelta().y() / 8.0;
  }
  // qDebug() << "wheelEvent" << event->angleDelta() << wheelAngle
  //          << event->modifiers();

  if (event->modifiers().testFlag(Qt::ShiftModifier)) {
    double distance =
        mult * wheelPanFactor * wheelAngle / 360 * viewSizeZoomed()
;
    // Choose direction so that object under the mouse stays the same
    // Calculate projection*view matrix
    vx::ProjectiveMap<double, 3> projectionView =
        this->projectionMatrix(windowSize.width(), windowSize.height()) *
        this->viewMatrix();
    // Invert matrix
    vx::ProjectiveMap<double, 3> projectionViewInv = inverse(projectionView);
    // Invert vector from mouse pos into the direction -1 in Z direction
    vx::Vector<double, 3> mousePos = toVector(event->pos(), windowSize);
    vx::Vector<double, 3> mousePos2 =
        mousePos + vx::Vector<double, 3>(0, 0, -1);
    vx::Vector<double, 3> direction = normalize(
        projectionViewInv.map(mousePos2) - projectionViewInv.map(mousePos));
    vx::Vector<double, 3> move = direction * distance;

    // move *= this->pixelSize(windowSize);
    auto lookAt = this->lookAt();
    lookAt -= move;
    this->setLookAt(lookAt);
  } else {
    double zoomLogNew = this->limitZoomLog(
        this->zoomLog() + mult * wheelZoomFactor * wheelAngle / 360);

    View3DValues upd;
    // Change lookAt so that the object under the mouse stays the same
    // while zooming
    if (true) {
      vx::Vector<double, 3> p(event->pos().x(), -event->pos().y(), 0);
      p -= vx::Vector<double, 3>(windowSize.width(), -windowSize.height(), 0) /
           2;
      p *= pixelSize(windowSize);
      upd.setLookAt(this->lookAt() +
                    this->orientation().map(
                        p * (1 - std::exp(this->zoomLog() - zoomLogNew))));
    }

    upd.setZoomLog(zoomLogNew);

    this->update(upd);
  }
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

  // qDebug() << event->key() << event->modifiers();
  bool isKeypad = (event->modifiers() & Qt::KeypadModifier) != 0;
  bool hasShift = (event->modifiers() & Qt::ShiftModifier) != 0;
  bool hasCtrl = (event->modifiers() & Qt::ControlModifier) != 0;

  double factor = 1.0;
  if (event->modifiers().testFlag(Qt::AltModifier)) factor = 0.1;

  // Note: move() will apply the zoom later on
  double panDistance = viewSizeUnzoomed() * keyPanFactor * factor;

  // Front/Back View
  if (isKeypad && (event->key() == Qt::Key_1 || event->key() == Qt::Key_End)) {
    // Use the Ctrl key to toggle between both sides (front and back)
    if (hasCtrl) {
      setFixedAngle("back");
    } else {
      setFixedAngle("front");
    }
  }

  // Right/Left View
  if (isKeypad &&
      (event->key() == Qt::Key_3 || event->key() == Qt::Key_PageDown)) {
    if (hasCtrl) {
      setFixedAngle("left");
    } else {
      setFixedAngle("right");
    }
  }

  // Top/Bottom View
  if (isKeypad && (event->key() == Qt::Key_7 || event->key() == Qt::Key_Home)) {
    if (hasCtrl) {
      setFixedAngle("bottom");
    } else {
      setFixedAngle("top");
    }
  }

  if (!hasCtrl) {
    // This rotates the view in keyRotateAmountDeg degree increments
    if (isKeypad &&
        (event->key() == Qt::Key_4 || event->key() == Qt::Key_Left)) {
      if (!hasShift) {
        this->setOrientation(this->orientation() *
                             vx::rotationFromAxisAngleDeg(
                                 {0, -1, 0}, keyRotateAmountDeg * factor));
      } else {
        this->setOrientation(this->orientation() *
                             vx::rotationFromAxisAngleDeg(
                                 {0, 0, -1}, keyRotateAmountDeg * factor));
      }
    } else if (isKeypad &&
               (event->key() == Qt::Key_6 || event->key() == Qt::Key_Right)) {
      if (!hasShift) {
        this->setOrientation(this->orientation() *
                             vx::rotationFromAxisAngleDeg(
                                 {0, 1, 0}, keyRotateAmountDeg * factor));
      } else {
        this->setOrientation(this->orientation() *
                             vx::rotationFromAxisAngleDeg(
                                 {0, 0, 1}, keyRotateAmountDeg * factor));
      }
    } else if (isKeypad &&
               (event->key() == Qt::Key_2 || event->key() == Qt::Key_Down)) {
      if (!hasShift) {
        this->setOrientation(this->orientation() *
                             vx::rotationFromAxisAngleDeg(
                                 {1, 0, 0}, keyRotateAmountDeg * factor));
      }
    } else if (isKeypad &&
               (event->key() == Qt::Key_8 || event->key() == Qt::Key_Up)) {
      if (!hasShift) {
        this->setOrientation(this->orientation() *
                             vx::rotationFromAxisAngleDeg(
                                 {-1, 0, 0}, keyRotateAmountDeg * factor));
      }
    }
  } else {
    // Pan view
    vx::Vector<double, 3> move(0, 0, 0);
    if (isKeypad &&
        (event->key() == Qt::Key_4 || event->key() == Qt::Key_Left)) {
      if (!hasShift) {
        move = {panDistance, 0, 0};
      } else {
      }
    } else if (isKeypad &&
               (event->key() == Qt::Key_6 || event->key() == Qt::Key_Right)) {
      if (!hasShift) {
        move = vx::Vector<double, 3>(-panDistance, 0, 0);
      } else {
      }
    } else if (isKeypad &&
               (event->key() == Qt::Key_2 || event->key() == Qt::Key_Down)) {
      if (!hasShift) {
        move = vx::Vector<double, 3>(0, panDistance, 0);
      }
    } else if (isKeypad &&
               (event->key() == Qt::Key_8 || event->key() == Qt::Key_Up)) {
      if (!hasShift) {
        move = vx::Vector<double, 3>(0, -panDistance, 0);
      }
    }

    if (move != vx::Vector<double, 3>(0, 0, 0)) {
      this->move(move);
    }
  }

  if ((event->key() == Qt::Key_O) ||
      (isKeypad &&
       (event->key() == Qt::Key_5 || event->key() == Qt::Key_Clear))) {
    switchProjection();
  }

  if ((!isKeypad && event->key() == Qt::Key_Home) ||
      (isKeypad &&
       (event->key() == Qt::Key_0 || event->key() == Qt::Key_Insert))) {
    // Move to center and reset zoom
    this->resetView(View3DProperty::LookAt | View3DProperty::ZoomLog);
  }

  if (isKeypad &&
      (event->key() == Qt::Key_9 || event->key() == Qt::Key_PageUp)) {
    this->setOrientation(this->orientation() *
                         vx::rotationFromAxisAngleDeg({0, 1, 0}, 180.0));
  }

  if (isKeypad && (event->key() == Qt::Key_Plus)) {
    if (!hasCtrl) {
      this->setZoomLog(limitZoomLog(this->zoomLog() + keyZoomAmount * factor));
    } else {
      this->move({0, 0, panDistance});
    }
  }
  if (isKeypad && (event->key() == Qt::Key_Minus)) {
    if (!hasCtrl) {
      this->setZoomLog(
          this->limitZoomLog(this->zoomLog() - keyZoomAmount * factor));
    } else {
      this->move({0, 0, -panDistance});
    }
  }

  if (event->key() == Qt::Key_Escape) {
    if (currentMouseAction_) {
      currentMouseAction_->revert();
      currentMouseAction_.reset();
    }
  }
}

void View3D::setFixedAngle(QString direction) {
  if (direction == "front") {
    this->setOrientation(vx::Rotation<double, 3>({1, 0, 0, 0}));
  } else if (direction == "back") {
    this->setOrientation(vx::Rotation<double, 3>({0, 0, 1, 0}));
  } else if (direction == "right") {
    this->setOrientation(vx::Rotation<double, 3>({0.707, 0, 0.707, 0}));
  } else if (direction == "left") {
    this->setOrientation(vx::Rotation<double, 3>({0.707, 0, -0.707, 0}));
  } else if (direction == "top") {
    this->setOrientation(vx::Rotation<double, 3>({0.707, -0.707, 0, 0}));
  } else if (direction == "bottom") {
    this->setOrientation(vx::Rotation<double, 3>({0.707, 0.707, 0, 0}));
  }
}

void View3D::switchProjection() {
  if (this->fieldOfView() == 0)
    this->setFieldOfView(40 / 180.0 * M_PI);
  else
    this->setFieldOfView(0);
}

void View3D::move(const vx::Vector<double, 3>& vectorViewSpace) {
  // lookAt -= this->orientation().map(vectorViewSpace / this->zoom);
  this->setLookAt(this->lookAt() -
                  inverse(viewMatrix()).mapVector(vectorViewSpace));
}

void View3D::rotate(vx::Rotation<double, 3> rotation) {
  this->setOrientation(this->orientation() * inverse(rotation));
}

void View3D::moveZoom(double value) {
  this->setZoomLog(this->limitZoomLog(this->zoomLog() + value));
}

void View3D::setBoundingBox(const vx::BoundingBox3D& boundingBox) {
  // TODO: handle empty bounding boxes?
  this->boundingBox_ = boundingBox;

  // TODO: If there is a minimum zoom depending on the boundingBox then the zoom
  // value might have to be updated here
}

void View3D::setZoomLogMinMax(double zoomLogMin, double zoomLogMax) {
  if (zoomLogMax < zoomLogMin) {
    qWarning() << "View3D::setZoomLogMinMax: zoomLogMax < zoomLogMin";
    return;
  }
  // Above outside -70..70 std::exp(zoomLog) might overflow in single precision
  zoomLogMin_ = std::max(-70.0, zoomLogMin);
  zoomLogMax_ = std::min(70.0, zoomLogMax);
}
void View3D::setZoomLogUnlimited() {
  this->setZoomLogMinMax(-std::numeric_limits<double>::infinity(),
                         +std::numeric_limits<double>::infinity());
}

void View3D::resetView(View3DProperty toReset) {
  View3DValues upd;

  if ((toReset & View3DProperty::Orientation) == View3DProperty::Orientation)
    upd.setOrientation(vx::Rotation<double, 3>({1, 0, 0, 0}));

  if ((toReset & (View3DProperty::ZoomLog | View3DProperty::LookAt)) !=
      View3DProperty::None) {
    auto bb = boundingBox();
    if (bb.isEmpty()) {
      qWarning() << "View3D::resetView(): empty bounding box";
    } else {
      auto size = bb.max() - bb.min();
      auto diag = size.length();
      if ((toReset & View3DProperty::ZoomLog) == View3DProperty::ZoomLog)
        // Set zoom value so that the view size is the same as the diagonal of
        // the bounding box
        // qDebug() << bb.min() << bb.max() << size << diag <<
        // this->viewSizeUnzoomed();
        upd.setZoomLog(
            this->limitZoomLog(-std::log(diag / this->viewSizeUnzoomed())));
      if ((toReset & View3DProperty::LookAt) == View3DProperty::LookAt) {
        auto pos = vectorCast<double>(toVector((bb.max() + bb.min()) / 2));
        if (resetKeepViewZ) {
          auto orient = this->orientation();
          if ((upd.valid & View3DProperty::Orientation) ==
              View3DProperty::Orientation)
            orient = upd.orientation;
          auto posViewOld = inverse(orient).map(this->lookAt());
          auto posView = inverse(orient).map(pos);
          posView[2] = posViewOld[2];
          if (true) {
            // Make sure z value is inside bounding box
            auto bbView = BoundingBox3D::empty();
            for (const auto& corner : bb.corners())
              bbView += BoundingBox3D::pointV(inverse(orient).map(corner));
            posView[2] =
                std::max((double)bbView.min()[2],
                         std::min((double)bbView.max()[2], posView[2]));
          }
          pos = orient.map(posView);
        }
        upd.setLookAt(pos);
      }
    }
  }

  this->update(upd);
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
    double scale = 1;
    scale *= interval / 1000.0;
    if (*zoomButton == 0)
      moveZoom(-scale);
    else if (*zoomButton == 1)
      moveZoom(scale);
  });

  connect(sn, &vx::spnav::SpaceNavVisualizer::motionEvent, this,
          [this, zoomButton](vx::spnav::SpaceNavMotionEvent* ev) {
            // qDebug() << "Event" << this << ev->translation() <<
            // ev->rotation();

            move(vectorCast<double>(toVector(ev->translation())) * 2e-5);

            vx::Vector<double, 3> rotation =
                vectorCast<double>(toVector(ev->rotation())) * 1e-4;
            auto angle = sqrt(squaredNorm(rotation));
            if (angle > 1e-4)
              rotate(vx::rotationFromAxisAngle(rotation, angle));
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

void View3D::setLookAt(const vx::Vector<double, 3>& value) {
  View3DValues upd;
  upd.setLookAt(value);
  this->update(upd);
}

void View3D::setOrientation(const vx::Rotation<double, 3>& value) {
  View3DValues upd;
  upd.setOrientation(value);
  this->update(upd);
}

void View3D::setZoomLog(double value) {
  View3DValues upd;
  upd.setZoomLog(value);
  this->update(upd);
}

void View3D::setViewSizeUnzoomed(double value) {
  View3DValues upd;
  upd.setViewSizeUnzoomed(value);
  this->update(upd);
}

void View3D::setFieldOfView(double value) {
  View3DValues upd;
  upd.setFieldOfView(value);
  this->update(upd);
}

void View3D::update(const View3DValues& values) {
  View3DValues copy(values);
  copy.valid &= this->changable();

  if (copy.valid == View3DProperty::None) return;

  this->values_.updateFrom(copy);

  if (vx::debug_option::Log_View3DUpdates()->get())
    qDebug() << "View3D::update" << (uint32_t)copy.valid << this->values_.lookAt
             << this->values_.orientation << this->values_.zoomLog
             << this->values_.viewSizeUnzoomed << this->values_.fieldOfView;

  Q_EMIT changed(copy);
}
