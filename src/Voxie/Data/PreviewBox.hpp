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

#include <VoxieClient/DBusTypeList.hpp>

#include <QtCore/QObject>

#include <QtGui/QVector3D>

namespace vx {

/**
 * @brief The PreviewBox class is the data representation of a previewbox
 * @see PreviewBoxNode
 */
class PreviewBox {
  QVector3D size_;
  QVector3D origin_;
  bool active_;

 public:
  PreviewBox();
  PreviewBox(QVector3D size, QVector3D origin, bool active = true);

  QVector3D size() { return this->size_; }
  QVector3D origin() { return this->origin_; }
  bool active() { return this->active_; }
  void setActive(bool active) { this->active_ = active; }
  void setOrigin(QVector3D origin) { this->origin_ = origin; }
  void setSize(QVector3D size) { this->size_ = size; }
};

}  // namespace vx
