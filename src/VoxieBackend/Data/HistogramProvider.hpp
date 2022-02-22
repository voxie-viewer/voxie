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

#include "FloatImage.hpp"

#include <VoxieBackend/VoxieBackend.hpp>

#include <QMutex>
#include <QObject>
#include <QSharedPointer>
#include <QVector>

#include <cmath>

namespace vx {

/**
 * Holds a histogram, consisting of minimum and maximum bounds for the input
 * values, and a set of evenly distributed buckets.
 *
 * This object may be safely shared across threads.
 */
class VOXIEBACKEND_EXPORT HistogramProvider : public QObject {
  Q_OBJECT
 public:
  static constexpr unsigned int DefaultBucketCount = 100;

  struct VOXIEBACKEND_EXPORT Data {
    QVector<quint64> buckets;
    /**
     * @brief xAxisLog If true values on the x-axis are scaled logarithmically
     */
    bool xAxisLog = false;
    double minimumValue = 0.0;
    double maximumValue = 1.0;

    /**
     * @brief maximumCount The number of values the bucket with the most values
     * has
     */
    quint64 maximumCount = 0;

    double findPercentile(double percentile) const;
    /**
     * @brief bucketIndexOfValue Calculates the index of the bucket that the
     * passed value would be put into if it were in the histogram.
     * @param value A double representing the value.
     * @return Returns a qint64 representing the bucket index that the passed
     * value would be in. Might be larger than the bucketCount - 1 or smaller
     * than 0.
     */
    qint64 bucketIndexOfValue(const double value) const;
  };

  using DataPtr = QSharedPointer<Data>;

  explicit HistogramProvider(QObject* parent = nullptr);
  virtual ~HistogramProvider() = default;

  template <typename Iterator, typename Func>
  static DataPtr generateData(double minValue, double maxValue,
                              unsigned int bucketCount, Iterator begin,
                              Iterator end, Func mapper,
                              bool xAxisLog = false) {
    DataPtr data = DataPtr::create();
    data->minimumValue = xAxisLog ? logf(minValue) : minValue;
    data->maximumValue = xAxisLog ? logf(maxValue) : maxValue;

    data->buckets.resize(bucketCount);

    // the factor is the inverse of the size (size as in the difference between
    // the smallest and largest possible item in a bucket) of a single bucket
    double factor = (bucketCount - 1) / (maxValue - minValue);

    // loop through all elements we want to put into the buckets
    for (auto it = begin; it != end; ++it) {
      auto value = mapper(*it);

      // skip non-positive values in log mode (only positive numbers are defined
      // for the logarithm)
      if (xAxisLog) {
        if (value <= 0.0f) continue;

        value = logf(value);
      }

      // calculate the index of the bucket that the item should be put into by
      // firstly calculating the value of the item using the mapper function,
      // then getting the value of the item relative to the minValue and then
      // using the factor to calculate bucket index
      qint64 index = (value - minValue) * factor;

      // if the index is within the bounds increment the item count for the
      // specified bucket and also calculate the maximum count by checking if
      // the bucket is the bucket with the most values after incrementing and
      // setting its value count as the new maximumCount if it is
      if (index >= 0 && index < bucketCount) {
        data->maximumCount =
            std::max(data->maximumCount, ++data->buckets[index]);
      }
    }

    return data;
  }

  template <typename Container, typename Func>
  static DataPtr generateData(double minValue, double maxValue,
                              unsigned int bucketCount, Container container,
                              Func mapper) {
    return generateData(minValue, maxValue, bucketCount, container.begin(),
                        container.end(), mapper);
  }

  template <typename Iter, typename Func>
  static std::pair<double, double> getMinMax(Iter begin, Iter end,
                                             Func mapper) {
    double minValue = begin == end ? 0.0 : double(mapper(*begin));
    double maxValue = minValue;

    for (auto it = begin; it != end; ++it) {
      const auto& value = *it;
      minValue = std::min<double>(minValue, mapper(value));
      maxValue = std::max<double>(maxValue, mapper(value));
    }

    return std::make_pair(minValue, maxValue);
  }

  template <typename Container, typename Func>
  void setDataFromContainer(unsigned int bucketCount, Container container,
                            Func mapper) {
    auto minMax = getMinMax(container.begin(), container.end(), mapper);
    setData(generateData(minMax.first, minMax.second, bucketCount, container,
                         mapper));
  }

  void setDataFromFloatImage(FloatImage& img, unsigned int bucketCount) {
    FloatBuffer buffer = img.getBufferCopy();
    float minValue = 0.0f;
    if (buffer.length() > 0) {
      minValue = std::numeric_limits<float>::max();
      for (size_t i = 0; i < buffer.length(); i++) {
        if (buffer[i] < minValue && !std::isnan(buffer[i])) {
          minValue = buffer[i];
        }
      }
    }
    float maxValue = 0.0f;
    if (buffer.length() > 0) {
      maxValue = std::numeric_limits<float>::min();
      for (size_t i = 0; i < buffer.length(); i++) {
        if (buffer[i] > maxValue && !std::isnan(buffer[i])) {
          maxValue = buffer[i];
        }
      }
    }
    setDataFromFloatImage(img, bucketCount, minValue, maxValue);
  }

  void setDataFromFloatImage(FloatImage& img, unsigned int bucketCount,
                             double minValue, double maxValue);

  /**
   * @brief setData Used to assign data to this HistogramProvider.
   * @param data Pointer to the data to assign.
   */
  void setData(DataPtr data);
  /**
   * @brief getData Returns a pointer to the data that is assigned to this
   * HistogramProvider. Note that when manually modifying the data at the
   * pointer returned by this method, HistogramProvider::dataChanged() should be
   * called afterwards.
   * @return Returns a pointer to the data of this HistogramProvider.
   */
  DataPtr getData() const;

 Q_SIGNALS:
  void dataChanged(vx::HistogramProvider::DataPtr data);

 private:
  mutable QMutex mutex;

  DataPtr data;
};

}  // namespace vx

Q_DECLARE_METATYPE(vx::HistogramProvider::DataPtr)
