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

#include <Voxie/Vis/HistogramVisualizerWidget.hpp>
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
 * @brief Aggregates and displays the entries of a table node as a histogram.
 */
class HistogramVisualizer : public vx::visualization::SimpleVisualizer {
  Q_OBJECT
  VX_NODE_IMPLEMENTATION("de.uni_stuttgart.Voxie.Visualizer.Histogram")

 public:
  HistogramVisualizer();
  virtual ~HistogramVisualizer() override;

  void setTable(vx::TableNode* table);
  vx::TableNode* table() const;
  QSharedPointer<vx::TableData> tableData() const;

  vx::SharedFunPtr<RenderFunction> getRenderFunction() override;
  QWidget* mainView() override { return histogramWidget; }

  QSharedPointer<QObject> getPropertyUIData(QString propertyName) override;

 private:
  vx::HistogramVisualizerWidget* histogramWidget = nullptr;

  struct BucketCache {
    std::vector<quint64> buckets;

    // the number of values in the bucket with the most values
    quint64 maximumCount;

    float minValue = 0.f;
    float maxValue = 0.f;
    QSharedPointer<vx::TableData> tableData;
    bool needUpdate = true;
    bool logarithmicX = false;

    void updateBuckets(vx::HistogramProperties& properties);
    void populateHistogramProvider(
        vx::HistogramProvider& histogramProvider) const;
  };

  void updateTable();
  void updateData();

  vx::TableNode* tableNode = nullptr;

  QSharedPointer<BucketCache> cache;
  QSharedPointer<vx::HistogramProvider> histogramProvider;

  QComboBox* columnSelector = nullptr;
};
