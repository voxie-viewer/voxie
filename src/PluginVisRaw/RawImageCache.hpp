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

#include <QtCore/QJsonObject>
#include <QtCore/QMetaObject>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>

namespace vx {
class TomographyRawData2DAccessor;
class ImageDataPixel;
class DataVersion;
}  // namespace vx

// TODO: Load images from background thread

// TODO: Implement proper cache of multiple images / prefetching

/**
 * @brief The RawImageCache contains raw images which were loaded and which
 * might be reused. (Currently the cache only contains one image.)
 */
class RawImageCache : public QObject {
  Q_OBJECT

  QSharedPointer<vx::TomographyRawData2DAccessor> data_;

  /*
  bool isLoading = false;
  qint64 loadingImage;
  bool isPending = false;
  qint64 pendingImage;
  */

  QMutex mutex;
  // Protected by mutex
  qint64 cachedImageId = -1;
  QJsonObject cachedImageMetadata;
  QJsonObject cachedImageKind;
  QSharedPointer<vx::DataVersion> cachedVersion;
  QString cachedStream;
  QSharedPointer<vx::ImageDataPixel> cachedImage;
  bool backgroundLoadRunning = false;
  // If backgroundLoadDiscard is true the background load operation will discard
  // its result
  bool backgroundLoadDiscard;
  // mainStream / mainImageId point to the image which should be loaded
  QString mainStream = "";
  qint64 mainImageId = -1;
  QJsonObject mainImageMetadata;
  QJsonObject mainImageKind;

  QMetaObject::Connection dataReloadConnection;

  static QSharedPointer<vx::ImageDataPixel> loadImageBlocking(
      const QSharedPointer<vx::TomographyRawData2DAccessor>& data,
      const QString& stream, qint64 i, const QJsonObject& imageKind,
      bool allowIncompleteData);

  RawImageCache();

  QWeakPointer<RawImageCache> self_;

 public:
  static QSharedPointer<RawImageCache> create();
  ~RawImageCache() {}

  QSharedPointer<vx::TomographyRawData2DAccessor> data();
  void setData(const QSharedPointer<vx::TomographyRawData2DAccessor>& data);

  /**
   * Tell the RawImageCache to attempt to load the image with ID id.
   *
   * Once the image has been loaded, the imageLoaded signal will be emitted.
   *
   * When there are further load requests afterwards, this load request might be
   * ignored.
   *
   * After this method is called, there will always be at least one call
   * emission of the imageLoaded signal (though multiple calls might trigger
   * only one emission).
   */
  // TODO: Should metadata be tracked in RawImageCache?
  void setMainImage(const QString& stream, qint64 id,
                    const QJsonObject& metadata, const QJsonObject& imageKind);

  /**
   * Get the main image from the cache, or a nullptr if the main image is not
   * set or has not yet been loaded.
   */
  std::tuple<QSharedPointer<vx::DataVersion>, QString, qint64, QJsonObject,
             QJsonObject, QSharedPointer<vx::ImageDataPixel>>
  getMainImage();

  /**
   * Attempt to get the image with ID id from the cache. If the image is not in
   * the cache, return a nullptr.
   */
  QSharedPointer<vx::ImageDataPixel> getImageFromCache(
      const QSharedPointer<vx::TomographyRawData2DAccessor>& data,
      const QSharedPointer<vx::DataVersion>& version, const QString& stream,
      qint64 id, const QJsonObject& imageKind);

  /**
   * Get the image, either from cache or by loading it.
   */
  QSharedPointer<vx::ImageDataPixel> getImage(
      const QSharedPointer<vx::TomographyRawData2DAccessor>& data,
      const QSharedPointer<vx::DataVersion>& version, const QString& stream,
      qint64 id, const QJsonObject& imageKind);

 Q_SIGNALS:
  /**
   * @brief imageLoaded signals that an image has been loaded
   */
  void imageLoaded();

  /**
   * @brief imageChanged signals that the main image's data has been changed
   */
  void imageChanged();
};
