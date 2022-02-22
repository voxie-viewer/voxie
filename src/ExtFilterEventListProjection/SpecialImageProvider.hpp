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

#include "EventProjectionCommon.hpp"
#include "ProjectionImage.hpp"

#include <QAtomicInt>
#include <QMutex>
#include <QObject>
#include <QSharedPointer>

#include <memory>

Q_DECLARE_METATYPE(QSharedPointer<vx::t3r::ProjectionImage>)

namespace vx {
namespace t3r {

class EventProjectionProvider;
class ProjectionImage;

class SpecialImageProvider : public QObject {
  Q_OBJECT
 public:
  SpecialImageProvider(EventProjectionProvider& provider, std::size_t width,
                       std::size_t height);

  void initialize(QString tagName);

  bool isValid() const;
  bool isComplete() const;

  QSharedPointer<ProjectionImage> getImage() const;

 Q_SIGNALS:
  void imageChanged(QSharedPointer<vx::t3r::ProjectionImage> image);
  void imageCompleted(QSharedPointer<vx::t3r::ProjectionImage> image);

 private:
  void updateImage(QSharedPointer<ProjectionImage> image);

  std::vector<StreamID> getStreamIDsWithTag(QString tagName) const;

  QAtomicInt initialized;
  QAtomicInt valid;
  QAtomicInt complete;

  QSharedPointer<ProjectionImage> image;

  EventProjectionProvider& provider;

  mutable QMutex mutex;
};

}  // namespace t3r
}  // namespace vx
