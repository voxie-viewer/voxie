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

#include "VisualizerView.hpp"

#include <VoxieBackend/Data/ImageDataPixel.hpp>

#include <Voxie/Node/ParameterCopy.hpp>

#include <Voxie/Vis/VisualizerNode.hpp>

#include <QtCore/QDebug>

#include <QtGui/QPainter>

#include <VoxieBackend/IO/SharpThread.hpp>

static bool verbose = false;
// static bool verbose = true;

using namespace vx;
using namespace vx::visualization;

// TODO: use background thread

VisualizerView::VisualizerView(QWidget* parent, VisualizerNode* visualizer)
    : QWidget(parent), visualizer(visualizer) {
  QImage image(":/icons/transparency_.png");
  b.setTextureImage(image);
  b.setStyle(Qt::TexturePattern);
  this->setFocusPolicy(Qt::WheelFocus);
  this->setMouseTracking(true);
  this->setFocus();

  qRegisterMetaType<QSharedPointer<vx::ImageDataPixel>>();

  // Trigger initial drawing of widget
  this->triggerRedraw();
}
VisualizerView::~VisualizerView() {}

void VisualizerView::triggerRedraw() {
  vx::checkOnMainThread("VisualizerView::triggerRedraw");
  if (verbose)
    qDebug() << "VisualizerView::triggerRedraw" << redrawPending
             << redrawRequested;

  redrawRequested = true;

  maybeStartRedraw();
}

void VisualizerView::maybeStartRedraw() {
  vx::checkOnMainThread("VisualizerView::maybeStartRedraw");
  if (verbose)
    qDebug() << "VisualizerView::maybeStartRedraw" << redrawPending
             << redrawRequested;

  if (!redrawRequested) return;
  if (redrawPending) return;

  enqueueOnThread(this, [this]() {
    if (verbose)
      qDebug() << "VisualizerView::maybeStartRedraw callback" << redrawPending
               << redrawRequested << redrawRunning;

    redrawPending = false;

    // After redrawRunning has been set to false (which happens on the main
    // thread), maybeStartRedraw() will be called again and the code will
    // get here again
    if (redrawRunning) return;

    redrawRequested = false;

    VisualizerNode* visualizer = this->visualizer;
    if (!visualizer) {
      qWarning() << "Attempting to draw a destroyed visualizer";
      renderFinished(QSharedPointer<vx::ImageDataPixel>());
      return;
    }

    Q_EMIT this->beforeRender();

    QSize size = this->size();

    auto renderFunction = visualizer->getRenderFunction();

    QSharedPointer<vx::ParameterCopy> parameters;
    try {
      parameters = vx::ParameterCopy::getParameters(visualizer);
    } catch (vx::Exception& e) {
      qWarning() << "Error while getting parameters for rendering slice "
                    "image layer"
                 << ": " << e.what();
      renderFinished(QSharedPointer<vx::ImageDataPixel>());
      return;
    }

    if (size.width() < 0 || size.height() < 0) {
      qWarning() << "Got negative size while rendering visualizer";
      renderFinished(QSharedPointer<vx::ImageDataPixel>());
      return;
    }

    auto options = createQSharedPointer<VisualizerRenderOptions>(true);

    auto result = makeSharedQObject<VisualizerViewRenderResult>();
    QObject::connect(result.data(), &VisualizerViewRenderResult::renderFinished,
                     this, &VisualizerView::renderFinished);

    // Move the calculation to a separate thread. This is done only after
    // waiting for the main thread to return to the main loop to return in
    // order to combine multiple draw requests.
    auto thread = new SharpThread(
        [result, parameters, renderFunction, size, options]() -> void {
          if (verbose) qDebug() << "VisualizerView::maybeStartRedraw thread";
          try {
            if (verbose) qDebug() << "VisualizerView::doRedraw" << size;

            auto resultImage = ImageDataPixel::createInst(
                size.width(), size.height(), 4, DataType::Float32, false);

            renderFunction(resultImage, vx::VectorSizeT2(0, 0),
                           vx::VectorSizeT2(size.width(), size.height()),
                           parameters, options);

            Q_EMIT result->renderFinished(resultImage);
          } catch (vx::Exception& e) {
            qWarning() << "Error while rendering visualizer: " << e.what();
            Q_EMIT result->renderFinished(QSharedPointer<vx::ImageDataPixel>());
            return;
          }
          if (verbose)
            qDebug() << "VisualizerView::maybeStartRedraw thread finished";
        });
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(thread, &QThread::finished, this, [this]() {
      this->redrawRunning = false;
      this->maybeStartRedraw();
    });
    redrawRunning = true;
    thread->start();
  });
  redrawPending = true;
}

void VisualizerView::renderFinished(
    const QSharedPointer<vx::ImageDataPixel>& result) {
  this->lastRenderResult = result;
  this->update();
}

// void VisualizerView::triggerRedraw() { this->update(); }

void VisualizerView::paintEvent(QPaintEvent*) {
  /*
  VisualizerNode* visualizer = this->visualizer;
  if (!visualizer) {
    qWarning() << "Attempting to paint a destroyed visualizer";
    return;
  }

  QSize size = this->size();

  QPainter painter(this);
  painter.setBrush(b);
  painter.drawRect(0, 0, size.width(), size.height());

  auto render = visualizer->getRenderFunction();
  auto parameters = ParameterCopy::getParameters(visualizer);
  auto img = ImageDataPixel::createInst(size.width(), size.height(), 4,
                                        DataType::Float32, false);
  // image.fill(qRgba(0, 0, 0, 0));  // Fill with transparent // TODO
  // TODO: do this in a background thread
  (*render)(img, vx::VectorSizeT2(0, 0),
            vx::VectorSizeT2(size.width(), size.height()), parameters);
  auto qimg = img->convertToQImage();

  painter.drawImage(0, 0, qimg);
  */

  QSize size = this->size();
  QPainter painter(this);
  painter.setBrush(b);
  painter.drawRect(0, 0, size.width(), size.height());
  if (lastRenderResult) {
    auto qimg = lastRenderResult->convertToQImage();
    painter.drawImage(0, 0, qimg);
  }
}

void VisualizerView::resizeEvent(QResizeEvent* ev) {
  Q_UNUSED(ev);
  triggerRedraw();
}

void VisualizerView::mousePressEvent(QMouseEvent* event) {
  Q_EMIT this->forwardMousePressEvent(event);
}
void VisualizerView::mouseReleaseEvent(QMouseEvent* event) {
  Q_EMIT this->forwardMouseReleaseEvent(event);
}
void VisualizerView::mouseMoveEvent(QMouseEvent* event) {
  Q_EMIT this->forwardMouseMoveEvent(event);
}
void VisualizerView::keyPressEvent(QKeyEvent* event) {
  Q_EMIT this->forwardKeyPressEvent(event);
}
void VisualizerView::keyReleaseEvent(QKeyEvent* event) {
  Q_EMIT this->forwardKeyReleaseEvent(event);
}
void VisualizerView::wheelEvent(QWheelEvent* event) {
  Q_EMIT this->forwardWheelEvent(event);
}

SimpleVisualizer::SimpleVisualizer(
    const QSharedPointer<vx::NodePrototype>& prototype)
    : VisualizerNode(prototype), view_(new VisualizerView(nullptr, this)) {
  QObject::connect(this, &QObject::destroyed, view_, &QObject::deleteLater);
}
SimpleVisualizer::~SimpleVisualizer() {}

QWidget* SimpleVisualizer::mainView() { return view(); }

void SimpleVisualizer::triggerRedraw() { view()->triggerRedraw(); }
