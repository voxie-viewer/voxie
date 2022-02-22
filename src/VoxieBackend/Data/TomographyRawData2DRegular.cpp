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

#include "TomographyRawData2DRegular.hpp"

#include <VoxieBackend/Data/TomographyRawData2DRegularShmemInst.hpp>

#include <VoxieClient/DBusAdaptors.hpp>

#include <VoxieClient/ObjectExport/DBusCallUtil.hpp>

using namespace vx;

namespace {
class TomographyRawData2DRegularAdaptorImpl
    : public TomographyRawData2DRegularAdaptor {
  TomographyRawData2DRegular* object;

 public:
  TomographyRawData2DRegularAdaptorImpl(TomographyRawData2DRegular* object)
      : TomographyRawData2DRegularAdaptor(object), object(object) {}
  ~TomographyRawData2DRegularAdaptorImpl() override {}

  std::tuple<QString, quint32, QString> dataType() const override {
    try {
      return getDataTypeStruct(object->dataType());
    } catch (vx::Exception& e) {
      e.handle(object);
      return vx::dbusDefaultReturnValue();
    }
  }

  vx::TupleVector<double, 2> gridSpacing() const override {
    try {
      return object->gridSpacing();
    } catch (vx::Exception& e) {
      e.handle(object);
      return vx::TupleVector<double, 2>(0, 0);
    }
  }

  qulonglong imageCount() const override {
    try {
      return object->imageCount();
    } catch (vx::Exception& e) {
      e.handle(object);
      return 0;
    }
  }

  vx::TupleVector<double, 2> imageOrigin() const override {
    try {
      return object->imageOrigin();
    } catch (vx::Exception& e) {
      e.handle(object);
      return vx::TupleVector<double, 2>(0, 0);
    }
  }

  vx::TupleVector<quint64, 2> imageShape() const override {
    try {
      return object->imageShape();
    } catch (vx::Exception& e) {
      e.handle(object);
      return std::make_tuple(0, 0);
    }
  }

  std::tuple<QMap<QString, QDBusVariant>, qint64,
             std::tuple<QString, quint32, QString>,
             std::tuple<quint64, quint64, quint64>,
             std::tuple<qint64, qint64, qint64>, QMap<QString, QDBusVariant>>
  GetDataReadonly(const QMap<QString, QDBusVariant>& options) override {
    try {
      vx::ExportedObject::checkOptions(options);

      return object->getDataReadonly().toDBus();
    } catch (vx::Exception& e) {
      e.handle(object);
      return vx::Array3Info().toDBus();
    }
  }

  std::tuple<QMap<QString, QDBusVariant>, qint64,
             std::tuple<QString, quint32, QString>,
             std::tuple<quint64, quint64, quint64>,
             std::tuple<qint64, qint64, qint64>, QMap<QString, QDBusVariant>>
  GetDataWritable(const QDBusObjectPath& update,
                  const QMap<QString, QDBusVariant>& options) override {
    try {
      vx::ExportedObject::checkOptions(options);

      auto updateObj = vx::DataUpdate::lookup(update);

      return object->getDataWritable(updateObj).toDBus();
    } catch (vx::Exception& e) {
      e.handle(object);
      return vx::Array3Info().toDBus();
    }
  }

  void Save(const QString& fileName,
            const QMap<QString, QDBusVariant>& options) override {
    try {
      vx::ExportedObject::checkOptions(options);

      throw vx::Exception(
          "de.uni_stuttgart.Voxie.NotSupported",
          "TODO: TomographyRawData2DRegular.Save not implemented");
      (void)fileName;
    } catch (vx::Exception& e) {
      e.handle(object);
      return;
    }
  }
};
}  // namespace

TomographyRawData2DRegular::TomographyRawData2DRegular(DataType dataType)
    : dataType_(dataType) {
  // TODO: interfaces de.uni_stuttgart.Voxie.TomographyRawDataBase and
  // de.uni_stuttgart.Voxie.TomographyRawData (currently both empty)
  new TomographyRawData2DRegularAdaptorImpl(this);
}
TomographyRawData2DRegular::~TomographyRawData2DRegular() {}

QList<QString> TomographyRawData2DRegular::supportedDBusInterfaces() {
  return {
      "de.uni_stuttgart.Voxie.TomographyRawData2DRegular",
  };
}

QSharedPointer<TomographyRawData2DRegular>
TomographyRawData2DRegular::createInst(size_t width, size_t height,
                                       size_t numberOfImages,
                                       DataType dataType) {
  switch (dataType) {
    case DataType::Float32:
      return createBase<TomographyRawData2DRegularShmemInst<float>>(
          width, height, numberOfImages, dataType);
      /*
    case DataType::Float64:
      return createBase<TomographyRawData2DRegularShmemInst<double>>(width,
    height, numberOfImages,

                                dataType); case
    DataType::Int8: return
    createBase<TomographyRawData2DRegularShmemInst<int8_t>>(width, height,
    numberOfImages, , dataType); case DataType::Int16:
    return createBase<TomographyRawData2DRegularShmemInst<int16_t>>(width,
    height, numberOfImages, dataType); case DataType::Int32: return
    createBase<TomographyRawData2DRegularShmemInst<int32_t>>(width, height,
    numberOfImages,
                                dataType); case
    DataType::Int64: return
    createBase<TomographyRawData2DRegularShmemInst<int64_t>>(width, height,
    numberOfImages, , dataType); case DataType::UInt8:
      */
      return createBase<TomographyRawData2DRegularShmemInst<uint8_t>>(
          width, height, numberOfImages, dataType);
    case DataType::UInt16:
      /*
      return createBase<TomographyRawData2DRegularShmemInst<uint16_t>>(width,
    height, numberOfImages,

                                dataType); case
    DataType::UInt32: return
    createBase<TomographyRawData2DRegularShmemInst<uint32_t>>(width, height,
    numberOfImages, , dataType); case DataType::UInt64:
    return createBase<TomographyRawData2DRegularShmemInst<uint64_t>>(width,
    height, numberOfImages, dataType);
      */
    default:
      throw Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                      "Invalid data type");
  }
}
