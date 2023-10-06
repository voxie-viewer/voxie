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

#include <Voxie/Data/Colorizer.hpp>
#include <Voxie/Data/PropertySection.hpp>
#include <Voxie/Data/TableData.hpp>
#include <Voxie/Data/TableNode.hpp>
#include <Voxie/Node/FilterNode.hpp>
#include <VoxieBackend/Data/HistogramProvider.hpp>
#include <VoxieBackend/Data/SurfaceData.hpp>

#include <Voxie/IO/RunFilterOperation.hpp>

#include <PluginFilter/Prototypes.forward.hpp>

#include <QtDBus/QDBusAbstractAdaptor>

namespace vx {
class PropertySection;
// class Colorizer;

namespace filters {

/**
 * @brief This class colorizes a labeled Surface.
 */
class ColorizeLabeledSurface : public FilterNode {
  NODE_PROTOTYPE_DECL(ColorizeLabeledSurface)

 public:
  ColorizeLabeledSurface();

 private:
  TableNode* tableNode = nullptr;

  QComboBox* columnKeySelector = nullptr;
  QComboBox* columnValueSelector = nullptr;

  QSharedPointer<HistogramProvider> histogramProvider;

  QSharedPointer<vx::io::RunFilterOperation> calculate(
      bool isAutomaticFilterRun) override;

  void setTable(TableNode* table);

  QSharedPointer<TableData> getTableData() const;

  QSharedPointer<QObject> getPropertyUIData(QString propertyName) override;

  void updateTable();

  void updateSelectedColumn();

  void updateSurface(
      const QSharedPointer<SurfaceDataTriangleIndexed>& outputSurface,
      QSharedPointer<vx::io::RunFilterOperation> operation);
};
}  // namespace filters
}  // namespace vx
