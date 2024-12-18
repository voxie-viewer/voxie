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

#include <QtCore/QList>

namespace vx {
class VOXIECORESHARED_EXPORT BernsteinPolynomial {
  QList<double> coefficients_;

 public:
  explicit BernsteinPolynomial(const QList<double>& coefficients);
  ~BernsteinPolynomial();

  const QList<double>& coefficients() const { return coefficients_; }

  static BernsteinPolynomial parse(const QList<double>& raw);
  QList<double> toRaw() const;

  bool isZero() const {
    for (const auto& coefficient : this->coefficients())
      if (coefficient != 0) return false;
    return true;
  }
};

class VOXIECORESHARED_EXPORT PiecewisePolynomialFunction {
 public:
  class VOXIECORESHARED_EXPORT Breakpoint {
    double position_;
    double value_;
    double integral_;

   public:
    explicit Breakpoint(double position, double value, double integral);
    ~Breakpoint();

    double position() const { return position_; }
    double value() const { return value_; }
    double integral() const { return integral_; }

    static Breakpoint parse(
        const std::tuple<double, std::tuple<double, double>>& raw);
    std::tuple<double, std::tuple<double, double>> toRaw() const;
  };

 private:
  QList<Breakpoint> breakpoints_;
  QList<BernsteinPolynomial> intervals_;

 public:
  explicit PiecewisePolynomialFunction(
      const QList<Breakpoint>& breakpoints,
      const QList<BernsteinPolynomial>& intervals);
  ~PiecewisePolynomialFunction();

  const QList<Breakpoint>& breakpoints() const { return breakpoints_; }
  const QList<BernsteinPolynomial>& intervals() const { return intervals_; }

  static PiecewisePolynomialFunction parse(
      const std::tuple<QList<std::tuple<double, std::tuple<double, double>>>,
                       QList<QList<double>>>& raw);
  std::tuple<QList<std::tuple<double, std::tuple<double, double>>>,
             QList<QList<double>>>
  toRaw() const;
};
}  // namespace vx
