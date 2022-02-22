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

#include <VoxieBackend/Data/FloatImage.hpp>

#include <QtCore/QRunnable>

#include <QtGui/QImage>

namespace vx {
class Colorizer;
class ColorizerEntry;
}  // namespace vx

/**
 * Provides a QRunnable implementation to execute the colorizing process of a
 * Colorizer.
 * @author Hans Martin Berner
 */
class ColorizerWorker : public QObject, public QRunnable {
  Q_OBJECT
 public:
  /**
   * Takes a source image and a colorizer and clones them to make sure no
   * inconsistencies occur during threaded execution.
   * @param sourceImage the image to be colorized
   * @param colorizer the colorizer including the settings
   */
  ColorizerWorker(vx::FloatImage sourceImage,
                  const QList<vx::ColorizerEntry>& mappings);
  void run() override;

 Q_SIGNALS:
  /**
   * Signals that the thread has finished processing the image.
   * @param image the resulting image
   */
  void imageColorized(QImage image);

 private:
  vx::FloatImage sourceImage;
  vx::Colorizer* colorizer;
};
