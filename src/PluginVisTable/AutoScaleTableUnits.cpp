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

#include "AutoScaleTableUnits.hpp"

#include <PluginVisTable/Prototypes.hpp>
#include <PluginVisTable/TableUtils.hpp>

#include <Voxie/Data/MetricUnit.hpp>
#include <Voxie/Data/Prototypes.hpp>

#include <VoxieBackend/IO/Operation.hpp>
#include <VoxieBackend/IO/OperationRegistry.hpp>

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QFutureWatcher>

#include <array>

using namespace vx::io;
using namespace vx::filters;
using namespace vx;
using namespace TableUtils;

AutoScaleTableUnits::AutoScaleTableUnits()
    : FilterNode(getPrototypeSingleton()),
      properties(new AutoScaleTableUnitsProperties(this)),
      threadPool(new QThreadPool(this)) {
  this->setAutomaticDisplayName("Auto-scale table units");
}

QSharedPointer<io::RunFilterOperation> AutoScaleTableUnits::calculate(
    bool isAutomaticFilterRun) {
  Q_UNUSED(isAutomaticFilterRun);
  QSharedPointer<RunFilterOperation> operation(
      new RunFilterOperation(), [](QObject* obj) { obj->deleteLater(); });

  if (auto tableData = getTableData()) {
    operation->setDescription("Auto-scaling table units");

    OperationRegistry::instance()->addOperation(operation);

    futureTableData = QtConcurrent::run([this, tableData, operation]() {
      return doCalculation(tableData, operation);
    });

    auto futureWatcher = new QFutureWatcher<QSharedPointer<TableData>>();
    futureWatcher->setFuture(futureTableData);
    connect(futureWatcher, &QFutureWatcher<QSharedPointer<TableData>>::finished,
            this, [this, operation]() {
              updateTable(futureTableData.result(), operation);
            });
    connect(futureWatcher, &QFutureWatcher<QSharedPointer<TableData>>::finished,
            futureWatcher, &QObject::deleteLater);
    connect(this, &QObject::destroyed, operation.data(),
            &vx::io::Operation::cancel);
  } else {
    operation->cancel();
  }

  return operation;
}

TableNode* AutoScaleTableUnits::getTable() const {
  return dynamic_cast<TableNode*>(properties->table());
}

QSharedPointer<TableData> AutoScaleTableUnits::getTableData() const {
  if (TableNode* table = getTable()) {
    return qSharedPointerDynamicCast<TableData>(table->data());
  }
  return QSharedPointer<TableData>();
}

QSharedPointer<TableData> AutoScaleTableUnits::doCalculation(
    QSharedPointer<TableData> tableData,
    QSharedPointer<vx::io::Operation> operation) {
  int columnCount = tableData->columns().size();

  std::vector<int> logAverages = computeLogAverages(tableData, operation);

  std::vector<NumberManipulator> manipulators(columnCount);
  std::vector<MetricUnit> targetUnits(columnCount);
  std::vector<double> conversionRates(columnCount);
  QList<TableColumn> outputColumns;

  for (int columnIndex = 0; columnIndex < columnCount; ++columnIndex) {
    auto& column = tableData->columns()[columnIndex];
    manipulators[columnIndex] = getNumberManipulator(*column.type());

    // Determine source unit
    MetricUnit source = column.metadata().value("unit").variant().toString();
    MetricUnit target = source;

    // Find most suitable conversion prefix (taking into account the exponent)
    target.setPrefix(MetricUnit::findClosestValidPrefix(
        logAverages[columnIndex] / source.getExponent() -
        int(source.getPrefix())));

    // Compute conversion rate
    targetUnits[columnIndex] = target;
    conversionRates[columnIndex] = source.getConversionRate(target);

    // Update unit in metadata
    auto metadata = column.metadata();
    if (metadata.contains("unit")) {
      metadata["unit"] = QDBusVariant(targetUnits[columnIndex].toString());
    }
    outputColumns << TableColumn(column.name(), column.type(),
                                 column.displayName(), metadata);
  }

  auto outputData = TableData::create(outputColumns);

  auto update = outputData->createUpdate();

  quint64 progress = 0;
  quint64 rowCount = tableData->rowCount();

  // TODO use multiple threads when performing conversion
  for (auto& row : tableData->getRowsByIndex()) {
    QList<QVariant> outputRow;
    for (int columnIndex = 0; columnIndex < columnCount; ++columnIndex) {
      if (isApplicableColumn(tableData->columns()[columnIndex])) {
        outputRow << manipulators[columnIndex](
            row.data()[columnIndex],
            [&conversionRates, columnIndex](double value) {
              return value * conversionRates[columnIndex];
            });
      } else {
        outputRow << row.data()[columnIndex];
      }
    }
    outputData->addRow(update, outputRow);
    operation->updateProgress(0.5f + 0.5f * ((++progress) / float(rowCount)));
  }

  update->finish(QJsonObject());

  return outputData;
}

std::vector<int> AutoScaleTableUnits::computeLogAverages(
    QSharedPointer<TableData> tableData,
    QSharedPointer<vx::io::Operation> operation) {
  static constexpr quint64 batchSize = 100000;
  static constexpr quint64 logSize = 100;

  auto& columns = tableData->columns();
  int columnCount = columns.size();
  std::vector<int> logAverages(columnCount);

  int applColumnCount = std::count_if(columns.begin(), columns.end(),
                                      &AutoScaleTableUnits::isApplicableColumn);

  for (int columnIndex = 0; columnIndex < columnCount; ++columnIndex) {
    if (!isApplicableColumn(tableData->columns()[columnIndex])) {
      continue;
    }

    QAtomicInt progress = 0;

    threadPool->start(functionalRunnable([tableData, columnIndex, &logAverages,
                                          &progress, operation,
                                          applColumnCount]() {
      std::array<quint64, logSize> logarithms;
      logarithms.fill(0);

      auto extractor =
          getNumberExtractor(*tableData->columns()[columnIndex].type());

      quint64 rowCount = tableData->rowCount();
      quint64 valueCount = 0;

      // Batch row queries to reduce memory usage
      for (quint64 batch = 0; batch < rowCount; batch += batchSize) {
        // Obtain rows from current batch
        auto rows = tableData->getRowsByIndex(batch, batch + batchSize);
        for (int i = 0; i < rows.size(); ++i) {
          // Run extractor on row to obtain value list
          for (auto value : extractor(rows[batch + i].data()[columnIndex])) {
            // Compute base-10 logarithm of current value
            double logValue = std::log10(std::abs(value));
            // Ensure that the logarithm is real before proceeding
            if (!std::isnan(logValue)) {
              // Compute the log-array index at which to increment
              int logIndex = std::round(logValue) + int(logSize) / 2;

              // Increment log-array (while ensuring that index is in bounds)
              logarithms[std::min<int>(std::max(0, logIndex), logSize - 1)]++;
              valueCount++;
            }
          }
        }
      }

      // Compute average logarithm
      qint64 logSum = 0;
      for (quint64 i = 0; i < logarithms.size(); ++i) {
        logSum += qint64(logarithms[i]) * (qint64(i) - qint64(logSize) / 2);
      }
      double logAverage = double(logSum) / valueCount;

      logAverages[columnIndex] = std::round(logAverage);

      operation->updateProgress(0.5f * (++progress / float(applColumnCount)));
    }));
  }

  threadPool->waitForDone();

  return logAverages;
}

void AutoScaleTableUnits::updateTable(
    QSharedPointer<TableData> tableData,
    QSharedPointer<RunFilterOperation> operation) {
  if (tableData != nullptr) {
    QSharedPointer<TableNode> tableNode;

    // TODO: Clean up, don't use childNodes()
    for (Node* childNode : this->childNodes()) {
      if (TableNode* output = dynamic_cast<TableNode*>(childNode)) {
        tableNode = output->thisShared();
        break;
      }
    }

    if (!tableNode) {
      tableNode = createNode<TableNode>();
      if (TableNode* inputTable = getTable()) {
        tableNode->setManualDisplayName(inputTable->manualDisplayName());
      }
      this->addChildNode(tableNode.data());
    }

    tableNode->setData(tableData);
  }
  operation->emitFinished();
}

bool AutoScaleTableUnits::isApplicableColumn(const TableColumn& column) {
  return isPolyNumericType(*column.type()) &&
         MetricUnit(column.metadata().value("unit").variant().toString());
}

NODE_PROTOTYPE_IMPL(AutoScaleTableUnits)
