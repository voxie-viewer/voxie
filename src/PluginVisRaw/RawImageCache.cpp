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

#include "RawImageCache.hpp"

#include <Voxie/Component/Plugin.hpp>

#include <VoxieClient/DBusUtil.hpp>

#include <VoxieBackend/Data/ImageDataPixelInst.hpp>
#include <VoxieBackend/Data/TomographyRawData2DAccessor.hpp>
#include <VoxieBackend/Data/TomographyRawData2DRegular.hpp>

#include <QtCore/QDebug>

using namespace vx;

QSharedPointer<RawImageCache> RawImageCache::create() {
  vx::checkOnMainThread("RawImageCache::create()");
  auto result = QSharedPointer<RawImageCache>(
      new RawImageCache(), [](QObject* obj) { obj->deleteLater(); });
  result->self_ = result;
  return result;
}

RawImageCache::RawImageCache() {}

QSharedPointer<vx::TomographyRawData2DAccessor> RawImageCache::data() {
  QMutexLocker locker(&this->mutex);
  return data_;
}
void RawImageCache::setData(
    const QSharedPointer<vx::TomographyRawData2DAccessor>& data) {
  {
    QMutexLocker locker(&this->mutex);

    if (data == data_) return;

    data_ = data;

    // Invalidate cache
    cachedImageId = -1;
    cachedImageMetadata = QJsonObject{};
    cachedVersion = QSharedPointer<vx::DataVersion>();
    cachedStream = "";
    cachedImageKind = QJsonObject{};
    cachedImage = QSharedPointer<vx::ImageDataPixel>();
    // Make sure a potentially running operation does not return a now incorrect
    // result
    backgroundLoadDiscard = true;
  }

  Q_EMIT this->imageLoaded();
}

QSharedPointer<vx::ImageDataPixel> RawImageCache::loadImageBlocking(
    const QSharedPointer<TomographyRawData2DAccessor>& data,
    const QString& stream, qint64 i, const QJsonObject& imageKind,
    bool allowIncompleteData) {
  qDebug() << "Loading raw image #" << i << "in stream" << stream;
  try {
    auto numberOfImages = data->numberOfImages(stream);
    if (i < 0 || i >= (qint64)numberOfImages) {
      qCritical() << "Attempting to load out of range raw image #" << i
                  << " (have " << numberOfImages << " images)";
      return ImageDataPixel::createInst(0, 0, 1, DataType::Float32, false);
    }
    VectorSizeT2 size = data->imageSize(stream, i);
    auto img1 =
        ImageDataPixel::createInst(size.x, size.y, 1, DataType::Float32, true);
    // auto img = qSharedPointerDynamicCast<ImageDataPixelInst<float, 1>>(img1);
    auto img = img1;  // TODO
    if (!img)
      throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                          "img == nullptr");
    data->readImage(stream, i, img, imageKind, VectorSizeT2(0, 0),
                    VectorSizeT2(0, 0), size, allowIncompleteData);
    return img;
  } catch (Exception& e) {
    qWarning() << "Failed to load raw image #" << i << e.what();
    return ImageDataPixel::createInst(0, 0, 1, DataType::Float32, false);
  }
}

// TODO: do this in background thread
void RawImageCache::setMainImage(const QString& stream, qint64 id,
                                 const QJsonObject& metadata,
                                 const QJsonObject& imageKind) {
  vx::checkOnMainThread("RawImageCache::setMainImage");

  auto self = self_.lock();
  if (!self) {
    qWarning() << "RawImageCache::setMainImage(): self is null";
    return;
  }

  auto data = this->data();

  if (!data) {
    qWarning() << "No TomographyRawData2DAccessor connected";
    return;
  }

  if (!data->hasStream(stream)) {
    qWarning() << "Invalid stream name" << stream << "used";
    return;
  }

  auto numberOfImages = data->numberOfImages(stream);
  if (id < 0 || (quint64)id >= numberOfImages) {
    qWarning() << "Attempting to load out of range raw image #" << id
               << " (have " << numberOfImages << " images)";
    return;
  }

  auto version = data->currentVersion();

  QSharedPointer<vx::ImageDataPixel> fromCache;
  {
    QMutexLocker locker1(&this->mutex);

    mainStream = stream;
    mainImageId = id;
    mainImageMetadata = metadata;
    mainImageKind = imageKind;

    if (cachedImage && version == cachedVersion && stream == cachedStream &&
        cachedImageId == id && cachedImageKind == imageKind) {
      fromCache = cachedImage;
      // Make sure that currently running operations won't override the result
      backgroundLoadDiscard = true;
      // Make sure cachedImageMetadata is up to date (might change even if image
      // id etc. don't change)
      cachedImageMetadata = metadata;
    } else {
      // Another background load is already running, wait until that is finished
      if (backgroundLoadRunning) return;

      vx::enqueueOnBackgroundThread([self]() {
        bool quit = false;
        while (!quit) {
          QSharedPointer<vx::TomographyRawData2DAccessor> data2;
          QString stream2;
          qint64 id2;
          QJsonObject metadata2;
          QJsonObject imageKind2;
          {
            QMutexLocker locker2(&self->mutex);
            data2 = self->data_;
            stream2 = self->mainStream;
            id2 = self->mainImageId;
            metadata2 = self->mainImageMetadata;
            imageKind2 = self->mainImageKind;
            self->backgroundLoadDiscard = false;
          }
          auto version2 = data2->currentVersion();

          auto img = loadImageBlocking(data2, stream2, id2, imageKind2,
                                       /*allowIncompleteData*/ true);

          bool ignore = false;
          {
            QMutexLocker locker2(&self->mutex);
            if (self->backgroundLoadDiscard) {
              // Ignore the current result, because data has changed or the
              // currently cached image has been returned
              ignore = true;
            } else {
              QObject::disconnect(self->dataReloadConnection);

              self->cachedImage = img;
              self->cachedStream = stream2;
              self->cachedVersion = version2;
              self->cachedImageId = id2;
              self->cachedImageMetadata = metadata2;
              self->cachedImageKind = imageKind2;

              if (img) {
                // TODO: This should not need to listen on
                // fakeTomographyRawData2DRegular() for changes
                self->dataReloadConnection = QObject::connect(
                    img->fakeTomographyRawData2DRegular().data(),
                    &TomographyRawData2DRegular::dataChanged, self.data(),
                    &RawImageCache::imageChanged);
              }
            }

            if (self->mainStream == self->cachedStream &&
                self->mainImageId == self->cachedImageId &&
                self->mainImageKind == self->cachedImageKind) {
              // Image is up to date, quit thread
              self->backgroundLoadRunning = false;
              quit = true;
              // Make sure cachedImageMetadata is up to date (might change even
              // if image id etc. don't change)
              self->cachedImageMetadata = self->mainImageMetadata;
            }
          }
          if (!ignore) Q_EMIT self->imageLoaded();
        }
      });
      backgroundLoadRunning = true;
    }
  }
  if (fromCache) Q_EMIT imageLoaded();
}

std::tuple<QSharedPointer<vx::DataVersion>, QString, qint64, QJsonObject,
           QJsonObject, QSharedPointer<vx::ImageDataPixel>>
RawImageCache::getMainImage() {
  {
    QMutexLocker locker(&this->mutex);
    if (cachedImage)
      return std::make_tuple(cachedVersion, cachedStream, cachedImageId,
                             cachedImageMetadata, cachedImageKind, cachedImage);
  }
  return std::make_tuple(QSharedPointer<vx::DataVersion>(), "", -1,
                         QJsonObject{}, QJsonObject{},
                         QSharedPointer<vx::ImageDataPixel>());
}

QSharedPointer<vx::ImageDataPixel> RawImageCache::getImageFromCache(
    const QSharedPointer<vx::TomographyRawData2DAccessor>& data,
    const QSharedPointer<vx::DataVersion>& version, const QString& stream,
    qint64 id, const QJsonObject& imageKind) {
  {
    QMutexLocker locker(&this->mutex);
    if (cachedImage && data == this->data_ && version == cachedVersion &&
        stream == cachedStream && cachedImageId == id &&
        cachedImageKind == imageKind) {
      auto image = cachedImage;
      // Check whether the image is complete
      // TODO: This should not need to check fakeTomographyRawData2DRegular()
      if (!image->fakeTomographyRawData2DRegular()
               ->currentVersion()
               ->metadata()
               .contains("Status"))
        return image;
    }
  }
  return QSharedPointer<vx::ImageDataPixel>();
}

QSharedPointer<vx::ImageDataPixel> RawImageCache::getImage(
    const QSharedPointer<vx::TomographyRawData2DAccessor>& data,
    const QSharedPointer<vx::DataVersion>& version, const QString& stream,
    qint64 id, const QJsonObject& imageKind) {
  auto image = getImageFromCache(data, version, stream, id, imageKind);

  if (!data)
    throw Exception("de.uni_stuttgart.Voxie.Error",
                    "No TomographyRawData2DAccessor connected");
  if (!image)
    image = loadImageBlocking(data, stream, id, imageKind,
                              /*allowIncompleteData*/ false);
  return image;
}
