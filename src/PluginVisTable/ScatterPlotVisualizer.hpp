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

#include <Voxie/Data/TableNode.hpp>
#include <VoxieBackend/Data/HistogramProvider.hpp>

#include <PluginVisTable/Prototypes.forward.hpp>

#include <Voxie/Vis/VisualizerNode.hpp>
#include <Voxie/Vis/VisualizerView.hpp>

#include <QtCore/QObject>
#include <QtCore/QVarLengthArray>

#include <QtGui/QPaintEvent>

#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>

class ImageSelectionWidget;

/**
 * @brief Displays the entries of a table node as a scatter plot, using
 * customizable columns for the X and Y coordinates of each point.
 */
class ScatterPlotVisualizer : public vx::visualization::SimpleVisualizer {
  Q_OBJECT
  VX_NODE_IMPLEMENTATION("de.uni_stuttgart.Voxie.Visualizer.ScatterPlot")

 public:
  ScatterPlotVisualizer();
  virtual ~ScatterPlotVisualizer() override;

  void setTable(vx::TableNode* table);
  vx::TableNode* table() const;
  QSharedPointer<vx::TableData> tableData() const;

  vx::SharedFunPtr<RenderFunction> getRenderFunction() override;

  QSharedPointer<QObject> getPropertyUIData(QString propertyName) override;

 private:
  struct PointCache {
    struct Point {
      QVector2D position;
      float colorValue;
    };
    std::vector<Point> points;
    QVector2D min;
    QVector2D max;
    QSharedPointer<vx::TableData> tableData;
    bool needUpdate = true;

    void update(vx::ScatterPlotPropertiesCopy properties);
    void updateHistogramProvider(vx::HistogramProvider& histogramProvider);
  };

  void updateTable();
  void updatePoints();

  vx::TableNode* tableNode = nullptr;

  QSharedPointer<PointCache> cache;
  QSharedPointer<vx::HistogramProvider> histogramProvider;

  QComboBox* columnXSelector = nullptr;
  QComboBox* columnYSelector = nullptr;
  QComboBox* columnColorSelector = nullptr;
};
