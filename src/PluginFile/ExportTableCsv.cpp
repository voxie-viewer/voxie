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

#include "ExportTableCsv.hpp"

#include <Voxie/Data/TableData.hpp>

#include <VoxieBackend/Property/PropertyType.hpp>

#include <QtCore/QFile>

using namespace vx;
using namespace vx::file;

static inline QString escapeCsvField(QString str) {
  return "\"" + str.replace('\"', "\"\"") + "\"";
}

static void saveCsvToFile(TableData* data, QFile& file) {
  if (!file.open(QIODevice::WriteOnly)) {
    qWarning() << "Unable to open file" << file.errorString();
    return;
  }

  // Write column headers
  QTextStream output(&file);
  for (int i = 0; i < data->columns().size(); i++) {
    const auto& column = data->columns()[i];
    if (i > 0) {
      output << ",";
    }

    QString unitSuffix =
        column.metadata().contains("unit")
            ? "[" + column.metadata()["unit"].variant().toString() + "]"
            : "";
    output << escapeCsvField(column.name() + unitSuffix);
  }
  output << "\n";

  // Write data rows
  for (const auto& row : data->getRowsByIndex()) {
    for (int i = 0; i < data->columns().size(); i++) {
      const auto& column = data->columns()[i];
      if (i > 0) {
        output << ",";
      }
      output << escapeCsvField(column.type()->valueToString(row.data()[i]));
    }
    output << "\n";
  }
}

ExportTableCsv::ExportTableCsv()
    : vx::io::Exporter(
          "de.uni_stuttgart.Voxie.FileFormat.Table.Csv.Export",
          vx::io::FilenameFilter("Comma-separated values", {"*.csv"}),
          {"de.uni_stuttgart.Voxie.Data.Table"}) {}

QSharedPointer<vx::OperationResult> ExportTableCsv::exportData(
    const QSharedPointer<vx::Data>& data, const QString& fileName) {
  return runThreaded(
      "Export " + fileName,
      [data, fileName](const QSharedPointer<vx::io::Operation>& op) {
        auto tableData = qSharedPointerDynamicCast<vx::TableData>(data);
        if (!tableData)
          throw vx::Exception(
              "de.uni_stuttgart.Voxie.InternalError",
              "ExportTableCsv::exportData(): data is not a TableData");

        (void)op;  // TODO: progress bar
        QFile file(fileName);
        saveCsvToFile(tableData.data(), file);
      });
}
