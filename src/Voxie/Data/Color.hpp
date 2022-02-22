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

#include <QtGui/QColor>
#include <QtGui/QVector4D>

namespace vx {
class VOXIECORESHARED_EXPORT Color {
  vx::TupleVector<double, 4> col;

 public:
  Color() : col(std::make_tuple(0, 0, 0, 1)) {}
  Color(double r, double g, double b, double a = 1.0) : col(r, g, b, a) {}
  Color(const vx::TupleVector<double, 4>& tuple) : col(tuple) {}
  Color(const QColor color)
      : col(color.redF(), color.greenF(), color.blueF(), color.alphaF()) {}
  Color(const QVector4D color)
      : col(color.x(), color.y(), color.z(), color.w()) {}

  const vx::TupleVector<double, 4>& asTuple() const { return col; }
  const QColor asQColor() const {
    return QColor::fromRgbF(std::get<0>(col), std::get<1>(col),
                            std::get<2>(col), std::get<3>(col));
  }
  const QVector4D asQVector4D() const {
    return QVector4D(std::get<0>(col), std::get<1>(col), std::get<2>(col),
                     std::get<3>(col));
  }

  static Color black() { return Color(std::make_tuple(0, 0, 0, 1)); }

  double red() const { return std::get<0>(col); }
  void setRed(double value) { std::get<0>(col) = value; }
  double green() const { return std::get<1>(col); }
  void setGreen(double value) { std::get<1>(col) = value; }
  double blue() const { return std::get<2>(col); }
  void setBlue(double value) { std::get<2>(col) = value; }
  double alpha() const { return std::get<3>(col); }
  void setAlpha(double value) { std::get<3>(col) = value; }
};
}  // namespace vx
