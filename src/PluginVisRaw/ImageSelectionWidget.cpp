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
// https://bugreports.qt.io/browse/QTBUG-48351
#include <QtDBus/QDBusConnection>

#include "ImageSelectionWidget.hpp"

#include <PluginVisRaw/Prototypes.hpp>
#include <PluginVisRaw/RawData2DVisualizer.hpp>

#include <QtCore/QThreadPool>

#include <QtWidgets/QGridLayout>

using namespace vx;
ImageSelectionWidget::ImageSelectionWidget(QWidget* parent, RawVisualizer* rv)
    : QWidget(parent), rv(rv) {
  timer = new QTimer(this);
  timer->setInterval(timerSec);

  connect(this->timer, &QTimer::timeout, this, [this]() {
    if (this->rv->properties->waitForImages()) {
      // TODO: This probably could be done in a nicer way
      // TODO: Make sure that each image is show at least for a certain period
      // of time?
      if (!this->rv->isUpToDate()) return;
    }
    this->forward();
  });

  QGridLayout* layout = new QGridLayout(this);
  layout->setSpacing(2);
  forwardsButton = new QPushButton(QIcon(":/icons-voxie/skip-next.png"), "");
  backwardsButton =
      new QPushButton(QIcon(":/icons-voxie/skip-previous.png"), "");
  playButton = new QPushButton(QIcon(":/icons-voxie/play.png"), "");

  connect(forwardsButton, &QPushButton::clicked, [=]() { forward(); });
  connect(backwardsButton, &QPushButton::clicked, [=]() { backward(); });
  connect(playButton, &QPushButton::clicked, [=]() { play(); });

  layout->addWidget(backwardsButton, 0, 0);
  layout->addWidget(playButton, 0, 1);
  layout->addWidget(forwardsButton, 0, 2);
  backwardsButton->show();
  playButton->show();
  forwardsButton->show();

  connect(
      rv->properties,
      &vx::visualizer_prop::TomographyRawDataProperties::imagesPerSecondChanged,
      this, [this](float imagesPerSecond) {
        this->changeTimer(1000.0 / imagesPerSecond);
      });
  this->changeTimer(1000.0 / rv->properties->imagesPerSecond());

  this->setLayout(layout);
}

void ImageSelectionWidget::forward() {
  try {
    // qDebug() << "enter forward" << rv->properties->currentImage();
    auto imageCount = rv->getCurrentImageListCount();
    qint64 id = rv->properties->currentImage();

    auto nextId = id + 1;
    if (nextId < (qint64)imageCount) {
      rv->properties->setCurrentImage(nextId);
    } else {
      if (imageCount != 0) {
        rv->properties->setCurrentImage(0);
      }
    }
    // qDebug() << "leave forward";
  } catch (vx::Exception& e) {
    qWarning() << "Error in ImageSelectionWidget::forward():" << e.message();
  }
}

void ImageSelectionWidget::changeTimer(int newTimer) {
  if (timer->isActive()) {
    timer->stop();
    timer->setInterval(newTimer);
    timer->start();
  } else {
    timer->setInterval(newTimer);
  }
}

void ImageSelectionWidget::backward() {
  try {
    auto imageCount = rv->getCurrentImageListCount();
    qint64 id = rv->properties->currentImage();

    auto nextId = id - 1;
    if (nextId >= 0) {
      rv->properties->setCurrentImage(nextId);
    } else {
      if (imageCount != 0) {
        rv->properties->setCurrentImage(imageCount - 1);
      }
    }
  } catch (vx::Exception& e) {
    qWarning() << "Error in ImageSelectionWidget::backward():" << e.message();
  }
}

void ImageSelectionWidget::play() {
  // TODO switch icons

  if (running) {
    this->playButton->setIcon(QIcon(":/icons-voxie/play.png"));
    running = false;
    timer->stop();
  } else {
    this->playButton->setIcon(QIcon(":/icons-voxie/pause.png"));
    running = true;
    timer->start();
  }
}
