/*
 * Copyright (c) 2014-2023 The Voxie Authors
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

#include <VoxieClient/Vector.hpp>

#include <VoxieBackend/Data/SeriesData.hpp>

namespace vx {
class VOXIEBACKEND_EXPORT VolumeSeriesData : public SeriesData {
  Q_OBJECT
  VX_REFCOUNTEDOBJECT

  vx::Vector<double, 3> volumeOrigin_;
  vx::Vector<double, 3> volumeSize_;

 public:
  const vx::Vector<double, 3>& volumeOrigin() { return volumeOrigin_; }
  const vx::Vector<double, 3>& volumeSize() { return volumeSize_; }

  QList<QString> supportedDBusInterfaces() override;

 public:
  // TODO: should this be private so that only the create() method can call it?
  VolumeSeriesData(const QList<QSharedPointer<SeriesDimension>>& dimensions,
                   const vx::Vector<double, 3>& volumeOrigin,
                   const vx::Vector<double, 3>& volumeSize);

 public:
  ~VolumeSeriesData();

 protected:
  void checkNewData(const QSharedPointer<Data>& data) override;
};

}  // namespace vx
