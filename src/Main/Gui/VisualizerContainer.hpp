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

#include <QtWidgets/QAction>
#include <QtWidgets/QMdiArea>
#include <QtWidgets/QMdiSubWindow>
#include <QtWidgets/QWidget>

namespace vx {
class VisualizerNode;

enum class VisualizerWindowMode { Normal, Minimize, Maximize, FullScreen };
}  // namespace vx

class VisualizerContainer : public QWidget {
  Q_OBJECT
  QIcon icon;

 public:
  vx::VisualizerNode* const visualizer;
  QMdiArea* const container;
  QMdiSubWindow* window;
  bool isAttached = true;

 public:
  explicit VisualizerContainer(QMdiArea* container,
                               vx::VisualizerNode* visualizer);

  ~VisualizerContainer();

  virtual void changeEvent(QEvent* event) override;

  void activate();

  void closeWindow();

  vx::VisualizerNode* getVisualizer();

  QPoint getVisualizerPosition();

  void setVisualizerPosition(QPoint pos);

  QSize getVisualizerSize();

  void setVisualizerSize(QSize size);

  void switchPopState();

  void setWindowMode(vx::VisualizerWindowMode mode);

 protected:
  virtual void closeEvent(QCloseEvent* ev) override;

 private:
  void moveToNewMdiChild();

  void subWindowChanged(Qt::WindowStates oldState, Qt::WindowStates newState);

  void destroyme(QObject*);

 Q_SIGNALS:
  void sidePanelVisiblityChanged(bool isVisible);

 public Q_SLOTS:
};
