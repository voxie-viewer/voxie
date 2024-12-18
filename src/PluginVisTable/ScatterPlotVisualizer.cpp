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

#include <PluginVisTable/LabeledAxis.hpp>
#include <PluginVisTable/PercentileExtractor.hpp>
#include <PluginVisTable/Prototypes.hpp>
#include <PluginVisTable/ScatterPlotVisualizer.hpp>
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

VX_NODE_INSTANTIATION(ScatterPlotVisualizer)

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

ScatterPlotVisualizer::ScatterPlotVisualizer()
    : SimpleVisualizer(getPrototypeSingleton()),
      properties(new ScatterPlotProperties(this)) {
  view()->setMinimumSize(300 / 96.0 * this->view()->logicalDpiX(),
                         200 / 96.0 * this->view()->logicalDpiY());

  cache = decltype(cache)::create();
  histogramProvider = decltype(histogramProvider)::create();

  QWidget* section = new QWidget;
  section->setLayout(new QVBoxLayout());
  section->layout()->setContentsMargins(0, 0, 0, 0);
  section->setWindowTitle(tr("Mapping"));
  addPropertySection(section);

  columnXSelector = new QComboBox;
  section->layout()->addWidget(labeledWidget(
      properties->columnXProperty()->displayName(), columnXSelector));

  columnYSelector = new QComboBox;
  section->layout()->addWidget(labeledWidget(
      properties->columnYProperty()->displayName(), columnYSelector));

  columnColorSelector = new QComboBox;
  section->layout()->addWidget(labeledWidget(
      properties->columnColorProperty()->displayName(), columnColorSelector));

  connect(properties, &ScatterPlotProperties::logarithmicXChanged, this,
          &ScatterPlotVisualizer::updatePoints);

  connect(properties, &ScatterPlotProperties::logarithmicYChanged, this,
          &ScatterPlotVisualizer::updatePoints);

  connect(properties, &ScatterPlotProperties::pointLimitChanged, this,
          &ScatterPlotVisualizer::updatePoints);

  connect(properties, &ScatterPlotProperties::pointScaleChanged, this,
          &ScatterPlotVisualizer::updatePoints);

  using Props = ScatterPlotProperties;

  // Connect all relevant event handlers
  auto connectColumnProperty = [this](QComboBox* comboBox,
                                      void (Props::*propSetter)(QString),
                                      void (Props::*propSignal)(QString)) {
    // Connect property to combo box
    connect(properties, propSignal, this, [=](QString columnName) {
      comboBox->setCurrentIndex(comboBox->findData(columnName));
      updatePoints();
    });

    auto currentIndexChanged =
        static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged);

    // Connect combo box to property
    connect(comboBox, currentIndexChanged, this, [=](int index) {
      (this->properties->*propSetter)(comboBox->itemData(index).toString());
      updatePoints();
    });
  };

  connectColumnProperty(columnXSelector, &ScatterPlotProperties::setColumnX,
                        &ScatterPlotProperties::columnXChanged);

  connectColumnProperty(columnYSelector, &ScatterPlotProperties::setColumnY,
                        &ScatterPlotProperties::columnYChanged);

  connectColumnProperty(columnColorSelector,
                        &ScatterPlotProperties::setColumnColor,
                        &ScatterPlotProperties::columnColorChanged);

  connect(properties, &ScatterPlotProperties::colorMapChanged, this,
          &ScatterPlotVisualizer::triggerRedraw);

  connect(properties, &ScatterPlotProperties::viewPercentileChanged, this,
          &ScatterPlotVisualizer::updatePoints);

  connect(properties, &ScatterPlotProperties::viewMarginChanged, this,
          &ScatterPlotVisualizer::updatePoints);

  connect(properties, &ScatterPlotProperties::tableChanged, this,
          [this](Node* newParent) {
            setTable(dynamic_cast<TableNode*>(newParent));
          });

  updateTable();
}

ScatterPlotVisualizer::~ScatterPlotVisualizer() {}

void ScatterPlotVisualizer::setTable(TableNode* table) {
  if (this->tableNode != nullptr) {
    disconnect(this->tableNode, &DataNode::dataChangedFinished, this,
               &ScatterPlotVisualizer::updateTable);
  }

  this->tableNode = table;

  if (table != nullptr) {
    connect(table, &DataNode::dataChangedFinished, this,
            &ScatterPlotVisualizer::updateTable);

    updateTable();
  }
}

TableNode* ScatterPlotVisualizer::table() const { return this->tableNode; }

QSharedPointer<TableData> ScatterPlotVisualizer::tableData() const {
  return table() != nullptr ? table()->tableData()
                            : QSharedPointer<TableData>();
}

vx::SharedFunPtr<VisualizerNode::RenderFunction>
ScatterPlotVisualizer::getRenderFunction() {
  return [cache = this->cache, histogramProvider = this->histogramProvider](
             const QSharedPointer<ImageDataPixel>& outputImage,
             const VectorSizeT2& outputRegionStart, const VectorSizeT2& size,
             const QSharedPointer<ParameterCopy>& parameters,
             const QSharedPointer<VisualizerRenderOptions>& options) {
    Q_UNUSED(options);

    static const QColor backgroundColor = qRgb(20, 20, 20);
    static const QColor axisColor = qRgb(200, 200, 200);

    static const float strokeWidth = 1.f;
    static const float axisMarginsBL = 50.f;
    static const float axisMarginsTR = 40.f;

    static const float intThresh = 10.f;

    QRectF rect(axisMarginsBL, axisMarginsTR,
                size.x - axisMarginsTR - axisMarginsBL,
                size.y - axisMarginsTR - axisMarginsBL);

    ScatterPlotPropertiesCopy properties(
        parameters->properties()[parameters->mainNodePath()]);

    if (cache->needUpdate) {
      // Recompute coordinate list from table and update histogram provider
      cache->update(properties);
      cache->updateHistogramProvider(*histogramProvider);
      cache->needUpdate = false;
    }

    QImage image(static_cast<int>(size.x), static_cast<int>(size.y),
                 QImage::Format_ARGB32);

    // Create colorizer
    vx::Colorizer colorizer;
    colorizer.setEntries(properties.colorMap());

    // Determine zoom factor
    QVector2D factor(rect.width(), rect.height());
    factor /= (cache->max - cache->min);
    factor *= QVector2D(1, -1);

    // Determine view offset
    QVector2D offset(rect.topLeft());
    offset -= cache->min * factor;
    offset += QVector2D(0, rect.height());

    QPainter painter(&image);
    painter.setClipRect(rect);

    image.fill(backgroundColor);

    float pScale = properties.pointScale();

    // Draw points
    for (auto& point : cache->points) {
      auto pos = point.position * factor + offset;
      pos -= QVector2D(pScale * 0.5f, pScale * 0.5f);

      painter.setPen(
          QPen(colorizer.getColor(point.colorValue).asQColor(), strokeWidth));
      painter.drawRect(QRectF(pos.x(), pos.y(), pScale, pScale));
    }

    // Determine inverse view transformation function
    auto reverseTransformX = [factor, offset, &properties](float val) {
      val = (val - offset.x()) / factor.x();
      return properties.logarithmicX() ? expf(val) : val;
    };

    auto reverseTransformY = [factor, offset, &properties](float val) {
      val = (val - offset.y()) / factor.y();
      return properties.logarithmicY() ? expf(val) : val;
    };

    // Determine original (non-logarithmic) value boundaries
    auto unscale = [&properties](QVector2D v) {
      return QVector2D(properties.logarithmicX() ? expf(v.x()) : v.x(),
                       properties.logarithmicY() ? expf(v.y()) : v.y());
    };
    QVector2D unscaledMin = unscale(cache->min);
    QVector2D unscaledMax = unscale(cache->max);

    QRectF axisRect(rect.left(), rect.bottom(), rect.width(), -rect.height());

    painter.setClipping(false);

    using TableUtils::getColumnLabel;

    // Draw X-axis
    LabeledAxis axisX(LabeledAxis::Horizontal);
    axisX.setColor(axisColor);
    axisX.setLowerValue(reverseTransformX(rect.left()));
    axisX.setUpperValue(reverseTransformX(rect.right()));
    axisX.setRect(axisRect);
    axisX.setLogScale(properties.logarithmicX());
    axisX.setIntegerLabels(unscaledMax.x() - unscaledMin.x() > intThresh);
    axisX.setLabel(getColumnLabel(cache->tableData, properties.columnX()));
    axisX.draw(painter);

    // Draw Y-axis
    LabeledAxis axisY(LabeledAxis::Vertical);
    axisY.setColor(axisColor);
    axisY.setLowerValue(reverseTransformY(rect.bottom()));
    axisY.setUpperValue(reverseTransformY(rect.top()));
    axisY.setRect(axisRect);
    axisY.setLogScale(properties.logarithmicY());
    axisY.setIntegerLabels(unscaledMax.y() - unscaledMin.y() > intThresh);
    axisY.setLabel(getColumnLabel(cache->tableData, properties.columnY()));
    axisY.draw(painter);

    outputImage->fromQImage(image, outputRegionStart);
  };
}

QSharedPointer<QObject> ScatterPlotVisualizer::getPropertyUIData(
    QString propertyName) {
  if (propertyName ==
      "de.uni_stuttgart.Voxie.Visualizer.ScatterPlot.ColorMap") {
    return histogramProvider;
  } else {
    return Node::getPropertyUIData(propertyName);
  }
}

void ScatterPlotVisualizer::updateTable() {
  QString title = "Scatter plot";

  if (table() != nullptr) {
    // Pass table data to point cache
    cache->tableData = tableData();

    // Update inner window title
    title += " - " + table()->displayName();

    // Save the current X, Y and color columns
    auto currentColumnX = properties->columnX();
    auto currentColumnY = properties->columnY();
    auto currentColumnColor = properties->columnColor();

    columnXSelector->clear();
    columnYSelector->clear();
    columnColorSelector->clear();

    // Populate the column selection comboboxes
    if (auto tableData = this->tableData()) {
      for (auto& column : tableData->columns()) {
        if (TableUtils::isNumericType(*column.type())) {
          columnXSelector->addItem(column.displayName(), column.name());
          columnYSelector->addItem(column.displayName(), column.name());
          columnColorSelector->addItem(column.displayName(), column.name());
        }
      }
    }

    // Restore the X and Y columns
    properties->setColumnX(currentColumnX);
    properties->setColumnY(currentColumnY);
    properties->setColumnColor(currentColumnColor);
  }

  setAutomaticDisplayName(title);

  updatePoints();
}

void ScatterPlotVisualizer::updatePoints() {
  cache->needUpdate = true;
  triggerRedraw();
}

void ScatterPlotVisualizer::PointCache::update(
    ScatterPlotPropertiesCopy properties) {
  points.clear();
  min = {};
  max = {};

  if (tableData) {
    int xColumn = tableData->getColumnIndexByName(properties.columnX()),
        yColumn = tableData->getColumnIndexByName(properties.columnY()),
        cColumn = tableData->getColumnIndexByName(properties.columnColor());

    auto isValidColumnIndex = [this](int index) {
      return index >= 0 && index < tableData->columns().size();
    };

    // Ensure that column indices are not out of range
    if (!isValidColumnIndex(xColumn) || !isValidColumnIndex(yColumn)) {
      return;
    }

    bool isColorized = isValidColumnIndex(cColumn);

    // Compute maximum number of rows and allocate memory for point list
    quint64 rowCount = std::min(static_cast<quint64>(properties.pointLimit()),
                                tableData->rowCount());
    points.reserve(rowCount);

    // Define point mapping function
    auto rowToPoint = [xColumn, yColumn](const TableRow& row) {
      return QVector2D(row.data()[xColumn].toDouble(),
                       row.data()[yColumn].toDouble());
    };

    // Define color mapping function
    auto rowToColor = [cColumn](const TableRow& row) -> float {
      return row.data()[cColumn].toDouble();
    };

    // Obtain row list
    auto rows = tableData->getRowsByIndex(0, rowCount);

    for (auto& row : rows) {
      QVector2D point = rowToPoint(row);
      // Skip NaN entries
      if (point.x() == point.x() && point.y() == point.y()) {
        points.push_back({point, isColorized ? rowToColor(row) : 0.f});
      }
    }

    auto pointGetX = [](const Point& p) { return p.position.x(); };
    auto pointGetY = [](const Point& p) { return p.position.y(); };

    auto pointSetX = [](Point& p, float v) { p.position.setX(v); };
    auto pointSetY = [](Point& p, float v) { p.position.setY(v); };

    // Apply per-axis logarithmic scaling
    auto applyLogScale = [=](auto& getter, auto& setter) {
      // Remove entries <= 0 to avoid NaNs
      points.erase(
          std::remove_if(points.begin(), points.end(),
                         [=](auto& point) { return getter(point) <= 0.f; }),
          points.end());
      for (auto& point : points) {
        setter(point, logf(getter(point)));
      }
    };

    if (properties.logarithmicX()) {
      applyLogScale(pointGetX, pointSetX);
    }
    if (properties.logarithmicY()) {
      applyLogScale(pointGetY, pointSetY);
    }

    // Compute view range by percentile
    float percentile = 0.5f * (1.f + (properties.viewPercentile() / 100.f));

    using PercentileExtractor::findPercentile;

    min.setX(findPercentile(points, 1.f - percentile, pointGetX));
    min.setY(findPercentile(points, 1.f - percentile, pointGetY));
    max.setX(findPercentile(points, percentile, pointGetX));
    max.setY(findPercentile(points, percentile, pointGetY));

    // Add relative margins
    float margin = properties.viewMargin() / 100.f;
    QVector2D range(max.x() - min.x(), max.y() - min.y());
    min -= margin * range;
    max += margin * range;
  }
}

void ScatterPlotVisualizer::PointCache::updateHistogramProvider(
    HistogramProvider& histogramProvider) {
  histogramProvider.setDataFromContainer(
      HistogramProvider::DefaultBucketCount, points,
      [](const Point& point) { return point.colorValue; });
}
