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

#include <Voxie/Vis/VisualizerNode.hpp>

#include <QtCore/QPointer>

#include <QtWidgets/QWidget>

namespace vx {
namespace visualization {

class VOXIECORESHARED_EXPORT VisualizerViewRenderResult : public QObject {
  Q_OBJECT

 Q_SIGNALS:
  void renderFinished(const QSharedPointer<vx::ImageDataPixel>& result);
};

/**
 * @brief Widget which will show the content of a visualizer using its render()
 * function
 */
class VOXIECORESHARED_EXPORT VisualizerView : public QWidget {
  Q_OBJECT

  QPointer<VisualizerNode> visualizer;
  QBrush b;

 public:
  VisualizerView(QWidget* parent, VisualizerNode* visualizer);
  ~VisualizerView() override;

  void triggerRedraw();

 protected:
  void paintEvent(QPaintEvent*) override;

  void resizeEvent(QResizeEvent* ev) override;

  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void keyReleaseEvent(QKeyEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;

 private:
  void maybeStartRedraw();
  void renderFinished(const QSharedPointer<vx::ImageDataPixel>& result);

  // redrawRequested is set to true when triggerRedraw() has been called and a
  // new redraw operation has to be started
  bool redrawRequested = false;
  // redrawPending is set to true when the redraw is waiting for a return to the
  // main loop (in order to combine multiple requests)
  bool redrawPending = false;
  // redrawRunning is set to true while a redraw operation is actually running
  bool redrawRunning = false;

  QSharedPointer<vx::ImageDataPixel> lastRenderResult;

 Q_SIGNALS:
  /**
   * This signal is emitted on the main thread before the rendering is started.
   *
   * Is is emitted before the parameter are read, i.e. changes to the parameters
   * in the handlers for this signal will be considered.
   */
  void beforeRender();

  void forwardMousePressEvent(QMouseEvent* event);
  void forwardMouseReleaseEvent(QMouseEvent* event);
  void forwardMouseMoveEvent(QMouseEvent* event);
  void forwardKeyPressEvent(QKeyEvent* event);
  void forwardKeyReleaseEvent(QKeyEvent* event);
  void forwardWheelEvent(QWheelEvent* event);
};

/**
 * @brief A visualizer which consists only of a VisualizerView.
 */
class VOXIECORESHARED_EXPORT SimpleVisualizer : public VisualizerNode {
  Q_OBJECT

  VisualizerView* view_;

 public:
  explicit SimpleVisualizer(const QSharedPointer<vx::NodePrototype>& prototype);
  ~SimpleVisualizer() override;

  VisualizerView* view() { return view_; }

  QWidget* mainView() override;

  void triggerRedraw();
};
}  // namespace visualization
}  // namespace vx
