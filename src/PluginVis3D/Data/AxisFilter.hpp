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

#include <QQuaternion>
#include <QVector3D>

/**
 * @brief The AxisFilter class is a small helper class to remove specific vector
 * elements to perform.
 * @author Robin Höchster
 */
class AxisFilter : public QObject {
  Q_OBJECT

  bool _filterX = false;
  bool _filterY = false;
  bool _filterZ = false;

 public:
  AxisFilter() {}

  /**
   * @brief filterX returns true if the x-filter is active.
   * @return
   */
  bool filterX() const { return _filterX; }

  /**
   * @brief filterY returns true if the y-filter is active.
   * @return
   */
  bool filterY() const { return _filterY; }

  /**
   * @brief filterZ returns true if the z-filter is active.
   * @return
   */
  bool filterZ() const { return _filterZ; }

  /**
   * @brief setFilterX Sets the filter for the x-value.
   * @param filter If true, the x-value will be removed from vectors.
   */
  void setFilterX(bool filter) {
    if (_filterX == filter) return;
    _filterX = filter;
    Q_EMIT changed();
  }

  /**
   * @brief setFilterY Sets the filter for the y-value.
   * @param filter If true, the y-value will be removed from vectors.
   */
  void setFilterY(bool filter) {
    if (_filterY == filter) return;
    _filterY = filter;
    Q_EMIT changed();
  }

  /**
   * @brief setFilterZ Sets the filter for the z-value.
   * @param filter If true, the z-value will be removed from vectors.
   */
  void setFilterZ(bool filter) {
    if (_filterZ == filter) return;
    _filterZ = filter;
    Q_EMIT changed();
  }

  /**
   * @brief filter sets all values of the 3D vector to 0.0 that were
   * previously selected to be filtered.
   * @param vector
   */
  void filter(QVector3D& vector) {
    if (_filterX) vector.setX(0.0f);
    if (_filterY) vector.setY(0.0f);
    if (_filterZ) vector.setZ(0.0f);
  }

  /**
   * @brief filter sets all values of the quaternion to 0.0 that were
   * previously selected to be filtered.
   * @param vector
   */
  void filter(QQuaternion& quaternion) {
    if (_filterX) quaternion.setX(0.0f);
    if (_filterY) quaternion.setY(0.0f);
    if (_filterZ) quaternion.setZ(0.0f);
  }

 Q_SIGNALS:
  /**
   * @brief changed is emitted if the filter has been changed.
   */
  void changed();
};
