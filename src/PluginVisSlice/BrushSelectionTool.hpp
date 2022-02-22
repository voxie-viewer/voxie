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
#include <PluginVisSlice/Prototypes.forward.hpp>
#include <PluginVisSlice/ToolUtils.hpp>
#include <PluginVisSlice/Visualizer2DTool.hpp>
#include <QtWidgets/QPushButton>
#include <VoxieBackend/Data/VolumeDataVoxelInst.hpp>
#include <set>

#include <Voxie/Interfaces/SegmentationI.hpp>
#include <Voxie/Interfaces/StepManagerI.hpp>

class SliceVisualizer;

namespace vx {
/**
 * @brief The BrushSelectionTool class provides ways to put voxels with a Brush
 * into the selection state during a segmentation workflow.
 * @author Max Keller, Alex ...
 */
class BrushSelectionTool : public Visualizer2DTool {
  Q_OBJECT

 public:
  BrushSelectionTool(QWidget* parent, SliceVisualizer* sv);
  ~BrushSelectionTool() {}

  /**
   * @brief returns the radius of the brush
   * @param radius brush radius in pixels
   */
  void setBrushRadius(quint8 radius);

  /**
   * @brief Returns the brush radius in pixels
   */
  quint8 getBrushRadius();

  // TODO: Add a brush Icon!
  QIcon getIcon() override { return QIcon(":/icons/ruler-triangle.png"); }
  QString getName() override { return "Brush Selection"; }

  void triggerLayerRedraw(bool isMouseValid = true);
  void getStepManager();

 public Q_SLOTS:
  void activateTool() override;
  void deactivateTool() override;
  void toolMousePressEvent(QMouseEvent* e) override;
  void toolMouseReleaseEvent(QMouseEvent* ev) override;
  void toolMouseMoveEvent(QMouseEvent* e) override;
  void toolLeaveEvent(QEvent* e) override;
  void toolKeyPressEvent(QKeyEvent* e) override;
  void toolKeyReleaseEvent(QKeyEvent* e) override;
  void toolWheelEvent(QWheelEvent* e) override { Q_UNUSED(e) }

 private:
  SliceVisualizer* sv;
  vx::StepManagerI* stepManager = nullptr;
  quint8 brushRadius = 10;

  QPushButton* valueButton;
  bool mousePressed = false;
  QPoint startPos;
  void inline runBrushSelection(QPoint point);
};

class BrushSelectionLayer : public Layer {
  Q_OBJECT
  REFCOUNTEDOBJ_DECL(BrushSelectionLayer)

 public:
  BrushSelectionLayer(SliceVisualizer* sv);
  ~BrushSelectionLayer() {}

  QString getName() override {
    return "de.uni_stuttgart.Voxie.SliceVisualizer.Layer.BrushSelection";
  }

  void render(QImage& outputImage,
              const QSharedPointer<vx::ParameterCopy>& parameters) override;

  /**
   * @brief returns the radius of the brush
   * @param radius brush radius in pixels
   */
  void setBrushRadius(quint8 radius);

  /**
   * @brief set the radius of the brush
   * @return radius of brush in pixels
   */
  quint8 getBrushRadius();

  bool mousePosValid = false;

 private:
  /**
   * @brief Draw a circle around the current position of the mouse, to indicate
   * the area that could be selected
   * @param radius brush radius in pixels
   */
  static void drawCircle(QImage& outputImage, float visibility, QPointF point,
                         int radius);

 private:
  SliceVisualizer* sv;
  quint8 brushRadius = 10;

  float visibility;

  QPoint mousePos;
};

}  // namespace vx
