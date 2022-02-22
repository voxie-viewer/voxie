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

#include "QVecLineEdit.hpp"

using namespace vx::visualization;

static QRegExp parseVector(
    R"reg((-?\d+(?:\.\d+)?(?:[eE][+-]?[0-9]+)?)\s*,\s*(-?\d+(?:\.\d+)?(?:[eE][+-]?[0-9]+)?)\s*,\s*(-?\d+(?:\.\d+)?(?:[eE][+-]?[0-9]+)?))reg");
static QRegExp parseQuaternion(
    R"reg((-?\d+(?:\.\d+)?(?:[eE][+-]?[0-9]+)?)\s*,\s*(-?\d+(?:\.\d+)?(?:[eE][+-]?[0-9]+)?)\s*,\s*(-?\d+(?:\.\d+)?(?:[eE][+-]?[0-9]+)?)\s*,\s*(-?\d+(?:\.\d+)?(?:[eE][+-]?[0-9]+)?))reg");

void QVecLineEdit::setVector(const QVector3D& vec) {
  this->setText(QVecLineEdit::toString(vec));
}

QVector3D QVecLineEdit::getVector(bool* isValid) {
  if (::parseVector.exactMatch(this->text()) == false) {
    if (isValid != nullptr) *isValid = false;
    return QVector3D();
  }
  if (isValid != nullptr) *isValid = true;
  qreal x, y, z;
  x = ::parseVector.cap(1).toFloat();
  y = ::parseVector.cap(2).toFloat();
  z = ::parseVector.cap(3).toFloat();

  return QVector3D(x, y, z);
}

void QVecLineEdit::setQuaternion(const QQuaternion& quat) {
  this->setText(QVecLineEdit::toString(quat));
}

QQuaternion QVecLineEdit::getQuaternion(bool* isValid) {
  if (::parseQuaternion.exactMatch(this->text()) == false) {
    if (isValid != nullptr) *isValid = false;
    return QQuaternion();
  }
  if (isValid != nullptr) *isValid = true;
  qreal x, y, z, w;
  w = ::parseQuaternion.cap(1).toFloat();
  x = ::parseQuaternion.cap(2).toFloat();
  y = ::parseQuaternion.cap(3).toFloat();
  z = ::parseQuaternion.cap(4).toFloat();

  return QQuaternion(w, x, y, z);
}
