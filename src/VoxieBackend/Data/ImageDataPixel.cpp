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

// QDBusConnection should be included as early as possible:
// https://bugreports.qt.io/browse/QTBUG-48351 /
// https://bugreports.qt.io/browse/QTBUG-48377
#include <QtDBus/QDBusConnection>

#include "ImageDataPixel.hpp"

#include <VoxieBackend/Data/ImageDataPixelInst.hpp>

#include <VoxieClient/DBusAdaptors.hpp>

#include <QtCore/QCoreApplication>

// TODO: DBus interface: Should UpdateBuffer() be exported over DBus? Should
// CreateUpdate() make sure that the data is actually in the buffer (so that the
// update won't be overwritten by GPU data later)?

using namespace vx;

namespace vx {
class ImageDataPixelAdaptorImpl : public ImageDataPixelAdaptor {
  ImageDataPixel* object;

 public:
  ImageDataPixelAdaptorImpl(ImageDataPixel* object)
      : ImageDataPixelAdaptor(object), object(object) {}
  ~ImageDataPixelAdaptorImpl() override {}

  qulonglong componentCount() const override {
    return object->componentCount();
  }

  vx::TupleVector<quint64, 2> size() const override {
    return vx::TupleVector<quint64, 2>(object->width(), object->height());
  }

  std::tuple<QString, quint32, QString> dataType() const override {
    return getDataTypeStruct(object->dataType());
  }

  /* TODO: remove?
  double GetPixel(quint64 x, quint64 y) override {
    try {
      if (x >= width() || y >= height())
        throw vx::Exception("de.uni_stuttgart.Voxie.IndexOutOfRange",
                            "Index is out of range");
      return object->image().getPixel(x, y);
    } catch (vx::Exception& e) {
      e.handle(object);
      return 0;
    }
  }
  void SetPixel(quint64 x, quint64 y, double val) override {
    try {
      if (x >= width() || y >= height())
        throw vx::Exception("de.uni_stuttgart.Voxie.IndexOutOfRange",
                            "Index is out of range");
      object->image().setPixel(x, y, val);
    } catch (vx::Exception& e) {
      e.handle(object);
    }
  }
  */

  /*
  // ExtractSlice() already does this if UpdateBuffer is not set to false
  void UpdateBuffer(const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      auto cast = dynamic_cast<ImageDataPixelInst<float, 1>*>(object);
      if (!cast) return;  // Only needed for ImageDataPixelInst<float, 1>
      cast->image().switchMode(FloatImage::STDMEMORY_MODE);
    } catch (vx::Exception& e) {
      e.handle(object);
    }
  }
  */

  std::tuple<QMap<QString, QDBusVariant>, qint64,
             std::tuple<QString, quint32, QString>,
             std::tuple<quint64, quint64, quint64>,
             std::tuple<qint64, qint64, qint64>, QMap<QString, QDBusVariant>>
  GetDataReadonly(const QMap<QString, QDBusVariant>& options) override {
    try {
      vx::ExportedObject::checkOptions(options);

      return object->getData(false).toDBus();
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
      updateObj->validateCanUpdate(object);

      return object->getData(true).toDBus();
    } catch (vx::Exception& e) {
      e.handle(object);
      return vx::Array3Info().toDBus();
    }
  }
};
}  // namespace vx

ImageDataPixel::ImageDataPixel(DataType dataType, bool writableFromDBus)
    : Data(), dataType_(dataType), writableFromDBus_(writableFromDBus) {
  new ImageDataPixelAdaptorImpl(this);
  // qDebug() << "ImageDataPixel::ImageDataPixel" << this << writableFromDBus;
}
ImageDataPixel::~ImageDataPixel() {
  // qDebug() << "ImageDataPixel::~ImageDataPixel" << this;
}

template <typename T>
QSharedPointer<ImageDataPixel> ImageDataPixel::createInst2(
    size_t width, size_t height, size_t componentCount, DataType dataType,
    bool writableFromDBus) {
  if (componentCount == 1)
    return createBase<ImageDataPixelInst<T, 1>>(width, height, writableFromDBus,
                                                dataType);
  if (componentCount == 3)
    return createBase<ImageDataPixelInst<T, 3>>(width, height, writableFromDBus,
                                                dataType);
  if (componentCount == 4)
    return createBase<ImageDataPixelInst<T, 4>>(width, height, writableFromDBus,
                                                dataType);

  throw Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                  "Invalid component count");
}

QSharedPointer<ImageDataPixel> ImageDataPixel::createInst(
    size_t width, size_t height, size_t componentCount, DataType dataType,
    bool writableFromDBus) {
  switch (dataType) {
    case DataType::Float32:
      return createInst2<float>(width, height, componentCount, dataType,
                                writableFromDBus);
      /*
    case DataType::Float64:
      return createInst2<double>(width, height, componentCount,

                                dataType,writableFromDBus); case
    DataType::Int8: return createInst2<int8_t>(width, height,
    componentCount, writableFromDBus, dataType); case DataType::Int16:
    return createInst2<int16_t>(width, height, componentCount,
                                dataType,writableFromDBus); case
    DataType::Int32: return createInst2<int32_t>(width, height,
    componentCount,
                                dataType,writableFromDBus); case
    DataType::Int64: return createInst2<int64_t>(width, height,
    componentCount, writableFromDBus, dataType); case DataType::UInt8:
      */
      return createInst2<uint8_t>(width, height, componentCount, dataType,
                                  writableFromDBus);
    case DataType::UInt16:
      /*
      return createInst2<uint16_t>(width, height, componentCount,

                                dataType,writableFromDBus); case
    DataType::UInt32: return createInst2<uint32_t>(width, height,
    componentCount, writableFromDBus, dataType); case DataType::UInt64:
    return createInst2<uint64_t>(width, height, componentCount,
                                dataType,writableFromDBus);
      */
    default:
      throw Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                      "Invalid data type");
  }
}

QList<QString> ImageDataPixel::supportedDBusInterfaces() {
  return {
      "de.uni_stuttgart.Voxie.ImageDataPixel",
  };
}

template <typename T>
int roundAndClamp(const T& val) {
  T val2 = val + 0.5f;
  if (val2 <= 0) return 0;
  if (val2 >= 255) return 255;
  int vali = (int)val2;
  if (vali <= 0) return 0;
  if (vali >= 255) return 255;
  return vali;
}
QImage ImageDataPixel::convertToQImage() {
  QImage qimage(this->width(), this->height(), QImage::Format_ARGB32);
  // TODO: Handle integer types properly?
  this->performInGenericContextWithComponents<4>([&](const auto& img) {
    const auto& array = img->array();

    QRgb* qimgbuffer = (QRgb*)qimage.bits();

    auto width = array.template size<0>();
    auto height = array.template size<1>();
    for (size_t y = 0; y < height; y++) {
      size_t qimgY = height - y - 1;
      for (size_t x = 0; x < width; x++) {
        auto val = array(x, y);
        int r = roundAndClamp(std::get<0>(val) * 255);
        int g = roundAndClamp(std::get<1>(val) * 255);
        int b = roundAndClamp(std::get<2>(val) * 255);
        int a = roundAndClamp(std::get<3>(val) * 255);
        // a = 255; // TODO
        qimgbuffer[qimgY * width + x] = qRgba(r, g, b, a);
      }
    }
  });
  return qimage;
}

void ImageDataPixel::fromQImage(const QImage& image,
                                const vx::VectorSizeT2& outputRegionStart) {
  size_t width = image.width();
  size_t height = image.height();

  if (outputRegionStart.x > this->width() ||
      (outputRegionStart.x + width) > this->width() ||
      (outputRegionStart.x + width) < width ||
      outputRegionStart.y > this->height() ||
      (outputRegionStart.y + height) > this->height() ||
      (outputRegionStart.y + height) < height)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Region is outsize image");

  // TODO: Handle other QImage formats?
  if (image.format() != QImage::Format_ARGB32)
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "Unknown QImage format");

  // TODO: Handle integer types properly?
  this->performInGenericContextWithComponents<4>([&](const auto& img) {
    using ComponentType =
        typename std::decay<decltype(*img)>::type::ComponentType;
    using ElementType = typename std::decay<decltype(*img)>::type::ElementType;

    const auto& array = img->array();

    const QRgb* qimgbuffer = (const QRgb*)image.bits();

    for (size_t y = 0; y < height; y++) {
      size_t qimgY = height - y - 1;
      for (size_t x = 0; x < width; x++) {
        auto val = qimgbuffer[qimgY * width + x];
        ComponentType r = static_cast<ComponentType>(qRed(val)) /
                          static_cast<ComponentType>(255);
        ComponentType g = static_cast<ComponentType>(qGreen(val)) /
                          static_cast<ComponentType>(255);
        ComponentType b = static_cast<ComponentType>(qBlue(val)) /
                          static_cast<ComponentType>(255);
        ComponentType a = static_cast<ComponentType>(qAlpha(val)) /
                          static_cast<ComponentType>(255);
        array(outputRegionStart.x + x, outputRegionStart.y + y) =
            ElementType{r, g, b, a};
      }
    }
  });
}
