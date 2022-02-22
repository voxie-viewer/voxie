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

#include <QtCore/QtGlobal>

#include <QtGui/QVector3D>

#include <array>

namespace vx {
namespace spnav {

class VOXIECORESHARED_EXPORT SpaceNavEvent {
 public:
  SpaceNavEvent();
  virtual ~SpaceNavEvent();
};

class VOXIECORESHARED_EXPORT SpaceNavMotionEvent : public SpaceNavEvent {
  std::array<qint16, 6> data_;

 public:
  SpaceNavMotionEvent(const std::array<qint16, 6>& data);
  ~SpaceNavMotionEvent() override;

  const std::array<qint16, 6>& data() const { return data_; }

  QVector3D translation() const {
    return QVector3D(data()[0], data()[1], -data()[2]);
  }
  QVector3D rotation() const {
    return QVector3D(data()[3], data()[4], -data()[5]);
  }
};

class VOXIECORESHARED_EXPORT SpaceNavButtonEvent : public SpaceNavEvent {
  quint16 button_;

 public:
  SpaceNavButtonEvent(quint16 button);
  ~SpaceNavButtonEvent() override;

  quint16 button() const { return button_; }

  virtual bool pressed() const = 0;
};

class VOXIECORESHARED_EXPORT SpaceNavButtonPressEvent
    : public SpaceNavButtonEvent {
 public:
  SpaceNavButtonPressEvent(quint16 button);
  ~SpaceNavButtonPressEvent() override;

  bool pressed() const override { return true; }
};

class VOXIECORESHARED_EXPORT SpaceNavButtonReleaseEvent
    : public SpaceNavButtonEvent {
 public:
  SpaceNavButtonReleaseEvent(quint16 button);
  ~SpaceNavButtonReleaseEvent() override;

  bool pressed() const override { return false; }
};

}  // namespace spnav
}  // namespace vx
