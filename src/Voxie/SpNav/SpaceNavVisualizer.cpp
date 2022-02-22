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

#include "SpaceNavVisualizer.hpp"

#include <Voxie/IVoxie.hpp>
#include <Voxie/Vis/VisualizerNode.hpp>

#include <QtCore/QAbstractNativeEventFilter>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QMutex>

#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

using namespace vx;
using namespace vx::spnav;

SpaceNavVisualizerMaster::SpaceNavVisualizerMaster() : QObject() {
  client = SpaceNavClient::getClient();

  connect(vx::voxieRoot().activeVisualizerProvider(),
          &vx::ActiveVisualizerProvider::activeVisualizerChanged, this,
          [this](VisualizerNode* now) {
            if (visualizers.contains(now)) {
              auto old = current;
              current = visualizers[now];
              if (old && old != current) old->looseFocus();
            }
          });

  connect(client.data(), &SpaceNavClient::motionEvent, this,
          [this](SpaceNavMotionEvent* ev) {
            if (current) Q_EMIT current->motionEvent(ev);
          });
  connect(client.data(), &SpaceNavClient::buttonPressEvent, this,
          [this](SpaceNavButtonPressEvent* ev) {
            if (current) Q_EMIT current->buttonPressEvent(ev);
          });
  connect(client.data(), &SpaceNavClient::buttonReleaseEvent, this,
          [this](SpaceNavButtonReleaseEvent* ev) {
            if (current) Q_EMIT current->buttonReleaseEvent(ev);
          });
}
SpaceNavVisualizerMaster::~SpaceNavVisualizerMaster() {
  // qDebug() << "~SpaceNavVisualizerMaster()";
}

QSharedPointer<SpaceNavVisualizerMaster> SpaceNavVisualizerMaster::getMaster() {
  static QWeakPointer<SpaceNavVisualizerMaster> weak;
  static QMutex weakMutex;

  QMutexLocker locker(&weakMutex);

  QSharedPointer<SpaceNavVisualizerMaster> strong = weak;
  if (strong) return strong;

  strong.reset(new SpaceNavVisualizerMaster(),
               [](SpaceNavVisualizerMaster* obj) { obj->deleteLater(); });
  weak = strong;
  return strong;
}

SpaceNavVisualizer::SpaceNavVisualizer(VisualizerNode* parent)
    : QObject(parent) {
  if (!parent) {
    qCritical() << "parent is null";
    return;
  }

  master = SpaceNavVisualizerMaster::getMaster();

  if (master->visualizers.contains(parent)) {
    qCritical() << "Visualizer" << parent << "already has a SpaceNavVisualizer";
    return;
  }

  connect(this, &QObject::destroyed, master.data(), [this, parent] {
    if (master->current == this) {
      master->current = nullptr;
    }
    master->visualizers.remove(parent);
  });

  if (vx::voxieRoot().activeVisualizerProvider()->activeVisualizer() == parent)
    master->current = this;
  master->visualizers[parent] = this;
}
SpaceNavVisualizer::~SpaceNavVisualizer() {}
