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

#include <QtCore/QDebug>
#include <QtCore/QList>

#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>

#include <QtWidgets/QProgressBar>

ImagePaintWidget::ImagePaintWidget(SliceVisualizer* sv, QWidget* parent)
    : QLabel(parent), sv(sv) {
  QImage image(":/icons/transparency_.png");
  b.setTextureImage(image);
  b.setStyle(Qt::TexturePattern);
  this->setFocusPolicy(Qt::WheelFocus);
  this->setMouseTracking(true);
  this->setFocus();
}

ImagePaintWidget::~ImagePaintWidget() {}

void ImagePaintWidget::wheelEvent(QWheelEvent* e) {
  sv->currentTool()->toolWheelEvent(e);
}

void ImagePaintWidget::mousePressEvent(QMouseEvent* e) {
  // qDebug() << "click";
  sv->currentTool()->toolMousePressEvent(e);
}

void ImagePaintWidget::mouseReleaseEvent(QMouseEvent* e) {
  sv->currentTool()->toolMouseReleaseEvent(e);
}

void ImagePaintWidget::keyPressEvent(QKeyEvent* e) {
  // quick tool switching with 1 ~ 9 & 0
  // qDebug() << "ImagePaintWidget::keyPressEvent" << e->key();
  int tool;
  if (e->key() >= Qt::Key_0 &&
      e->key() <= Qt::Key_9) {  // might want to exclude modifiers here
    tool = (e->key() - 0x30 - 1) % 10;
    if (tool >= 0 && tool < sv->tools().size()) {
      sv->switchToolTo(sv->tools().at(tool));
    }
  } else {
    sv->currentTool()->toolKeyPressEvent(e);
  }
}

void ImagePaintWidget::keyReleaseEvent(QKeyEvent* e) {
  sv->currentTool()->toolKeyReleaseEvent(e);
}

void ImagePaintWidget::mouseMoveEvent(QMouseEvent* e) {
  // TODO: only calculate values when needed?
  bool doValueLookup =
      false;  // TODO: currently this triggers a race condition ("what():
              // Exception: de.uni_stuttgart.Voxie.InternalError: clInstance ==
              // nullptr")

  auto& imgUnf = this->sv->sliceImage();
  auto& imgFilt = this->sv->filteredSliceImage();
  const auto& pos = e->pos();
  auto planePoint = imgUnf.pixelToPlanePoint(e->pos(), true);
  auto slice = this->sv->slice();
  auto threeDPoint = slice ? slice->getCuttingPlane().get3DPoint(planePoint.x(),
                                                                 planePoint.y())
                           : QVector3D(NAN, NAN, NAN);
  double valUnf = NAN;
  if (doValueLookup && pos.x() >= 0 && ((size_t)pos.x()) < imgUnf.getWidth() &&
      pos.y() >= 0 && ((size_t)pos.y()) < imgUnf.getHeight())
    valUnf = imgUnf.getPixel(pos.x(), imgUnf.getHeight() - 1 - pos.y());
  double valFilt = NAN;
  if (doValueLookup && pos.x() >= 0 && ((size_t)pos.x()) < imgFilt.getWidth() &&
      pos.y() >= 0 && ((size_t)pos.y()) < imgFilt.getHeight())
    valFilt = imgFilt.getPixel(pos.x(), imgFilt.getHeight() - 1 - pos.y());
  // TODO: rotation and translation of object
  double valNearest = NAN, valLinear = NAN;
  auto vol = dynamic_cast<vx::VolumeNode*>(this->sv->properties->volume());
  if (vol) {
    auto data =
        qSharedPointerDynamicCast<vx::VolumeDataVoxel>(vol->volumeData());
    if (data) {
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
  Q_EMIT this->sv->imageMouseMove(e, planePoint, threeDPoint, valUnf, valFilt,
                                  valNearest, valLinear);

  sv->currentTool()->toolMouseMoveEvent(e);
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
