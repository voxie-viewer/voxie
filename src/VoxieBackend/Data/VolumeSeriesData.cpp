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

#include "VolumeSeriesData.hpp"

#include <VoxieBackend/Data/VolumeData.hpp>

#include <VoxieBackend/DBus/DBusTypes.hpp>

#include <VoxieClient/DBusAdaptors.hpp>

namespace vx {
namespace {
class VolumeSeriesDataAdaptorImpl : public VolumeSeriesDataAdaptor {
  VolumeSeriesData* object;

 public:
  VolumeSeriesDataAdaptorImpl(VolumeSeriesData* object)
      : VolumeSeriesDataAdaptor(object), object(object) {}
  ~VolumeSeriesDataAdaptorImpl() override {}

  vx::TupleVector<double, 3> volumeOrigin() const override {
    try {
      return toTupleVector(object->volumeOrigin());
    } catch (Exception& e) {
      return e.handle(object);
    }
  }

  vx::TupleVector<double, 3> volumeSize() const override {
    try {
      return toTupleVector(object->volumeSize());
    } catch (Exception& e) {
      return e.handle(object);
    }
  }
};
}  // namespace
}  // namespace vx

vx::VolumeSeriesData::VolumeSeriesData(
    const QList<QSharedPointer<SeriesDimension>>& dimensions,
    const vx::Vector<double, 3>& volumeOrigin,
    const vx::Vector<double, 3>& volumeSize)
    : SeriesData(dimensions),
      volumeOrigin_(volumeOrigin),
      volumeSize_(volumeSize) {
  new VolumeSeriesDataAdaptorImpl(this);
}

vx::VolumeSeriesData::~VolumeSeriesData() {}

// Code to check that all elements are consistent, e.g. same origin and size
// (and all are volumes)
void vx::VolumeSeriesData::checkNewData(const QSharedPointer<Data>& data) {
  auto volumeData = qSharedPointerDynamicCast<VolumeData>(data);
  if (!volumeData)
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InvalidArgument",
        "Attempting to add non-volume data to VolumeSeriesData");

  if (volumeData->volumeOrigin() != this->volumeOrigin())
    throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "Attempting to add volume data with different volume "
                        "origin to VolumeSeriesData");

  if (volumeData->volumeSize() != this->volumeSize())
    throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "Attempting to add volume data with different volume "
                        "size to VolumeSeriesData");
}

QList<QString> vx::VolumeSeriesData::supportedDBusInterfaces() {
  return {
      "de.uni_stuttgart.Voxie.VolumeSeriesData",
      "de.uni_stuttgart.Voxie.SeriesData",
  };
}
