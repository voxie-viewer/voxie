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

#include <PluginVisSlice/Layer.hpp>
#include <PluginVisSlice/Visualizer2DTool.hpp>

#include <QtWidgets/QPushButton>

/**
 * @brief The DefaultTool class provides ways to modify the associated
 * slice using simple user input.
 */

class SliceVisualizer;

class DefaultTool : public Visualizer2DTool {
  Q_OBJECT
 public:
  DefaultTool(QWidget* parent, SliceVisualizer* sv);
  ~DefaultTool() {}

  QIcon getIcon() override { return QIcon(":/icons/controller-d-pad.png"); }
  QString getName() override { return "Default"; }

 public Q_SLOTS:
  void activateTool() override;
  void deactivateTool() override;
  void toolMousePressEvent(QMouseEvent* ev,
                           const vx::Vector<double, 2>& pixelPos) override;
  void toolMouseReleaseEvent(QMouseEvent* ev,
                             const vx::Vector<double, 2>& pixelPos) override;
  void toolMouseMoveEvent(QMouseEvent* ev,
                          const vx::Vector<double, 2>& pixelPos) override;
  void toolKeyPressEvent(QKeyEvent* ev) override;
  void toolKeyReleaseEvent(QKeyEvent* ev) override;
  void toolWheelEvent(QWheelEvent* ev,
                      const vx::Vector<double, 2>& pixelPos) override;

 private:
  /**
   * @brief savePoint converts this point to a 3D point and saves it
   * @param point
   */
  void savePoint(const vx::Vector<double, 2>& pixelPos);

 private:
  SliceVisualizer* sv;
  QPushButton* button;
};
