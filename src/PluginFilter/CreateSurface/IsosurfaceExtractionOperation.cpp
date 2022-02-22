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

#include "IsosurfaceExtractionOperation.hpp"

#include <PluginFilter/CreateSurface/SurfaceExtractor.hpp>
#include <Voxie/Data/VolumeNode.hpp>
#include <VoxieBackend/Data/SurfaceData.hpp>
#include <VoxieBackend/Data/VolumeDataVoxel.hpp>

#include <QApplication>
#include <QPointer>

using namespace vx;
using namespace vx::io;

IsosurfaceExtractionOperation::IsosurfaceExtractionOperation(
    const QSharedPointer<RunFilterOperation>& operation,
    const QSharedPointer<SurfaceExtractor>& extractor, float threshold,
    bool inverted)
    : operation(operation),
      extractor(extractor),
      threshold(threshold),
      inverted(inverted) {}
IsosurfaceExtractionOperation::~IsosurfaceExtractionOperation() {}

void IsosurfaceExtractionOperation::generateModel(VolumeNode* dataSet,
                                                  VolumeNode* dataSetLabel) {
  try {
    auto voxelData =
        qSharedPointerDynamicCast<VolumeDataVoxel>(dataSet->volumeData());

    if (voxelData) {
      auto labelData = dataSetLabel
                           ? qSharedPointerDynamicCast<VolumeDataVoxel>(
                                 dataSetLabel->volumeData())
                           : QSharedPointer<VolumeDataVoxel>();

      auto surface =
          extractor->extract(operation, voxelData.data(), labelData.data(),
                             this->threshold, this->inverted);
      surface->moveToThread(QApplication::instance()->thread());
      Q_EMIT generationDone(dataSet, surface, operation);
    } else {
      // TODO: Non-voxel datasets?
      qWarning()
          << "Input dataset for isosurface extraction is not a VolumeDataVoxel";
      Q_EMIT generationDone(
          dataSet, QSharedPointer<SurfaceDataTriangleIndexed>(), operation);
    }
  } catch (vx::OperationCancelledException&) {
    Q_EMIT generationDone(dataSet, QSharedPointer<SurfaceDataTriangleIndexed>(),
                          operation);
  }
}
