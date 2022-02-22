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

#include "EventProjectionCommon.hpp"
#include "EventProjectionWorker.hpp"
#include "ProjectionImage.hpp"
#include "ProjectionSettings.hpp"

#include <VoxieClient/Array.hpp>
#include <VoxieClient/DBusClient.hpp>
#include <VoxieClient/DBusProxies.hpp>
#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <QPair>
#include <QSharedPointer>
#include <QThreadStorage>

#include <array>

namespace vx {
namespace t3r {

class EventProjectionProvider;
class SpecialImageProvider;

class EventProjectionTask : public QObject {
  Q_OBJECT
 public:
  EventProjectionTask(EventProjectionProvider& provider);
  virtual ~EventProjectionTask();

  void setStreamID(StreamID streamID);
  void setOutputAccessor(
      QSharedPointer<de::uni_stuttgart::Voxie::TomographyRawData2DRegular>
          output,
      QSharedPointer<de::uni_stuttgart::Voxie::Data> outputData);
  void setRegionSettings(ProjectionRegionSettings region);
  const ProjectionRegionSettings& getRegionSettings() const;

  void setWhiteImageProvider(
      QSharedPointer<SpecialImageProvider> whiteImageProvider);

  void projectStream();

  float getProgress() const;

  QSharedPointer<ProjectionImage> getImage() const;

  EventProjectionProvider& getProvider() const;

 Q_SIGNALS:
  void imageChanged(QSharedPointer<vx::t3r::ProjectionImage> image);

 private:
  // TODO make values configurable or auto-detect at runtime
  static constexpr Timestamp TimestampBlockSize = 1e7;
  static constexpr qulonglong BlockSize = 1024 * 1024;

  using AttributeInfo =
      std::tuple<QString, std::tuple<QString, quint32, QString>, QString,
                 QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>;

  void setImage(QSharedPointer<ProjectionImage> image);
  void acceptImage(QSharedPointer<ProjectionImage> subImage);

  bool isInterrupted() const;

  void incrementWorkCounter();
  void decrementWorkCounter();

  void updateWhiteImage(QSharedPointer<ProjectionImage> whiteImage);
  void updateImageData();

  EventProjectionWorker::Rect getBoundingBox(
      const QMap<QString, QDBusVariant>& metadata) const;

  de::uni_stuttgart::Voxie::EventListDataAccessorOperations& accessor() const;

  EventProjectionProvider& provider;
  StreamID streamID = 0;
  QSharedPointer<ProjectionImage> image;
  QList<AttributeInfo> attributes;
  ProjectionRegionSettings region;

  QSharedPointer<SpecialImageProvider> whiteImageProvider;
  QSharedPointer<ProjectionImage> whiteImage;
  QAtomicInt whiteImageComplete;

  QSharedPointer<de::uni_stuttgart::Voxie::TomographyRawData2DRegular>
      outputProxy;
  QSharedPointer<de::uni_stuttgart::Voxie::Data> outputProxyData;

  QSharedPointer<vx::Exception> error;

  QThreadPool threadPool;
  mutable QMutex mutex;
  mutable QAtomicInt interrupted;
  mutable QAtomicInt workCounter;
  mutable QAtomicInt overallWorkCounter;

  friend class EventProjectionWorker;
};

}  // namespace t3r
}  // namespace vx
