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

#include <QtGui/QQuaternion>
#include <QtGui/QVector3D>

#include <QtWidgets/QLineEdit>

namespace vx {
namespace visualization {

/**
 * QLineEdit class specially designed for QVector3D and QQuaternion.
 * allows input of comma seperated vector components.
 */
class VOXIECORESHARED_EXPORT QVecLineEdit : public QLineEdit {
  Q_OBJECT
 public:
  explicit QVecLineEdit(QWidget* parent = 0) : QLineEdit(parent) {}

  void setVector(const QVector3D& vec);
  QVector3D getVector(bool* isValid = nullptr);

  void setQuaternion(const QQuaternion& quat);
  QQuaternion getQuaternion(bool* isValid = nullptr);

  static inline QString toString(const QVector3D& vector) {
    return QString::number(vector.x()) + ", " + QString::number(vector.y()) +
           ", " + QString::number(vector.z());
  }

  static inline QString toString(const QQuaternion& quat) {
    return QString::number(quat.scalar(), 'f') + ", " +
           QString::number(quat.x(), 'f') + ", " +
           QString::number(quat.y(), 'f') + ", " +
           QString::number(quat.z(), 'f');
  }
};

}  // namespace visualization
}  // namespace vx
