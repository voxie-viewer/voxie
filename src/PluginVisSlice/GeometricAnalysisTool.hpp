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
#include <PluginVisSlice/Visualizer2DTool.hpp>

#include <QtWidgets/QPushButton>

/**
 * @brief The GeometricAnalysisTool class provides ways to measure and display
 * values in an image.
 * @author David Haegele, Hans Martin Berner, Tilman Schalke
 */

class SliceVisualizer;

namespace vx {
class GeometricPrimitiveData;
}  // namespace vx

class GeometricAnalysisTool : public Visualizer2DTool {
  Q_OBJECT

 public:
  GeometricAnalysisTool(QWidget* parent, SliceVisualizer* sv);
  ~GeometricAnalysisTool() {}

  QIcon getIcon() override { return QIcon(":/icons/ruler-triangle.png"); }
  QString getName() override { return "Geometric Analysis"; }

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

 private:
  SliceVisualizer* sv;
  QPushButton* valueButton;
  bool mousePressed;
  QPoint startPos;
};

class GeometricAnalysisLayer : public Layer {
  Q_OBJECT
  REFCOUNTEDOBJ_DECL(GeometricAnalysisLayer)

 public:
  GeometricAnalysisLayer(SliceVisualizer* sv);
  ~GeometricAnalysisLayer() {}

  // QIcon getIcon() override { return QIcon(":/icons/ruler-triangle.png"); }
  QString getName() override {
    return "de.uni_stuttgart.Voxie.SliceVisualizer.Layer.GeometricAnalysis";
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

 private:
  static void drawPoint(QImage& outputImage,
                        vx::SlicePropertiesBase* properties, float visibility,
                        QPointF point, float distance, std::string name,
                        QVector3D vector, bool isSelected);
  void changePointName(QPoint point);
  /**
   * @brief redrawPoints redraws all the points that have been saved
   */
  static void redrawPoints(
      QImage& outputImage, vx::SlicePropertiesBase* properties,
      float visibility, const QSharedPointer<vx::GeometricPrimitiveData>& gpd,
      quint64 currentPointID);
  /**
   * @brief drawLine draws the line between the two points if they are in the
   * region of visibility around the slice
   * @param p1
   * @param p2
   */
  static void drawLine(QImage& outputImage, vx::SlicePropertiesBase* properties,
                       float visibility, QVector3D p1, QVector3D p2);

  static void drawEverything(QImage& outputImage,
                             vx::SlicePropertiesBase* properties,
                             float visibility);

 private:
  SliceVisualizer* sv;
  float visibility;

  QPoint mousePos;
  bool mousePosValid = false;
};
