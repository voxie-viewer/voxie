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

#include <Voxie/OldFilter/Filter2D.hpp>

#include <Voxie/OldFilterMask/Selection2DMask.hpp>

#include <QtGui/QPainterPath>
#include <QtGui/QTransform>

#include <QtWidgets/QPushButton>

class SliceVisualizer;

// Note: This is not really separated into tool and layer currently, the tool
// keeps a reference to the layer

/**
 * @brief The ToolSelection class provides ways to define a selection mask to
 * selectively apply filters to a slice.
 */

class ToolSelection;

class ToolSelectionLayer : public Layer {
  Q_OBJECT
  VX_REFCOUNTEDOBJECT

  QMutex previewMutex_;
  bool previewSet_ = false;
  QPainterPath preview_;

 public:
  ToolSelectionLayer(SliceVisualizer* sv);
  ~ToolSelectionLayer() {}

  QString getName() override {
    return "de.uni_stuttgart.Voxie.SliceVisualizer.Layer.ToolSelection";
  }

  void render(QImage& outputImage,
              const QSharedPointer<vx::ParameterCopy>& parameters,
              bool isMainImage) override;

  void clearPreview();
  void setPreview(const QPainterPath& preview);

  QPainterPath planeToImage(const QRectF& planeArea, const QSize& canvasSize,
                            const QPainterPath& path);
};

class ToolSelection : public Visualizer2DTool {
  Q_OBJECT
 public:
  ToolSelection(QWidget* parent, SliceVisualizer* sv);

  QIcon getIcon() override;
  QString getName() override;

  const QSharedPointer<ToolSelectionLayer>& layer() { return layer_; }

 public Q_SLOTS:
  void activateTool() override;
  void deactivateTool() override;
  void toolMousePressEvent(QMouseEvent* e,
                           const vx::Vector<double, 2>& pixelPos) override;
  void toolMouseReleaseEvent(QMouseEvent* e,
                             const vx::Vector<double, 2>& pixelPos) override;
  void toolMouseMoveEvent(QMouseEvent* e,
                          const vx::Vector<double, 2>& pixelPos) override;
  void toolKeyPressEvent(QKeyEvent* e) override;
  void toolKeyReleaseEvent(QKeyEvent* e) override { Q_UNUSED(e); }
  void toolWheelEvent(QWheelEvent* e,
                      const vx::Vector<double, 2>& pixelPos) override {
    Q_UNUSED(e);
    Q_UNUSED(pixelPos);
  }
  void setMask(vx::filter::Filter2D* filter);

 private:
  SliceVisualizer* sv;

  QPushButton* rectangleButton;
  QPushButton* ellipseButton;
  QPushButton* polygonButton;
  QPushButton* deleteButton;

  vx::filter::Filter2D* filter = nullptr;
  vx::filter::Selection2DMask* mask = nullptr;

  bool rectangleActive = false;
  bool ellipseActive = false;
  bool polygonActive = false;
  bool deleteActive = false;
  bool mousePressed = false;

  QVector<QPointF> previewPolygon;
  QVector<QPointF> polygon;
  vx::Vector<double, 2> startRect;
  vx::Vector<double, 2> middlePointEllipse;
  QPoint start;
  bool firstValue = true;

  QSharedPointer<ToolSelectionLayer> layer_;
};
