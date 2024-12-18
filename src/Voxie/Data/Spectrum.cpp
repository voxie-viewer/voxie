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
    // TODO: Move parsing a QVariant to a QtType somewhere else?
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
