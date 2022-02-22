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

#include "SpecialImageProvider.hpp"
#include "EventProjectionProvider.hpp"
#include "EventProjectionTask.hpp"
#include "ProjectionImage.hpp"

#include <VoxieClient/VoxieDBus.hpp>

using namespace vx;
using namespace vx::t3r;

namespace vx {
namespace t3r {

SpecialImageProvider::SpecialImageProvider(EventProjectionProvider& provider,
                                           std::size_t width,
                                           std::size_t height)
    : provider(provider) {
  qRegisterMetaType<QSharedPointer<vx::t3r::ProjectionImage>>();
  image = decltype(image)::create(width, height);
}

void SpecialImageProvider::initialize(QString tagName) {
  // Use double checked locking for improved performance
  if (initialized) return;
  QMutexLocker lock(&mutex);
  if (initialized) return;

  auto streamIDs = getStreamIDsWithTag(tagName);

  complete = false;

  if (streamIDs.empty()) {
    initialized = true;
    valid = false;
    return;
  }

  // TODO what to do in case of multiple streams with a matching tag?
  auto streamID = streamIDs.front();

  auto task = vx::executeOnMainThread([&]() {
    // The task will delete itself (using deleteLater) once it is complete
    return new EventProjectionTask(provider);
  });

  // Forward image change signals
  QObject::connect(task, &EventProjectionTask::imageChanged, this,
                   [this](QSharedPointer<ProjectionImage> image) {
                     updateImage(image);
                     Q_EMIT imageChanged(image);
                   });

  // Forward completion signal
  QObject::connect(task, &EventProjectionTask::destroyed, this, [this]() {
    complete = true;
    Q_EMIT imageCompleted(getImage());
  });

  ProjectionRegionSettings region;
  region.width = image->getWidth();
  region.height = image->getHeight();

  // TODO just use projected images directly, without a raw data tomography
  // object
  task->setStreamID(streamID);
  task->setRegionSettings(region);
  task->projectStream();

  initialized = true;
  valid = true;
}

bool SpecialImageProvider::isValid() const { return valid; }

bool SpecialImageProvider::isComplete() const { return complete; }

QSharedPointer<ProjectionImage> SpecialImageProvider::getImage() const {
  QMutexLocker lock(&mutex);
  return image;
}

void SpecialImageProvider::updateImage(QSharedPointer<ProjectionImage> image) {
  QMutexLocker lock(&mutex);
  this->image = image;
}

std::vector<StreamID> SpecialImageProvider::getStreamIDsWithTag(
    QString tagName) const {
  std::vector<StreamID> result;

  for (StreamID id = 0; id < provider.getStreamCount(); ++id) {
    if (provider.getStreamMetadata(id).contains(tagName)) {
      result.push_back(id);
    }
  }

  return result;
}

}  // namespace t3r
}  // namespace vx
