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

#include "Spectrum.hpp"

#include <VoxieClient/Format.hpp>

#include <VoxieBackend/Data/DataProperty.hpp>

#include <Voxie/Data/PiecewisePolynomialFunction.hpp>

#include <Voxie/Node/Node.hpp>
#include <Voxie/Node/Types.hpp>

QList<vx::SpectrumResult> vx::findMatchingSpectrums(
    const QList<PiecewisePolynomialFunction>& spectrums, double start,
    double end) {
  QList<vx::SpectrumResult> results;

  for (int i = 0; i < spectrums.size(); i++) {
    auto spectrum = spectrums[i];

    auto firstInterval = spectrum.intervals()[0];
    if (!firstInterval.isZero())
      qWarning() << "vx::findMatchingSpectrums(): Spectrum is non-zero before "
                    "first breakpoint";

    auto lastInterval = spectrum.intervals()[spectrum.intervals().size() - 1];
    if (!lastInterval.isZero())
      qWarning() << "vx::findMatchingSpectrums(): Spectrum is non-zero after "
                    "last breakpoint";

    if (spectrum.breakpoints().size() < 2) {
      qWarning() << "vx::findMatchingSpectrums(): Got spectrum with less than "
                    "two breakpoints";
      continue;
    }

    auto s_start = spectrum.breakpoints()[0].position();
    auto s_end =
        spectrum.breakpoints()[spectrum.breakpoints().size() - 1].position();

    if (s_start >= end || s_end <= start) continue;

    auto overlapStart = std::max(s_start, start);
    auto overlapEnd = std::min(s_end, end);

    auto weight = (overlapEnd - overlapStart) / (s_end - s_start);
    results.append(SpectrumResult(i, weight));
  }

  return results;
}

QList<vx::SpectrumResult> vx::findMatchingSpectrums(
    const QSharedPointer<SeriesDimension>& dimension, double start,
    double end) {
  if (dimension->property()->type() !=
      vx::types::PiecewisePolynomialFunctionType())
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "findMatchingSpectrums() called for dimension with "
                        "incorrect data type");

  QList<PiecewisePolynomialFunction> spectrums;
  for (const auto& entry : dimension->entries()) {
    // TODO: Move parsing a vx::Any to a QtType somewhere else?
    auto raw =
        Node::parseVariant<vx::types::PiecewisePolynomialFunction::RawType>(
            entry);
    auto polynomial = vx::PropertyValueConvertRaw<
        vx::types::PiecewisePolynomialFunction::RawType,
        vx::types::PiecewisePolynomialFunction::QtType>::fromRaw(raw);
    spectrums.append(polynomial);
  }

  return findMatchingSpectrums(spectrums, start, end);
}

vx::PiecewisePolynomialFunction vx::getValueSpectrum(
    const QList<PiecewisePolynomialFunction>& spectrums_,
    const QList<double>& values_) {
  bool debug = true;

  // TODO: Test this / clean this up
  // TODO: Handle overlapping spectrums

  if (spectrums_.size() != values_.size()) {
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.Error",
        "getValueSpectrum() called with lists with unequal size");
  }
  size_t count_ = spectrums_.size();

  // Filter out 0-values, check whether functions are non-zero at -/+inf
  QList<PiecewisePolynomialFunction> spectrums;
  QList<double> values;
  for (size_t i = 0; i < count_; i++) {
    if (!spectrums_[i].firstInterval().isZero())
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Spectrum is non-zero before first breakpoint");
    if (!spectrums_[i].lastInterval().isZero())
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Spectrum is non-zero after last breakpoint");

    if (values_[i] != 0) {
      spectrums << spectrums_[i];
      values << values_[i];
    }
  }
  size_t count = spectrums.size();

  std::vector<size_t> positions(count, 0);

  QList<PiecewisePolynomialFunction::Breakpoint> breakpointsW;
  QList<BernsteinPolynomial> intervalsW;
  QList<PiecewisePolynomialFunction::Breakpoint> breakpointsV;
  QList<BernsteinPolynomial> intervalsV;
  for (;;) {
    bool found = false;
    size_t selected = -1;
    double position = 0;
    for (size_t i = 0; i < count; i++) {
      const auto& spectrum = spectrums[i];

      if (positions[i] == (size_t)spectrum.breakpoints().size()) continue;

      if (!found ||
          position > spectrum.breakpoints()[positions[i]].position()) {
        found = true;
        selected = i;
        position = spectrum.breakpoints()[positions[i]].position();
      }
    }
    if (!found) break;

    const auto& spectrum = spectrums[selected];
    const auto& value = values[selected];
    const auto& breakpoint = spectrum.breakpoints()[positions[selected]];
    if (debug) qDebug() << "Got breakpoint" << value << breakpoint.position();

    if (breakpointsW.size() == 0) {
      breakpointsW << breakpoint;
      breakpointsV << PiecewisePolynomialFunction::Breakpoint(
          breakpoint.position(), breakpoint.value() == 0 ? 0 : value, 0);
      // This is the first interval
      intervalsW << BernsteinPolynomial({0});
      intervalsV << BernsteinPolynomial({NAN});
    } else {
      auto& lastBreakpointW = breakpointsW[breakpointsW.size() - 1];
      auto& lastBreakpointV = breakpointsV[breakpointsV.size() - 1];

      if (lastBreakpointW.position() < breakpoint.position()) {
        // Add an interval with "0" because there is a range without any data
        breakpointsW << breakpoint;
        intervalsW << BernsteinPolynomial({0});
        breakpointsV << PiecewisePolynomialFunction::Breakpoint(
            breakpoint.position(), breakpoint.value() == 0 ? 0 : value, 0);
        intervalsV << BernsteinPolynomial({NAN});
      } else if (lastBreakpointW.position() == breakpoint.position()) {
        // Merge breakpoints
        if (debug)
          qDebug() << "pre-merged" << lastBreakpointW.position()
                   << lastBreakpointW.value() << breakpoint.value();
        if (lastBreakpointW.value() != 0 && breakpoint.value() != 0)
          // Error: Overlapping at breakpoint
          throw vx::Exception(
              "de.uni_stuttgart.Voxie.Error",
              vx::format("Overlapping spectrum at breakpoint {}",
                         breakpoint.position()));
        else if (breakpoint.value() != 0) {
          lastBreakpointW = breakpoint;
          lastBreakpointV = PiecewisePolynomialFunction::Breakpoint(
              breakpoint.position(), value, 0);
        }
        if (debug)
          qDebug() << "merged" << lastBreakpointW.position()
                   << lastBreakpointW.value();
      } else {
        // Error: Overlapping intervals
        throw vx::Exception(
            "de.uni_stuttgart.Voxie.Error",
            vx::format("Overlapping spectrum at {}", breakpoint.position()));
      }
    }

    positions[selected]++;

    // Add all non-zero intervals
    for (;;) {
      const auto& interval = spectrum.intervals()[positions[selected]];
      if (interval.isZero()) break;

      const auto& breakpoint2 = spectrum.breakpoints()[positions[selected]];
      if (debug)
        qDebug() << "Got interval and breakpoint" << interval.coefficients()
                 << breakpoint2.position();

      breakpointsW << breakpoint2;
      intervalsW << interval;
      breakpointsV << PiecewisePolynomialFunction::Breakpoint(
          breakpoint2.position(), breakpoint2.value() == 0 ? 0 : value, 0);
      intervalsV << BernsteinPolynomial({value});

      positions[selected]++;
    }
  }

  // Add 0-interval at end
  intervalsW << BernsteinPolynomial({0});
  intervalsV << BernsteinPolynomial({NAN});

  auto polynomialW = PiecewisePolynomialFunction(breakpointsW, intervalsW);
  auto polynomialV = PiecewisePolynomialFunction(breakpointsV, intervalsV);
  if (debug)
    qDebug() << "Weights:" << polynomialW.description().toUtf8().data();
  return polynomialV;
}
