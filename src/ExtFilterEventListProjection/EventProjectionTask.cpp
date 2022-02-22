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

#include "EventProjectionTask.hpp"

#include "EventProjectionProvider.hpp"
#include "EventProjectionWorker.hpp"
#include "SpecialImageProvider.hpp"

// TODO fix include path
#include "../ExtFilterEventListClustering/DBusNumericUtil.hpp"

#include <VoxieClient/QtUtil.hpp>
#include <VoxieClient/VoxieDBus.hpp>

#include <QtCore/QDebug>

using namespace vx;
using namespace vx::t3r;

constexpr Timestamp EventProjectionTask::TimestampBlockSize;
constexpr qulonglong EventProjectionTask::BlockSize;

EventProjectionTask::EventProjectionTask(EventProjectionProvider& provider)
    : provider(provider) {}

EventProjectionTask::~EventProjectionTask() {
  interrupted = 1;
  threadPool.waitForDone();
}

void EventProjectionTask::setStreamID(StreamID streamID) {
  this->streamID = streamID;
}

void EventProjectionTask::setOutputAccessor(
    QSharedPointer<de::uni_stuttgart::Voxie::TomographyRawData2DRegular> output,
    QSharedPointer<de::uni_stuttgart::Voxie::Data> outputData) {
  // Store output proxy as a non-refcounted pointer, allowing us to detect
  // when it is no longer needed by the consumer (e.g. raw visualizer switching
  // images or being deleted)
  this->outputProxy = output;
  this->outputProxyData = outputData;
}

void EventProjectionTask::setRegionSettings(ProjectionRegionSettings region) {
  this->region = region;
}

const ProjectionRegionSettings& EventProjectionTask::getRegionSettings() const {
  return region;
}

void EventProjectionTask::setWhiteImageProvider(
    QSharedPointer<SpecialImageProvider> whiteImageProvider) {
  this->whiteImageProvider = whiteImageProvider;
}

void EventProjectionTask::projectStream() {
  qlonglong minimumTimestamp = 0;
  qlonglong maximumTimestamp = 0;
  QMap<QString, QDBusVariant> metadata;
  HANDLEDBUSPENDINGREPLY(
      accessor().GetStreamInfo(streamID, vx::emptyOptions(), minimumTimestamp,
                               maximumTimestamp, attributes, metadata));

  auto boundingBox = getBoundingBox(metadata);

  const auto& settings = getProvider().getProjectionSettings();

  minimumTimestamp = std::max<Timestamp>(minimumTimestamp, settings.minTime);
  maximumTimestamp = std::min<Timestamp>(maximumTimestamp, settings.maxTime);

  // Increase work counter to avoid race conditions during worker enqueueing
  incrementWorkCounter();

  if (whiteImageProvider) {
    // Increase work counter to allow post-completion updates while white image
    // is still processing
    incrementWorkCounter();

    QObject::connect(whiteImageProvider.data(),
                     &SpecialImageProvider::imageChanged, this,
                     &EventProjectionTask::updateWhiteImage);

    QObject::connect(whiteImageProvider.data(),
                     &SpecialImageProvider::imageCompleted, this,
                     &EventProjectionTask::updateWhiteImage);

    if (!whiteImageComplete) {
      updateWhiteImage(whiteImageProvider->getImage());
    }
  }

  int priority = 0;

  // Generate list of timestamp ranges for exporting
  for (Timestamp ts = minimumTimestamp; ts <= maximumTimestamp;
       ts += TimestampBlockSize) {
    auto worker = QSharedPointer<EventProjectionWorker>::create(*this);
    worker->start = ts;
    worker->end =
        std::min<Timestamp>(ts + TimestampBlockSize - 1, maximumTimestamp);
    worker->blockSize = BlockSize;
    worker->streamID = streamID;
    worker->minEnergy = settings.minEnergy;
    worker->maxEnergy = settings.maxEnergy;

    // TODO save bounding box in task
    worker->boundingBox = boundingBox;

    incrementWorkCounter();
    threadPool.start(
        functionalRunnable([this, worker]() {
          try {
            if (!isInterrupted()) {
              acceptImage(worker->process());
              QMutexLocker locker(&mutex);
              updateImageData();
            }
          } catch (vx::Exception& e) {
            qCritical() << "Error projecting block:" << e.what();
            // TODO: What should be done exactly on an error?
            QMutexLocker locker(&mutex);
            interrupted = 1;
            if (!error) error = createQSharedPointer<vx::Exception>(e);
            if (outputProxyData) {
              // Signal error via update
              int overall = overallWorkCounter;
              double progress = 1.0 * (overall - workCounter) / overall;
              provider.finishUpdate(provider.createUpdate(outputProxyData),
                                    false, progress, error);
            }
          }
          decrementWorkCounter();
        }),
        priority--);
  }

  // Decrease work counter to compensate for incrementation before enqueueing
  decrementWorkCounter();
}

QSharedPointer<ProjectionImage> EventProjectionTask::getImage() const {
  QMutexLocker locker(&mutex);
  return image;
}

EventProjectionProvider& EventProjectionTask::getProvider() const {
  return provider;
}

void EventProjectionTask::setImage(QSharedPointer<ProjectionImage> image) {
  QMutexLocker locker(&mutex);
  this->image = image;
}

void EventProjectionTask::acceptImage(
    QSharedPointer<ProjectionImage> subImage) {
  QMutexLocker locker(&mutex);
  if (image != nullptr) {
    subImage->combine(*image);
  }
  image = subImage;
  Q_EMIT imageChanged(image);
}

bool EventProjectionTask::isInterrupted() const {
  if (interrupted) {
    return true;
  }

  if (!outputProxy) {
    // No output object set: cannot be interrupted
    return false;
  }

  // Read property to test for errors
  outputProxy->imageCount();

  if (outputProxy->lastError().isValid()) {
    if (outputProxy->lastError().type() == QDBusError::UnknownObject) {
      // UnknownObject: this likely means that the raw object was deallocated
      qDebug() << "Output object was destroyed, interrupting projection";
    } else {
      // Other error: probably a bug
      qWarning() << "DBus call failed during projection:"
                 << outputProxy->lastError().name();
    }
    interrupted = 1;
    return true;
  } else {
    return false;
  }
}

void EventProjectionTask::incrementWorkCounter() {
  ++overallWorkCounter;
  ++workCounter;
}

void EventProjectionTask::decrementWorkCounter() {
  if ((--workCounter) == 0) {
    QMutexLocker locker(&mutex);

    if (!error && outputProxyData) {
      // Signal completion via update
      provider.finishUpdate(provider.createUpdate(outputProxyData), true, 1.0);
    }

    // Deallocate projection task
    deleteLater();
  }
}

void EventProjectionTask::updateWhiteImage(
    QSharedPointer<ProjectionImage> whiteImage) {
  if (!whiteImageComplete) {
    QMutexLocker locker(&mutex);
    this->whiteImage = whiteImage;
    updateImageData();
  }

  if (whiteImageProvider->isComplete() &&
      whiteImageComplete.testAndSetOrdered(0, 1)) {
    decrementWorkCounter();
  }
}

void EventProjectionTask::updateImageData() {
  // Skip image data update if no output accessor exists, the image is not
  // available yet or an error occurred
  if (!outputProxy || !image || error) return;

  // Create update object for each write
  auto update = provider.createUpdate(outputProxyData);

  // Get shared memory for data buffer
  vx::Array3Info dataWritable = HANDLEDBUSPENDINGREPLY(
      outputProxy->GetDataWritable(update->path(), vx::emptyOptions()));
  auto array = QSharedPointer<vx::Array3<float>>::create(dataWritable);

  if (whiteImage) {
    // Correct for different exposure times of main and white images
    double timeCorrectionFactor =
        whiteImage->getExposureTime() / image->getExposureTime();

    // Write normalized image data relative to white image
    for (size_t y = 0; y < region.height; ++y) {
      for (size_t x = 0; x < region.width; ++x) {
        (*array)(region.outputX + x, region.outputY + y, region.outputImageID) =
            image->get(x, y) / whiteImage->get(x, y) * timeCorrectionFactor;
      }
    }
  } else {
    // Write image data
    for (size_t y = 0; y < region.height; ++y) {
      for (size_t x = 0; x < region.width; ++x) {
        (*array)(region.outputX + x, region.outputY + y, region.outputImageID) =
            image->get(x, y);
      }
    }
  }

  // Finish update and immediately discard version
  int overall = overallWorkCounter;
  double progress = 1.0 * (overall - workCounter) / overall;
  provider.finishUpdate(update, false, progress);
}

EventProjectionWorker::Rect EventProjectionTask::getBoundingBox(
    const QMap<QString, QDBusVariant>& metadata) const {
  EventProjectionWorker::Rect rect = {{0, 0, 255, 255}};
  auto variantList =
      dbusGetVariantValue<QList<QDBusVariant>>(metadata["BoundingBox"]);
  if ((size_t)variantList.size() == rect.size()) {
    for (size_t i = 0; i < rect.size(); ++i) {
      rect[i] = dbusGetNumber<double>(variantList[i]);
    }
  }
  return rect;
}

de::uni_stuttgart::Voxie::EventListDataAccessorOperations&
EventProjectionTask::accessor() const {
  return *provider.getInputAccessor();
}
