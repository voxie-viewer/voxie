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

#include <PluginVisSlice/Visualizer2DTool.hpp>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>

class SliceVisualizer;

/**
 * @brief The ToolZoom class provides methods to zoom and drag the currently
 * displayed image.
 * @author Hans Martin Berner, David Haegele
 */

class ToolZoom : public Visualizer2DTool {
  Q_OBJECT
 public:
  ToolZoom(QWidget* parent, SliceVisualizer* sv);
  ~ToolZoom();

  QIcon getIcon() override;
  QString getName() override;

 private:
  QPushButton* zoomButton;
  SliceVisualizer* sv;
  QPoint dragStart;
  bool dragStartValid = false;
  // QDoubleSpinBox* zoomBox;
  QCheckBox* dragRedraw;
  // QDoubleSpinBox* xBox;
  // QDoubleSpinBox* yBox;
 public Q_SLOTS:
  void activateTool() override;
  void deactivateTool() override;
  void toolMousePressEvent(QMouseEvent* e) override;
  void toolMouseReleaseEvent(QMouseEvent* e) override;
  void toolMouseMoveEvent(QMouseEvent* e) override;
  void toolKeyPressEvent(QKeyEvent* e) override { Q_UNUSED(e) }
  void toolKeyReleaseEvent(QKeyEvent* e) override { Q_UNUSED(e) }
  void toolWheelEvent(QWheelEvent* e) override;
};
