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

#include <QString>

namespace vx {

/**
 * @brief MetricUnit represents an SI unit, consisting of a base unit, an
 * exponent and a magnitude prefix.
 */
class VOXIECORESHARED_EXPORT MetricUnit {
 public:
  enum class BaseType {
    None,
    Meter,
  };

  enum class Prefix {
    Yocto = -24,
    Zepto = -21,
    Atto = -18,
    Femto = -15,
    Pico = -12,
    Nano = -9,
    Micro = -6,
    Milli = -3,
    Centi = -2,
    Deci = -1,

    None = 0,

    Deca = 1,
    Hecto = 2,
    Kilo = 3,
    Mega = 6,
    Giga = 9,
    Tera = 12,
    Peta = 15,
    Exa = 18,
    Zetta = 21,
    Yotta = 24,
  };

  MetricUnit() = default;

  MetricUnit(const MetricUnit&) = default;
  MetricUnit& operator=(const MetricUnit&) = default;

  MetricUnit(MetricUnit&&) = default;
  MetricUnit& operator=(MetricUnit&&) = default;

  MetricUnit(BaseType baseType, int exponent = 1, Prefix prefix = Prefix::None);
  MetricUnit(QString unitString);

  double getConversionRate(const MetricUnit& toUnit) const;
  double convert(const MetricUnit& toUnit, double value) const;

  QString toString() const;

  void setBaseType(BaseType baseType);
  BaseType getBaseType() const;

  void setExponent(int exponent);
  int getExponent() const;

  void setPrefix(Prefix prefix);
  Prefix getPrefix() const;

  operator bool() const;

  static QString baseTypeToString(BaseType baseType);
  static BaseType stringToBaseType(QString baseTypeString);

  static QString prefixToString(Prefix prefix);
  static Prefix stringToPrefix(QString prefixString);

  static Prefix findClosestValidPrefix(int magnitude);

 private:
  BaseType baseType = BaseType::None;
  int exponent = 1;
  Prefix prefix = Prefix::None;
};

}  // namespace vx
