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

#include "VolumeData.hpp"

#include <VoxieClient/DBusAdaptors.hpp>

#include <VoxieBackend/Data/VolumeStructure.hpp>

#include <VoxieBackend/DBus/DBusTypes.hpp>

using namespace vx;

namespace vx {
namespace internal {
class VolumeDataAdaptorImpl : public VolumeDataAdaptor {
  Q_OBJECT

  VolumeData* object;

 public:
  VolumeDataAdaptorImpl(VolumeData* object)
      : VolumeDataAdaptor(object), object(object) {}
  virtual ~VolumeDataAdaptorImpl() {}

  QDBusObjectPath volumeStructure() const override {
    try {
      return ExportedObject::getPath(object->volumeStructure());
    } catch (vx::Exception& e) {
      e.handle(object);
      return ExportedObject::getPath(nullptr);
    }
  }

  std::tuple<QString, quint32, QString> dataType() const override {
    try {
      return getDataTypeStruct(object->getDataType());
    } catch (vx::Exception& e) {
      e.handle(object);
      return std::make_tuple("", 0, "");
    }
  }

  vx::TupleVector<double, 3> volumeOrigin() const override {
    try {
      return toTupleVector(object->origin());
    } catch (vx::Exception& e) {
      e.handle(object);
      return vx::TupleVector<double, 3>(0, 0, 0);
    }
  }

  vx::TupleVector<double, 3> volumeSize() const override {
    try {
      return toTupleVector(object->volumeSize());
    } catch (vx::Exception& e) {
      e.handle(object);
      return vx::TupleVector<double, 3>(0, 0, 0);
    }
  }

  void Save(const QString& fileName,
            const QMap<QString, QDBusVariant>& options) override {
    try {
      vx::ExportedObject::checkOptions(options);
      Q_UNUSED(fileName);
      throw Exception("de.uni_stuttgart.Voxie.NotImplemented",
                      "VolumeDataAdaptor.Save() not implemented");  // TODO
    } catch (vx::Exception& e) {
      e.handle(object);
    }
  }
};
}  // namespace internal
}  // namespace vx

using namespace vx::internal;

VolumeData::VolumeData(const QSharedPointer<VolumeStructure>& volumeStructure)
    : Data(), volumeStructure_(volumeStructure) {
  new VolumeDataAdaptorImpl(this);
  connect(this, &Data::dataChanged, this,
          [this](const QSharedPointer<DataVersion>& newVersion,
                 DataChangedReason reason) {
            Q_UNUSED(newVersion);
            if (reason != DataChangedReason::NewUpdate) Q_EMIT this->changed();
          });
}

VolumeData::~VolumeData() {}

#include "VolumeData.moc"
