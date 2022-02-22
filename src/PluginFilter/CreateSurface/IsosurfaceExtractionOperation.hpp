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
#include <Voxie/Data/VolumeNode.hpp>
#include <Voxie/IO/RunFilterOperation.hpp>

class IsosurfaceExtractionOperation : public QObject {
  Q_OBJECT

  QSharedPointer<vx::io::RunFilterOperation> operation;
  QSharedPointer<SurfaceExtractor> extractor;
  float threshold;
  bool inverted;

 public:
  IsosurfaceExtractionOperation(
      const QSharedPointer<vx::io::RunFilterOperation>& operation,
      const QSharedPointer<SurfaceExtractor>& extractor, float threshold,
      bool inverted);
  ~IsosurfaceExtractionOperation();

  void generateModel(vx::VolumeNode* voxelData, vx::VolumeNode* labelData);

 Q_SIGNALS:
  void generationDone(
      vx::VolumeNode* voxelData,
      const QSharedPointer<vx::SurfaceDataTriangleIndexed>& surface,
      QSharedPointer<vx::io::RunFilterOperation> operation);
};
