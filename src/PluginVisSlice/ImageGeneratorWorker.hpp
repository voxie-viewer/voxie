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

#include <Voxie/Data/Slice.hpp>

#include <VoxieBackend/Data/PlaneInfo.hpp>
#include <VoxieBackend/Data/SliceImage.hpp>
#include <VoxieBackend/Data/VolumeData.hpp>

#include <QtCore/QRunnable>

/**
 * Provides a QRunnable implementation to allow threaded execution of the image
 * generation procedure.
 * @author Hans Martin Berner
 */

class ImageGeneratorWorker : public QObject, public QRunnable {
  Q_OBJECT
 public:
  /**
   * A QRunnable object used to generate an image out of given slice
   * configuration. The slice object is not cloned.
   * @param slice the slice including the settings
   * @param sliceArea the subarea in the slice to be rendered
   * @param imageSize the targeted image size of the output image
   * @param interpolation method to be used to achieve the target size
   */
  ImageGeneratorWorker(const QSharedPointer<vx::VolumeData>& data,
                       const vx::PlaneInfo& plane, const QRectF& sliceArea,
                       const QSize& imageSize,
                       vx::InterpolationMethod interpolation);

  ~ImageGeneratorWorker();

  void run() override;

 Q_SIGNALS:
  /**
   * Signals that the run method has finished generating the image.
   * @param si represents the generated image
   */
  void imageGenerated(vx::SliceImage si);

 private:
  QSharedPointer<vx::VolumeData> data;
  vx::PlaneInfo plane;
  QRectF sliceArea;
  QSize imageSize;
  vx::InterpolationMethod interpolation;
  // static volatile int started;
  // static volatile int finished;
};
