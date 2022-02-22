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
 * @brief The SliceAdjustmentTool class provides ways to modify the associated
 * slice using simple user input.
 */

class SliceVisualizer;

class SliceAdjustmentTool : public Visualizer2DTool {
  Q_OBJECT
 public:
  SliceAdjustmentTool(QWidget* parent, SliceVisualizer* sv);
  ~SliceAdjustmentTool() {}

  QIcon getIcon() override { return QIcon(":/icons/controller-d-pad.png"); }
  QString getName() override { return "Slice Adjustment"; }

 public Q_SLOTS:
  void activateTool() override;
  void deactivateTool() override;
  void toolMousePressEvent(QMouseEvent* ev) override;
  void toolMouseReleaseEvent(QMouseEvent* ev) override;
  void toolMouseMoveEvent(QMouseEvent* ev) override;
  void toolKeyPressEvent(QKeyEvent* ev) override;
  void toolKeyReleaseEvent(QKeyEvent* ev) override;
  void toolWheelEvent(QWheelEvent* ev) override;

  void rotateSlice(vx::Slice* slice, const QVector3D& rotationAxis,
                   float rotationAngle);

  void moveSlice(vx::Slice* slice, float steps);

 private:
  SliceVisualizer* sv;
  QPushButton* adjustButton;
  bool isActive = false;
  bool dragUpdates = true;

  bool shiftDown = false;
  bool ctrlDown = false;
  bool xDown = false;
  bool yDown = false;

  bool rotatingInProgress = false;
  QVector2D rotationHypotenuse;

  //
  float tiltAngle = 1;
  float fineAdjustmentFactor = 0.1f;
};

class SliceAdjustmentLayer : public Layer {
  Q_OBJECT
  REFCOUNTEDOBJ_DECL(SliceAdjustmentLayer)

 public:
  SliceAdjustmentLayer(SliceVisualizer* sv);
  ~SliceAdjustmentLayer() {}

  QString getName() override {
    return "de.uni_stuttgart.Voxie.SliceVisualizer.Layer.SliceAdjustment";
  }

  void render(QImage& outputImage,
              const QSharedPointer<vx::ParameterCopy>& parameters) override;

 private:
  SliceVisualizer* sv;
};
