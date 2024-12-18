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

#include "Layer.hpp"

#include <VoxieBackend/IO/SharpThread.hpp>

static bool verbose = false;
// static bool verbose = true;

Layer::Layer() : RefCountedObject("Layer") {}
Layer::~Layer() {}

QString Layer::displayName() { return getName(); }

// TODO: Layer::triggerRedraw() / Layer::maybeStartRedraw() / Layer::doRedraw()
// are mostly the same as VisualizerView::...

void Layer::triggerRedraw() {
  vx::checkOnMainThread("Layer::triggerRedraw");
  if (verbose)
    qDebug() << "Layer::triggerRedraw" << getName() << redrawPending
             << redrawRequested;

  redrawRequested = true;

  maybeStartRedraw();
}

void Layer::maybeStartRedraw() {
  vx::checkOnMainThread("Layer::maybeStartRedraw");
  if (verbose)
    qDebug() << "Layer::maybeStartRedraw" << getName() << redrawPending
             << redrawRequested;

  if (!redrawRequested) {
    maybeUpdateIsUpToDate();
    return;
  }
  if (redrawPending) return;

  enqueueOnThread(this, [this]() {
    if (verbose)
      qDebug() << "Layer::maybeStartRedraw callback" << getName()
               << redrawPending << redrawRequested << redrawRunning;

    redrawPending = false;

    // After redrawRunning has been set to false (which happens on the main
    // thread), maybeStartRedraw() will be called again and the code will
    // get here again
    if (redrawRunning) return;

    redrawRequested = false;

    QSharedPointer<vx::ParameterCopy> parameters;
    QSize size;
    try {
      Q_EMIT getRenderingParameters(parameters, size);
      if (!parameters)
        throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                            "Failed to get parameters");
    } catch (vx::Exception& e) {
      qWarning() << "Error while getting parameters for rendering slice "
                    "image layer"
                 << getName() << ": " << e.what();
      clearResultImage();
      return;
    }

    // Move the calculation to a separate thread. This is done only after
    // waiting for the main thread to return to the main loop to return in
    // order to combine multiple draw requests.
    auto self = this->thisShared();
    auto thread = new SharpThread([self, parameters, size]() -> void {
      if (verbose)
        qDebug() << "Layer::maybeStartRedraw thread" << self->getName();
      self->doRedraw(parameters, size);
      if (verbose)
        qDebug() << "Layer::maybeStartRedraw thread finished"
                 << self->getName();
    });
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(thread, &QThread::finished, this, [this]() {
      this->redrawRunning = false;
      this->maybeStartRedraw();
    });
    redrawRunning = true;
    maybeUpdateIsUpToDate();
    thread->start();
  });
  redrawPending = true;
  maybeUpdateIsUpToDate();
}

void Layer::maybeUpdateIsUpToDate() {
  vx::checkOnMainThread("Layer::maybeUpdateIsUpToDate");
  auto newIsUpToDate = !redrawPending && !redrawRunning;
  if (verbose)
    qDebug() << this << "isUpToDate changes from" << isUpToDate_ << "to"
             << newIsUpToDate;
  if (isUpToDate_ != newIsUpToDate) {
    isUpToDate_ = newIsUpToDate;
    Q_EMIT isUpToDateChanged();
  }
}

bool Layer::isUpToDate() {
  vx::checkOnMainThread("Layer::isUpToDate");
  return isUpToDate_;
}

// Will be called on background thread
void Layer::doRedraw(const QSharedPointer<vx::ParameterCopy>& parameters,
                     const QSize& size) {
  try {
    if (verbose) qDebug() << "Layer::doRedraw" << getName();

    /*
    if (this->cachedImage.width() != size.width() ||
        this->cachedImage.height() != size.height())
      cachedImage = QImage(size, QImage::Format_ARGB32);
    */
    QImage cachedImage(size, QImage::Format_ARGB32);

    cachedImage.fill(qRgba(0, 0, 0, 0));  // Fill with transparent
    render(cachedImage, parameters, true);

    setResultImage(cachedImage);
  } catch (vx::Exception& e) {
    qWarning() << "Error while rendering slice image layer" << getName() << ": "
               << e.what();
    clearResultImage();
  }
}

// Will be called on background thread
void Layer::setResultImage(const QImage& image) {
  Q_EMIT resultImageChanged(image);
}
void Layer::clearResultImage() { Q_EMIT resultImageChanged(QImage()); }

void Layer::onResize(const QSize& size) {
  Q_UNUSED(size);
  this->triggerRedraw();
}
