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

// QDBusConnection should be included as early as possible:
// https://bugreports.qt.io/browse/QTBUG-48351 /
// https://bugreports.qt.io/browse/QTBUG-48377
#include <QtDBus/QDBusConnection>

#include "ImagePaintWidget.hpp"

#include <VoxieBackend/Data/VolumeDataVoxel.hpp>
#include <VoxieBackend/Data/VolumeDataVoxelInst.hpp>

#include <PluginVisSlice/SliceVisualizer.hpp>

#include <Voxie/Vis/View3D.hpp>

#include <QtCore/QDebug>
#include <QtCore/QList>

#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>

#include <QtWidgets/QProgressBar>

using namespace vx::visualization;

ImagePaintWidget::ImagePaintWidget(SliceVisualizer* sv, QWidget* parent)
    : QLabel(parent), sv(sv), mouseLast{0, 0} {
  // TODO: High-DPI displays (should use larger checkerboard with higher
  // resolution)
  QImage image(":/icons-voxie/transparency_.png");
  b.setTextureImage(image);
  b.setStyle(Qt::TexturePattern);
  this->setFocusPolicy(Qt::WheelFocus);
  this->setMouseTracking(true);
  this->setFocus();
}

ImagePaintWidget::~ImagePaintWidget() {}

void ImagePaintWidget::wheelEvent(QWheelEvent* event) {
  auto pos = getMousePosition(this, event);
  sv->view3d()->wheelEvent(pos, event, size());

  sv->currentTool()->toolWheelEvent(event, pos);
}

void ImagePaintWidget::mousePressEvent(QMouseEvent* event) {
  // qDebug() << "click";

  auto pos = getMousePosition(this, event);

  sv->view3d()->mousePressEvent(mouseLast, pos, event, size());

  sv->currentTool()->toolMousePressEvent(event, pos);

  this->mouseLast = pos;
}

void ImagePaintWidget::mouseReleaseEvent(QMouseEvent* event) {
  auto pos = getMousePosition(this, event);

  sv->view3d()->mouseReleaseEvent(mouseLast, pos, event, size());

  sv->currentTool()->toolMouseReleaseEvent(event, pos);

  this->mouseLast = pos;
}

void ImagePaintWidget::keyPressEvent(QKeyEvent* event) {
  sv->view3d()->keyPressEvent(event, size());

  // quick tool switching with 1 ~ 9 & 0
  // qDebug() << "ImagePaintWidget::keyPressEvent" << e->key();
  // Note: Switching with keys causes problems when layes like
  // BrushSelectionTool which do not expect to be activated by the user are
  // activated
  /*
  int tool;
  if (event->key() >= Qt::Key_0 && event->key() <= Qt::Key_9 &&
      (event->modifiers() & Qt::KeypadModifier) ==
          0) {  // might want to exclude modifiers here
    tool = (event->key() - 0x30 - 1) % 10;
    if (tool >= 0 && tool < sv->tools().size()) {
      sv->switchToolTo(sv->tools().at(tool));
    }
  } else
  */
  sv->currentTool()->toolKeyPressEvent(event);
}

void ImagePaintWidget::keyReleaseEvent(QKeyEvent* event) {
  sv->currentTool()->toolKeyReleaseEvent(event);
}

void ImagePaintWidget::mouseMoveEvent(QMouseEvent* event) {
  auto pos = getMousePosition(this, event);

  sv->view3d()->mouseMoveEvent(mouseLast, pos, event, size());

  auto planePoint = this->sv->pixelPosToPlanePosCurrentImage(pos);
  auto threeDPoint = this->sv->planePosTo3DPosCurrentImage(planePoint);

  // TODO: rotation and translation of object
  vx::Vector<double, 3> posVoxel;
  const vx::Vector<double, 3>* posVoxelPtr = nullptr;
  double valNearest = NAN, valLinear = NAN;
  auto vol = dynamic_cast<vx::VolumeNode*>(this->sv->properties->volume());
  if (vol) {
    auto data =
        qSharedPointerDynamicCast<vx::VolumeDataVoxel>(vol->volumeData());
    if (data) {
      posVoxel = data->getObjectToVoxelTrafo().map(threeDPoint);
      posVoxelPtr = &posVoxel;

      valNearest = data->performInGenericContext([threeDPoint](auto& volInst) {
        return (double)volInst.getVoxelMetric(
            threeDPoint, vx::InterpolationMethod::NearestNeighbor);
      });
      valLinear = data->performInGenericContext([threeDPoint](auto& volInst) {
        return (double)volInst.getVoxelMetric(threeDPoint,
                                              vx::InterpolationMethod::Linear);
      });
    }
    // TODO: Non-voxel datasets
  }
  Q_EMIT this->sv->imageMouseMove(event, planePoint, threeDPoint, posVoxelPtr,
                                  valNearest, valLinear);

  sv->currentTool()->toolMouseMoveEvent(event, pos);

  this->mouseLast = pos;
}

void ImagePaintWidget::leaveEvent(QEvent* e) {
  sv->currentTool()->toolLeaveEvent(e);
}

void ImagePaintWidget::paintEvent(QPaintEvent* pe) {
  Q_UNUSED(pe);
  QPainter painter(this);
  painter.setBrush(b);
  painter.drawRect(0, 0, this->size().width(), this->size().height());
  QMap<int, QImage> map = sv->_drawStack;
  int i;
  for (i = 0; i < sv->layers().size(); ++i) {
    if (map.contains(i)) {
      QImage im = map.value(i);
      if (im.width() > 0 && im.height() > 0) {
        // qDebug() << "drawing stack at " << i << " of range -1 to " <<
        // sv->tools.size()-1;
        painter.drawImage(0, 0, im);
      } /* else {
           qDebug() << "stack at " << i << " is null";
       }*/
    }
  }
  // this->setPixmap(pixmap);
}
