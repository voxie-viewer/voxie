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

// QDBusConnection should be included as early as possible:
// https://bugreports.qt.io/browse/QTBUG-48351
#include <QtDBus/QDBusConnection>

#include <PluginVisTable/HistogramVisualizer.hpp>
#include <PluginVisTable/LabeledAxis.hpp>
#include <PluginVisTable/PercentileExtractor.hpp>
#include <PluginVisTable/Prototypes.hpp>
#include <PluginVisTable/TableUtils.hpp>

#include <Voxie/Data/Colorizer.hpp>
#include <Voxie/Data/TableColumnListView.hpp>
#include <Voxie/Data/TableData.hpp>
#include <VoxieBackend/Data/ImageDataPixel.hpp>

#include <Voxie/Node/ParameterCopy.hpp>
#include <Voxie/Node/PropertyHelper.hpp>
#include <Voxie/Node/PropertyUI.hpp>

#include <Voxie/IVoxie.hpp>

#include <QtCore/QDebug>
#include <QtCore/QMap>
#include <QtCore/QMetaType>

#include <QtGui/QColor>
#include <QtGui/QOpenGLPaintDevice>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QRgb>

#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSizePolicy>

#include <cmath>

using namespace vx;
using namespace vx::visualization;

static QWidget* labeledWidget(QString labelText, QWidget* widget) {
  QWidget* labeledWidget = new QWidget;
  labeledWidget->setLayout(new QHBoxLayout);
  labeledWidget->layout()->setContentsMargins(0, 0, 0, 0);

  QLabel* label = new QLabel(labelText + ":");
  labeledWidget->layout()->addWidget(label);
  labeledWidget->layout()->addWidget(widget);

  return labeledWidget;
}

HistogramVisualizer::HistogramVisualizer()
    : SimpleVisualizer(getPrototypeSingleton()),
      properties(new visualizer_prop::HistogramProperties(this)) {
  view()->setMinimumSize(300 / 96.0 * this->view()->logicalDpiX(),
                         200 / 96.0 * this->view()->logicalDpiY());

  cache = decltype(cache)::create();
  histogramProvider = decltype(histogramProvider)::create();

  histogramWidget = new vx::HistogramVisualizerWidget();
  histogramWidget->setHistogramProvider(histogramProvider);
  histogramWidget->setAutomaticBucketLowerBound(false);
  histogramWidget->setAutomaticBucketUpperBound(false);

  QWidget* section = new QWidget;
  section->setLayout(new QVBoxLayout());
  section->layout()->setContentsMargins(0, 0, 0, 0);
  section->setWindowTitle(tr("Mapping"));
  addPropertySection(section);

  columnSelector = new QComboBox;
  section->layout()->addWidget(labeledWidget(
      properties->columnProperty()->displayName(), columnSelector));

  connect(properties, &HistogramProperties::logarithmicXChanged, this,
          &HistogramVisualizer::updateData);

  connect(properties, &HistogramProperties::logarithmicYChanged, this,
          &HistogramVisualizer::updateData);

  connect(properties, &HistogramProperties::bucketCountChanged, this,
          &HistogramVisualizer::updateData);

  using Props = HistogramProperties;

  // Connect all relevant event handlers
  auto connectColumnProperty = [this](QComboBox* comboBox,
                                      void (Props::*propSetter)(QString),
                                      void (Props::*propSignal)(QString)) {
    // Connect property to combo box
    connect(properties, propSignal, this, [=](QString columnName) {
      comboBox->setCurrentIndex(comboBox->findData(columnName));
      updateData();
    });

    auto currentIndexChanged =
        static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged);

    // Connect combo box to property
    connect(comboBox, currentIndexChanged, this, [=](int index) {
      (this->properties->*propSetter)(comboBox->itemData(index).toString());
      updateData();
    });
  };

  connectColumnProperty(columnSelector, &HistogramProperties::setColumn,
                        &HistogramProperties::columnChanged);

  connect(properties, &HistogramProperties::colorMapChanged, this,
          &HistogramVisualizer::updateData);

  connect(properties, &HistogramProperties::lowerBoundXFractionChanged, this,
          &HistogramVisualizer::updateData);
  connect(properties, &HistogramProperties::upperBoundXFractionChanged, this,
          &HistogramVisualizer::updateData);

  connect(properties, &HistogramProperties::tableChanged, this,
          [this](Node* newParent) {
            setTable(dynamic_cast<TableNode*>(newParent));
          });

  updateTable();
}

HistogramVisualizer::~HistogramVisualizer() {}

void HistogramVisualizer::setTable(TableNode* table) {
  if (this->tableNode != nullptr) {
    disconnect(this->tableNode, &DataNode::dataChangedFinished, this,
               &HistogramVisualizer::updateTable);
  }

  this->tableNode = table;

  if (table != nullptr) {
    connect(table, &DataNode::dataChangedFinished, this,
            &HistogramVisualizer::updateTable);

    updateTable();
  }
}

TableNode* HistogramVisualizer::table() const { return this->tableNode; }

QSharedPointer<TableData> HistogramVisualizer::tableData() const {
  return table() != nullptr ? table()->tableData()
                            : QSharedPointer<TableData>();
}

vx::SharedFunPtr<VisualizerNode::RenderFunction>
HistogramVisualizer::getRenderFunction() {
  return [](const QSharedPointer<ImageDataPixel>& outputImage,
            const VectorSizeT2& outputRegionStart, const VectorSizeT2& size,
            const QSharedPointer<ParameterCopy>& parameters,
            const QSharedPointer<VisualizerRenderOptions>& options) {
    Q_UNUSED(options);

    HistogramPropertiesCopy properties(
        parameters->properties()[parameters->mainNodePath()]);

    QImage image(static_cast<int>(size.x), static_cast<int>(size.y),
                 QImage::Format_ARGB32);
    image.fill(qRgba(0, 0, 0, 0));

    outputImage->fromQImage(image, outputRegionStart);
  };
}

QSharedPointer<QObject> HistogramVisualizer::getPropertyUIData(
    QString propertyName) {
  if (propertyName == "de.uni_stuttgart.Voxie.Visualizer.Histogram.ColorMap") {
    return histogramProvider;
  } else {
    return Node::getPropertyUIData(propertyName);
  }
}

void HistogramVisualizer::updateTable() {
  QString title = "Histogram";

  if (table() != nullptr) {
    // Pass table data to point cache
    cache->tableData = tableData();

    // Update inner window title
    title += " - " + table()->displayName();

    // Save the current data column
    auto currentColumn = properties->column();

    columnSelector->clear();

    // Populate the column selection combobox
    if (auto tableData = this->tableData()) {
      for (auto& column : tableData->columns()) {
        if (TableUtils::isNumericType(*column.type())) {
          columnSelector->addItem(column.displayName(), column.name());
        }
      }
    }

    // Restore the data column
    properties->setColumn(currentColumn);
  }

  setAutomaticDisplayName(title);

  updateData();
}

void HistogramVisualizer::updateData() {
  histogramWidget->setYAxisLogScale(properties->logarithmicY());
  QSharedPointer<Colorizer> colorizer = makeSharedQObject<Colorizer>();
  colorizer->setEntries(properties->colorMap());
  histogramWidget->setColorizer(colorizer);
  cache->updateBuckets(*properties);
  cache->populateHistogramProvider(*histogramProvider);

  double lowerBound = properties->lowerBoundXFraction();
  double upperBound = properties->upperBoundXFraction();

  // if the user sets the upper bound to be lower than the lower bound we just
  // treat the upper bound as lower bound and vice-versa
  if (upperBound < lowerBound) {
    double temp = lowerBound;
    lowerBound = upperBound;
    upperBound = temp;
  }

  auto histogramData = histogramProvider->getData();
  double difference = histogramData->maximumValue - histogramData->minimumValue;
  histogramWidget->setBucketLowerBoundOverrideValue(
      histogramData->bucketIndexOfValue(histogramData->minimumValue +
                                        difference * lowerBound));
  histogramWidget->setBucketUpperBoundOverrideValue(
      histogramData->bucketIndexOfValue(histogramData->minimumValue +
                                        difference * upperBound));

  cache->needUpdate = false;
  triggerRedraw();
}

void HistogramVisualizer::BucketCache::updateBuckets(
    HistogramProperties& properties) {
  buckets.resize(properties.bucketCount());

  for (auto& bucket : buckets) {
    bucket = 0;
  }

  maximumCount = 0;

  if (tableData) {
    int column = tableData->getColumnIndexByName(properties.column());

    // Ensure that column indices are not out of range
    if (column < 0 || column >= tableData->columns().size()) {
      return;
    }

    // Obtain row list
    std::vector<float> values;
    values.reserve(tableData->rowCount());
    for (const auto& row : tableData->getRowsByIndex()) {
      values.push_back(row.data()[column].toFloat());
    }

    logarithmicX = properties.logarithmicX();

    // Filter out non-positive rows if necessary
    if (properties.logarithmicX()) {
      values.erase(std::remove_if(values.begin(), values.end(),
                                  [=](float row) { return row <= 0.f; }),
                   values.end());
    }

    using PercentileExtractor::findPercentile;

    // Identity mapping function
    auto identity = [](float v) { return v; };

    // Determine lower and upper percentile values of table entries
    minValue = findPercentile(values, 1.f, identity);
    maxValue = findPercentile(values, 0.f, identity);

    if (properties.logarithmicX()) {
      minValue = logf(minValue);
      maxValue = logf(maxValue);
    }

    double factor = (buckets.size() - 1) / (maxValue - minValue);

    // Sort data points into buckets
    for (auto value : values) {
      if (properties.logarithmicX()) {
        value = logf(value);
      }

      auto index = qint64((value - minValue) * factor);
      if (index >= 0 && std::size_t(index) < buckets.size()) {
        maximumCount = std::max(maximumCount, ++buckets[index]);
      }
    }
  }
}

void HistogramVisualizer::BucketCache::populateHistogramProvider(
    HistogramProvider& histogramProvider) const {
  auto histogramData = HistogramProvider::DataPtr::create();

  histogramData->xAxisLog = logarithmicX;
  histogramData->minimumValue = minValue;
  histogramData->maximumValue = maxValue;

  histogramData->buckets.reserve(buckets.size());
  for (auto bucket : buckets) {
    histogramData->buckets.push_back(bucket);
    histogramData->maximumCount = maximumCount;
  }
  histogramProvider.setData(histogramData);
}

NODE_PROTOTYPE_IMPL_SEP(visualizer_prop::Histogram, HistogramVisualizer)
