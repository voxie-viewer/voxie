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

#include <Voxie/Voxie.hpp>

#include <Voxie/SpNav/SpaceNavEvent.hpp>

#include <QtCore/QObject>

namespace vx {
namespace spnav {

class VOXIECORESHARED_EXPORT SpaceNavClient : public QObject {
  Q_OBJECT

  friend class SpaceNavWidget;

  class Private {
   public:
    virtual ~Private();
    virtual bool isOk() const = 0;
  };

  class PrivateX11;
  Private* createPrivateX11();

  class PrivateUnix;
  Private* createPrivateUnix();

  QScopedPointer<Private> priv;

  SpaceNavClient();

 public:
  ~SpaceNavClient();

  static QSharedPointer<SpaceNavClient> getClient();

 Q_SIGNALS:
  // This events will always be emitted, regardless of which visualizer is
  // active
  void motionEvent(SpaceNavMotionEvent* event);
  void buttonPressEvent(SpaceNavButtonPressEvent* event);
  void buttonReleaseEvent(SpaceNavButtonReleaseEvent* event);
};

}  // namespace spnav
}  // namespace vx
