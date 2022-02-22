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

#include "InitializeColorizeWorker.hpp"

#include <Voxie/Data/Colorizer.hpp>

#include <QtCore/QDebug>

using namespace vx;

InitializeColorizeWorker::InitializeColorizeWorker() { dataSetInit = false; }

InitializeColorizeWorker::InitializeColorizeWorker(VolumeNode* dataSet)
    : dataSet(dataSet) {
  dataSetInit = true;
}

void InitializeColorizeWorker::run() {
  QPair<QPair<float, QRgb>, QPair<float, QRgb>> result;
  if (dataSetInit) {
    qDebug() << "initColotWorker if 1";
    result = Colorizer::initByAlgorithm(dataSet);
  } else {
    qDebug() << "initColotWorker if else";
    result = Colorizer::initByAlgorithm(image);
  }
  QVector<float> list = QVector<float>();
  list.append(result.first.first);
  list.append(result.second.first);
  Q_EMIT init(list);
}

void InitializeColorizeWorker::changeInitSet(
    const QSharedPointer<vx::ImageDataPixel>& newImage) {
  this->image = newImage;
}
