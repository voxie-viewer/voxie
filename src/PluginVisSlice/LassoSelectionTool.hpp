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

#include <PluginVisSlice/ToolUtils.hpp>
#include <QtWidgets/QPushButton>

#include <math.h>
#include <set>

#include <Voxie/Gui/ErrorMessage.hpp>
#include <Voxie/Interfaces/SegmentationI.hpp>
#include <Voxie/Interfaces/StepManagerI.hpp>

/**
 * @brief The LassoSelectionTool provides a way to select Voxels that are
 * inside a simple polygon that is span by the lasso. This selection can be used
 * in the segmentation.
 * @author Max Keller, Alex ...
 */

class SliceVisualizer;

namespace vx {

class LassoSelectionLayer;
class GeometricPrimitiveData;

class LassoSelectionTool : public Visualizer2DTool {
  Q_OBJECT

 public:
  LassoSelectionTool(QWidget* parent, SliceVisualizer* sv);
  ~LassoSelectionTool() {}

  QIcon getIcon() override { return QIcon(":/icons/ruler-triangle.png"); }
  QString getName() override { return "LassoSelection"; }

 public Q_SLOTS:
  void activateTool() override;
  void deactivateTool() override;
  void toolMousePressEvent(QMouseEvent* e) override;
  void toolMouseReleaseEvent(QMouseEvent* ev) override;
  void toolMouseMoveEvent(QMouseEvent* e) override;
  void toolKeyPressEvent(QKeyEvent* e) override;
  void toolKeyReleaseEvent(QKeyEvent* e) override;
  void toolWheelEvent(QWheelEvent* e) override { Q_UNUSED(e) }

 private:
  /**
   * @brief savePoint converts this point to a 3D point and saves it
   * @param point
   */
  void savePoint(QPointF point);
  bool getStepManager();

  /**
   * @brief Triggers the lassoSelectionLayer redraw.
   */
  void triggerLayerRedraw();

  /**
   * @brief Resets the LassoSelection-Tool and -Layer,
   * e. g. clears node & line list in the tool & Layer
   */
  void reset();

  /**
   * @brief checks if newLine intersect with a line in lines
   * @param newLine line
   */
  bool doesNewLineIntersect(QLineF newLine);

  /**
   * @brief checks if a new point closes the Polygon
   * @param point new point (not yet in nodes)
   */
  bool doesPointClosePolygon(QPointF point);

  /**
   * @brief returns closest node to passed point
   * @param point new point (not yet in nodes)
   */
  QPointF getClosestNode(QPointF point);

 private:
  // TODO: What happens if stepManager is destroyed?
  vx::StepManagerI* stepManager = nullptr;
  quint8 minDistance = 10;  // minimal distance in pixels between points (of
                            // Polygon) before they are considered equal
  QList<QPointF> nodes;     // points that span the Polygon
  QList<QLineF> lines;      // Lines between the nodes
  SliceVisualizer* sv;
  LassoSelectionLayer* lassoLayer;

  QPushButton* valueButton;
  bool mousePressed;
  QPoint startPos;
};

class LassoSelectionLayer : public Layer {
  Q_OBJECT
  REFCOUNTEDOBJ_DECL(LassoSelectionLayer)

 public:
  LassoSelectionLayer(SliceVisualizer* sv);
  ~LassoSelectionLayer() {}

  // QIcon getIcon() override { return QIcon(":/icons/ruler-triangle.png"); }
  QString getName() override {
    return "de.uni_stuttgart.Voxie.SliceVisualizer.Layer.LassoSelection";
  }

  void render(QImage& outputImage,
              const QSharedPointer<vx::ParameterCopy>& parameters) override;

 public Q_SLOTS:
  /**
   * @brief newVisibility ; Use this slot when the visibility of points to
   * either side of the slice change
   * @param newVis
   */
  void newVisibility(float newVis);
  /**
   * @brief adds a Node that should be drawn
   * @param node Point that should be drawn.
   */
  void addNode(QPointF node);

  /**
   * @brief set all nodes That should be drawn (removes old nodes)
   * @param nodes List of nodes
   */
  void setNodes(QList<QPointF> nodes);

 private:
  void drawPoint(QImage& outputImage, float visibility, QPointF point);
  void changePointName(QPoint point);
  /**
   * @brief redraw redraws all the points that have been saved
   */
  void redraw(QImage& outputImage, float visibility);
  /**
   * @brief drawLine draws the line between the two points if they are in the
   * region of visibility around the slice
   * @param p1
   * @param p2
   */
  void drawLine(QImage& outputImage, float visibility, QPointF p1, QPointF p2);

 private:
  SliceVisualizer* sv;
  float visibility;
  QList<QPointF> nodes;  // all points that span the polygon

  QPoint mousePos;
  bool mousePosValid = false;
};

}  // namespace vx
