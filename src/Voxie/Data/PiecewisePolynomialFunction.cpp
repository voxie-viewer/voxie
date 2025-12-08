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
#include <VoxieClient/JsonUtil.hpp>

#include <QJsonArray>

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

bool vx::BernsteinPolynomial::isZero() const {
  for (const auto& coefficient : this->coefficients())
    if (coefficient != 0) return false;
  return true;
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

vx::PiecewisePolynomialFunction vx::PiecewisePolynomialFunction::zero() {
  return PiecewisePolynomialFunction({}, {BernsteinPolynomial({0})});
}

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

QString vx::PiecewisePolynomialFunction::toString() const {
  if (intervals().size() == 0) {
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "intervals().size() == 0");
  }

  if (breakpoints().size() == 0) {
    return intervals()[0].isZero() ? "0" : "!=0";
  } else {
    auto i0 = intervals()[0].isZero() ? "0" : "!=0";
    auto i1 = intervals()[intervals().size() - 1].isZero() ? "0" : "!=0";
    return vx::format("{} [{} - {}] {}", i0, breakpoints()[0].position(),
                      breakpoints()[breakpoints().size() - 1].position(), i1);
  }
}

QString vx::PiecewisePolynomialFunction::description() const {
  QString res;

  for (int i = 0; i < intervals().size(); i++) {
    if (i > 0) {
      auto bp = breakpoints().at(i - 1);
      res += vx::format("Breakpoint at {} value {} integral {}\n",
                        bp.position(), bp.value(), bp.integral());
    }
    auto interv = intervals().at(i);
    res += vx::format("Polynomial rank {}:", interv.coefficients().size() - 1);
    for (const auto& coeff : interv.coefficients())
      res += vx::format(" {}", coeff);
    res += "\n";
  }

  res += "\n";
  res += QJsonDocument(this->toJson()).toJson();

  return res;
}

// TODO: Move this somewhere else?
namespace {

// Tuple iteration function (recursive step)
template <std::size_t RevIndex, typename... T>
struct JsonConversionHelperTuple {
  template <typename Func>
  static void tupleForEach(const std::tuple<T...>& tuple, const Func& func) {
    // Invoke callback function on current value
    func(std::get<sizeof...(T) - RevIndex>(tuple));

    // Invoke iteration function for next index
    JsonConversionHelperTuple<RevIndex - 1, T...>::tupleForEach(tuple, func);
  }
};

// Tuple iteration function (recursive tail)
template <typename... T>
struct JsonConversionHelperTuple<0, T...> {
  template <typename Func>
  static void tupleForEach(const std::tuple<T...>&, const Func&) {}
};

template <typename T>
struct JsonConversionHelper;

/*
// Json conversion fallback (empty string)
template <typename T>
struct JsonConversionHelper {
  using FallbackTag = void;
  static QJsonValue toJson(const T&) { return QJsonValue(); }
};
*/

// Json conversion specialization (QList)
template <typename T>
struct JsonConversionHelper<QList<T>> {
  static QJsonArray toJson(const QList<T>& value) {
    QJsonArray result;
    for (const auto& entry : value) {
      result.append(JsonConversionHelper<T>::toJson(entry));
    }
    return result;
  }
};

// Json conversion specialization (string)
template <>
struct JsonConversionHelper<QString> {
  static QJsonValue toJson(const QString& value) { return value; }
};

// Json conversion specialization (unsigned int)
template <>
struct JsonConversionHelper<quint64> {
  static QJsonValue toJson(quint64 value) {
    // TODO: Handle values >= 2**63
    return QJsonValue((qint64)value);
  }
};

// Json conversion specialization (signed int)
template <>
struct JsonConversionHelper<qint64> {
  static QJsonValue toJson(qint64 value) { return QJsonValue(value); }
};

// Json conversion specialization (single-precision floating point value)
template <>
struct JsonConversionHelper<float> {
  static QJsonValue toJson(float value) {
    if (std::isnan(value))
      return "NaN";
    else if (value == INFINITY)
      return "Infinity";
    else if (value == -INFINITY)
      return "-Infinity";
    else
      return value;
  }
};

// Json conversion specialization (double-precision floating point value)
template <>
struct JsonConversionHelper<double> {
  static QJsonValue toJson(double value) {
    if (std::isnan(value))
      return "NaN";
    else if (value == INFINITY)
      return "Infinity";
    else if (value == -INFINITY)
      return "-Infinity";
    else
      return value;
  }
};

// Json conversion specialization (tuple)
template <typename... T>
struct JsonConversionHelper<std::tuple<T...>> {
  static QJsonArray toJson(const std::tuple<T...>& tuple) {
    QJsonArray elements;
    JsonConversionHelperTuple<sizeof...(T), T...>::tupleForEach(
        tuple, [&](const auto& value) {
          using ElemType = typename std::decay<decltype(value)>::type;
          elements.append(JsonConversionHelper<ElemType>::toJson(value));
        });
    return elements;
  }
};

// Trait class to determine string conversion template specialization status
template <typename T, typename = void>
struct JsonConversionFallbackHelper {
  static constexpr bool isFallback = false;
};

/*
template <typename T>
struct JsonConversionFallbackHelper<
    T, typename JsonConversionHelper<T>::FallbackTag> {
  static constexpr bool isFallback = true;
};
*/

template <typename T>
static auto convertToJson(const T& t) {
  return JsonConversionHelper<T>::toJson(t);
}
}  // namespace

QJsonArray vx::PiecewisePolynomialFunction::toJson() const {
  return convertToJson(this->toRaw());
}
