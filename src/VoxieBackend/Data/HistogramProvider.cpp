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

#include "HistogramProvider.hpp"

#include <QVector>
#include <VoxieClient/QtUtil.hpp>

using namespace vx;

HistogramProvider::HistogramProvider(QObject* parent)
    : QObject(parent), data(DataPtr::create()) {
  qRegisterMetaType<vx::HistogramProvider::DataPtr>();
}

void HistogramProvider::setDataFromFloatImage(FloatImage& img,
                                              unsigned int bucketCount,
                                              double minValue,
                                              double maxValue) {
  if (thread() != QThread::currentThread())
    qCritical() << "Histogram::calculateHistogram() called from wrong thread"
                << QThread::currentThread() << "(belongs to" << thread() << ")";

  FloatBuffer buffer = img.getBufferCopy();

  if (buffer.numElements() > 0) {
    setData(generateData(minValue, maxValue, bucketCount, &buffer[0],
                         &buffer[buffer.size() - 1],
                         [&](float v) { return v; }));
  }
}

void HistogramProvider::setData(DataPtr data) {
  {
    QMutexLocker locker(&mutex);
    this->data = data;
  }

  Q_EMIT dataChanged(data);
}

HistogramProvider::DataPtr HistogramProvider::getData() const {
  QMutexLocker locker(&mutex);
  return data;
}

qint64 HistogramProvider::Data::bucketIndexOfValue(const double value) const {
  // the factor is the inverse of the size (size as in the difference between
  // the smallest and largest possible item in a bucket) of a single bucket
  double factor = (buckets.size()) / (maximumValue - minimumValue);

  return (value - minimumValue) * factor;
}

double HistogramProvider::Data::findPercentile(double percentile) const {
  if (buckets.empty()) {
    return 0.0;
  }

  quint64 sum = std::accumulate(buckets.begin(), buckets.end(), quint64(0));
  quint64 threshold = percentile * sum;
  quint64 runningSum = 0;
  for (auto it = buckets.begin(); it != buckets.end(); ++it) {
    runningSum += *it;
    if (runningSum >= threshold) {
      int index = std::distance(buckets.begin(), it);
      double excess = (runningSum - threshold) / double(*it);
      double ratio = (index + (1.0 - excess)) / buckets.size();
      return minimumValue + ratio * (maximumValue - minimumValue);
    }
  }

  return maximumValue;
}
