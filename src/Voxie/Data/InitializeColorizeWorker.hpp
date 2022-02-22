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

#include <Voxie/Voxie.hpp>

#include <QtCore/QObject>
#include <QtCore/QRunnable>
#include <QtCore/QSharedPointer>

namespace vx {
class VolumeNode;
class ImageDataPixel;

/**
 * @brief The InitializeColorizeWorker class provides the worker for
 * initializing.
 */
class VOXIECORESHARED_EXPORT InitializeColorizeWorker : public QObject,
                                                        public QRunnable {
  Q_OBJECT
 public:
  InitializeColorizeWorker();
  InitializeColorizeWorker(VolumeNode* dataSet);

  ~InitializeColorizeWorker() {}
  // QRunnable interface
 public:
  /**
   * @brief changeInitSet changes the reference for the init algorithm
   * @param newImage new reference
   */
  void changeInitSet(const QSharedPointer<vx::ImageDataPixel>& newImage);

 private:
  QSharedPointer<vx::ImageDataPixel> image;
  VolumeNode* dataSet;
  bool dataSetInit;

  // QRunnable interface
 public:
  /**
   * @brief run start the algorithm
   */
  void run() override;
 Q_SIGNALS:
  /**
   * @brief init signals the results
   * @param values results
   */
  void init(QVector<float> values);
};

}  // namespace vx
