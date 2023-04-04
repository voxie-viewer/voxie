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

#include <QtGui/QResizeEvent>

#include <QtWidgets/QLabel>

class SliceVisualizer;

/**
 * Provides a widget that can draw a stack of qimages on top of each other and
 * forward user input associated with the widget to tools.
 * @author Hans Martin Berner
 */

class ImagePaintWidget : public QLabel {
  Q_OBJECT
 public:
  /**
   * @brief ImagePaintWidget is a widget that can paint a stack of qimages and
   * forward its input events to slice visualizer tools.
   * @param sv the corresponding slice visualizer
   * @param parent
   */
  ImagePaintWidget(SliceVisualizer* sv, QWidget* parent = 0);
  ~ImagePaintWidget();
  /**
   * event forwarding methods
   */
  void mousePressEvent(QMouseEvent*) override;
  void mouseReleaseEvent(QMouseEvent*) override;
  void keyPressEvent(QKeyEvent*) override;
  void keyReleaseEvent(QKeyEvent*) override;
  void wheelEvent(QWheelEvent*) override;
  void mouseMoveEvent(QMouseEvent*) override;
  void resizeEvent(QResizeEvent* ev) override { Q_EMIT this->resized(ev); }
  void leaveEvent(QEvent*) override;
 Q_SIGNALS:
  void resized(QResizeEvent* ev);

 protected:
  void paintEvent(QPaintEvent*) override;

 private:
  QBrush b;
  SliceVisualizer* sv;
  QPoint mouseLast;
};
