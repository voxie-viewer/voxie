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

#include "PiecewisePolynomialFunction.hpp"

#include <VoxieClient/Exception.hpp>
#include <VoxieClient/Format.hpp>

#include <cmath>

vx::BernsteinPolynomial::BernsteinPolynomial(const QList<double>& coefficients)
    : coefficients_(coefficients) {
  if (this->coefficients().size() == 0)
    throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "Got Bernstein polynomial with 0 coefficients");
}
vx::BernsteinPolynomial::~BernsteinPolynomial() {}

vx::BernsteinPolynomial vx::BernsteinPolynomial::parse(
    const QList<double>& raw) {
  return BernsteinPolynomial(raw);
}
QList<double> vx::BernsteinPolynomial::toRaw() const {
  return this->coefficients();
}

vx::PiecewisePolynomialFunction::Breakpoint::Breakpoint(double position,
                                                        double value,
                                                        double integral)
    : position_(position), value_(value), integral_(integral) {
  if (!std::isfinite(this->position())) {
    throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "Breakpoint has non-finite position");
  }

  if (std::isfinite(this->value())) {
    // Finite value
    if (std::isnan(this->integral()) || this->integral() != 0)
      throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                          "Breakpoint has nonzero integral but finite value");
  } else if (std::isinf(this->value()) && this->value() < 0) {
    // Negative infinity
    if (!std::isnan(this->integral()) && this->integral() > 0)
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.InvalidArgument",
          "Breakpoint has positive integral but negative value");
  } else if (std::isinf(this->value()) && this->value() > 0) {
    // Positive infinity
    if (!std::isnan(this->integral()) && this->integral() < 0)
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.InvalidArgument",
          "Breakpoint has negative integral but positive value");
  } else {
    // NaN
    // Integral can be anything
  }
}
vx::PiecewisePolynomialFunction::Breakpoint::~Breakpoint() {}

vx::PiecewisePolynomialFunction::Breakpoint
vx::PiecewisePolynomialFunction::Breakpoint::parse(
    const std::tuple<double, std::tuple<double, double>>& raw) {
  return Breakpoint(std::get<0>(raw), std::get<0>(std::get<1>(raw)),
                    std::get<1>(std::get<1>(raw)));
}
std::tuple<double, std::tuple<double, double>>
vx::PiecewisePolynomialFunction::Breakpoint::toRaw() const {
  return std::make_tuple(this->position(),
                         std::make_tuple(this->value(), this->integral()));
}

vx::PiecewisePolynomialFunction::PiecewisePolynomialFunction(
    const QList<Breakpoint>& breakpoints,
    const QList<BernsteinPolynomial>& intervals)
    : breakpoints_(breakpoints), intervals_(intervals) {
  if (breakpoints.size() + 1 != intervals.size())
    throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "Invalid number of intervals");

  for (int i = 0; i + 1 < breakpoints.size(); i++) {
    if (!(breakpoints[i].position() < breakpoints[i + 1].position()))
      throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                          vx::format("Breakpoint {} position {} is not before "
                                     "breakpoint {} position {}",
                                     i, breakpoints[i].position(), i + 1,
                                     breakpoints[i + 1].position()));
  }
}
vx::PiecewisePolynomialFunction::~PiecewisePolynomialFunction() {}

vx::PiecewisePolynomialFunction vx::PiecewisePolynomialFunction::parse(
    const std::tuple<QList<std::tuple<double, std::tuple<double, double>>>,
                     QList<QList<double>>>& raw) {
  QList<Breakpoint> breakpoints;
  for (const auto& breakpoint : std::get<0>(raw))
    breakpoints.append(Breakpoint::parse(breakpoint));

  QList<BernsteinPolynomial> intervals;
  for (const auto& interval : std::get<1>(raw))
    intervals.append(BernsteinPolynomial::parse(interval));

  return PiecewisePolynomialFunction(breakpoints, intervals);
}
std::tuple<QList<std::tuple<double, std::tuple<double, double>>>,
           QList<QList<double>>>
vx::PiecewisePolynomialFunction::toRaw() const {
  QList<std::tuple<double, std::tuple<double, double>>> breakpoints;
  for (const auto& breakpoint : this->breakpoints())
    breakpoints.append(breakpoint.toRaw());

  QList<QList<double>> intervals;
  for (const auto& interval : this->intervals())
    intervals.append(interval.toRaw());

  return std::make_tuple(breakpoints, intervals);
}
