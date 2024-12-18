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

#include <VoxieClient/Vector.hpp>

#include <QtCore/QObject>

#include <QtGui/QIcon>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>

#include <QtWidgets/QWidget>

/**
 * @brief The Visualizer2DTool class provides an interface for tools to be able
 * to receive information about the user input on the displayed image.
 */
class Visualizer2DTool : public QWidget {
  // TODO: This should have a QWidget as a member instead of inheriting from
  // QWidget
  Q_OBJECT
 public:
  Visualizer2DTool(QWidget* parent);
  ~Visualizer2DTool();

  virtual QIcon getIcon();
  virtual QString getName() = 0;

 public Q_SLOTS:
  /**
   * @brief activateTool signals that this tool was switched in the slice
   * visualizer.
   */
  virtual void activateTool() = 0;
  /**
   * @brief deactivateTool signals that this tool has been switched away from in
   * the slice visualizer.
   */
  virtual void deactivateTool() = 0;

  /**
   * Used to forward events to the tool. Check with the callee as it might
   * catching events already. (e.g. numbers for switching tools)
   */
  virtual void toolMousePressEvent(QMouseEvent*,
                                   const vx::Vector<double, 2>& pixelPos) = 0;
  /**
   * Used to forward events to the tool. Check with the callee as it might
   * catching events already. (e.g. numbers for switching tools)
   */
  virtual void toolMouseReleaseEvent(QMouseEvent*,
                                     const vx::Vector<double, 2>& pixelPos) = 0;
  /**
   * Used to forward events to the tool. Check with the callee as it might
   * catching events already. (e.g. numbers for switching tools)
   */
  virtual void toolMouseMoveEvent(QMouseEvent*,
                                  const vx::Vector<double, 2>& pixelPos) = 0;
  /**
   * Used to forward events to the tool. Check with the callee as it might
   * catching events already. (e.g. numbers for switching tools)
   */
  virtual void toolKeyPressEvent(QKeyEvent*) = 0;
  /**
   * Used to forward events to the tool. Check with the callee as it might
   * catching events already. (e.g. numbers for switching tools)
   */
  virtual void toolKeyReleaseEvent(QKeyEvent*) = 0;
  /**
   * Used to forward events to the tool. Check with the callee as it might
   * catching events already. (e.g. numbers for switching tools)
   */
  virtual void toolWheelEvent(QWheelEvent*,
                              const vx::Vector<double, 2>& pixelPos) = 0;
  /**
   * Used to forward events to the tool. Check with the callee as it might
   * catching events already. (e.g. numbers for switching tools)
   */
  virtual void toolLeaveEvent(QEvent*){};
};
