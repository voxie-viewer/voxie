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

#pragma once

#include <Voxie/Voxie.hpp>

#include <Voxie/SpNav/SpaceNavClient.hpp>

#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>

namespace vx {
class VisualizerNode;

namespace spnav {

class SpaceNavVisualizer;

class VOXIECORESHARED_EXPORT SpaceNavVisualizerMaster : public QObject {
  Q_OBJECT

  friend class SpaceNavVisualizer;

  QSharedPointer<SpaceNavClient> client;

  QMap<VisualizerNode*, SpaceNavVisualizer*> visualizers;
  SpaceNavVisualizer* current = nullptr;

  SpaceNavVisualizerMaster();

 public:
  ~SpaceNavVisualizerMaster();

  static QSharedPointer<SpaceNavVisualizerMaster> getMaster();
};

class VOXIECORESHARED_EXPORT SpaceNavVisualizer : public QObject {
  Q_OBJECT

  QSharedPointer<SpaceNavVisualizerMaster> master;

 public:
  SpaceNavVisualizer(VisualizerNode* parent);
  ~SpaceNavVisualizer();

 Q_SIGNALS:
  // This events will only be emitted if the parent visualizer is active
  void motionEvent(SpaceNavMotionEvent* event);
  void buttonPressEvent(SpaceNavButtonPressEvent* event);
  void buttonReleaseEvent(SpaceNavButtonReleaseEvent* event);

  void looseFocus();
};

}  // namespace spnav
}  // namespace vx
