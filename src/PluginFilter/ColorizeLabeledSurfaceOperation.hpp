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

#include <PluginFilter/CreateSurface/SurfaceExtractor.hpp>
#include <QObject>
#include <Voxie/Data/Colorizer.hpp>
#include <Voxie/Data/SurfaceNode.hpp>
#include <Voxie/Data/TableData.hpp>
#include <Voxie/IO/RunFilterOperation.hpp>
#include <unordered_map>

class ColorizeLabeledSurfaceOperation : public QObject {
  Q_OBJECT

  QSharedPointer<vx::io::RunFilterOperation> operation;

 public:
  ColorizeLabeledSurfaceOperation(
      const QSharedPointer<vx::io::RunFilterOperation>& operation);
  ~ColorizeLabeledSurfaceOperation();

  void colorizeModel(vx::SurfaceNode* voxelData, vx::Colorizer* colorizer,
                     QSharedPointer<vx::TableData> inputTable,
                     int keyColumnIndex, int valueColumnIndex);

  void generateColorData(
      vx::Colorizer* colorizer, QSharedPointer<vx::SurfaceAttribute> labelIds,
      QVector<quint32> randomValues,
      const QSharedPointer<vx::SurfaceAttribute>& outputAttribute);

 private:
  std::unordered_map<int, float> rowMapping;

 Q_SIGNALS:
  void colorizationDone(
      const QSharedPointer<vx::SurfaceDataTriangleIndexed> outputSurface,
      QSharedPointer<vx::io::RunFilterOperation> operation);
};
