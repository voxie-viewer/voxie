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

#include <Voxie/Data/MetricUnit.hpp>

#include <VoxieClient/Exception.hpp>

#include <QMetaEnum>
#include <QRegularExpression>

#include <cmath>

using namespace vx;

MetricUnit::MetricUnit(MetricUnit::BaseType baseType, int exponent,
                       MetricUnit::Prefix prefix)
    : baseType(baseType), exponent(exponent), prefix(prefix) {}

MetricUnit::MetricUnit(QString unitString) {
  QRegularExpression unitRegex("(\\w*)(\\w)(\\^\\d+)?");
  auto result = unitRegex.match(unitString);
  if (result.hasMatch()) {
    setPrefix(stringToPrefix(result.captured(1)));
    setBaseType(stringToBaseType(result.captured(2)));
    setExponent(result.captured(3).mid(1).toInt());
  }
}

double MetricUnit::getConversionRate(const MetricUnit& toUnit) const {
  if (getBaseType() != toUnit.getBaseType() ||
      getExponent() != toUnit.getExponent()) {
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.Error",
        QString("Unit conversion failed: cannot convert from %1 to %2")
            .arg(toString(), toUnit.toString()));
  }

  int magnitude = (int(getPrefix()) - int(toUnit.getPrefix())) * getExponent();
  return std::pow(10, magnitude);
}

double MetricUnit::convert(const MetricUnit& toUnit, double value) const {
  return getConversionRate(toUnit) * value;
}

QString MetricUnit::toString() const {
  return prefixToString(getPrefix()) + baseTypeToString(getBaseType()) +
         (getExponent() != 1 ? "^" + QString::number(getExponent()) : "");
}

template <typename Enum>
QMap<QString, Enum> generateEnumLookupMap(Enum min, Enum max, Enum fallback,
                                          QString toString(Enum)) {
  QMap<QString, Enum> map;

  using Int = typename std::underlying_type<Enum>::type;

  QString fallbackString = toString(fallback);
  map.insert(fallbackString, fallback);

  for (Int i = Int(min); i <= Int(max); ++i) {
    QString string = toString(Enum(i));
    if (string != fallbackString) {
      map.insert(string, Enum(i));
    }
  }

  return map;
}

QString MetricUnit::baseTypeToString(MetricUnit::BaseType baseType) {
  switch (baseType) {
    case BaseType::Meter:
      return "m";

    case BaseType::None:
    default:
      return "";
  }
}

MetricUnit::BaseType MetricUnit::stringToBaseType(QString baseTypeString) {
  static QMap<QString, BaseType> lookupMap =
      generateEnumLookupMap(BaseType::None, BaseType::Meter, BaseType::None,
                            &MetricUnit::baseTypeToString);

  return lookupMap.value(baseTypeString, BaseType::None);
}

QString MetricUnit::prefixToString(MetricUnit::Prefix prefix) {
  switch (prefix) {
    case Prefix::Yocto:
      return "y";
    case Prefix::Zepto:
      return "z";
    case Prefix::Atto:
      return "a";
    case Prefix::Femto:
      return "f";
    case Prefix::Pico:
      return "p";
    case Prefix::Nano:
      return "n";
    case Prefix::Micro:
      return u8"µ";
    case Prefix::Milli:
      return "m";
    case Prefix::Centi:
      return "c";
    case Prefix::Deci:
      return "d";

    case Prefix::Deca:
      return "da";
    case Prefix::Hecto:
      return "h";
    case Prefix::Kilo:
      return "k";
    case Prefix::Mega:
      return "M";
    case Prefix::Giga:
      return "G";
    case Prefix::Tera:
      return "T";
    case Prefix::Peta:
      return "P";
    case Prefix::Exa:
      return "E";
    case Prefix::Zetta:
      return "Z";
    case Prefix::Yotta:
      return "Y";

    case Prefix::None:
    default:
      return "";
  }
}

MetricUnit::Prefix MetricUnit::stringToPrefix(QString prefixString) {
  static QMap<QString, Prefix> lookupMap = generateEnumLookupMap(
      Prefix::Yocto, Prefix::Yotta, Prefix::None, &MetricUnit::prefixToString);

  return lookupMap.value(prefixString, Prefix::None);
}

MetricUnit::Prefix MetricUnit::findClosestValidPrefix(int magnitude) {
  // Magnitudes between -3 and +3 are available as direct prefixes
  if (magnitude >= int(Prefix::Milli) && magnitude <= int(Prefix::Kilo)) {
    return Prefix(magnitude);
  }

  // Determine sign (magnitude can't be zero here due to the check above)
  int sign = magnitude / std::abs(magnitude);

  // Find nearest multiple of 3
  magnitude = (((magnitude * sign) + 1) / 3 * 3) * sign;

  // Clamp to SI prefix range
  return Prefix(
      std::max(int(Prefix::Yocto), std::min(magnitude, int(Prefix::Yotta))));
}

void MetricUnit::setBaseType(BaseType baseType) { this->baseType = baseType; }

MetricUnit::BaseType MetricUnit::getBaseType() const { return baseType; }

void MetricUnit::setExponent(int exponent) { this->exponent = exponent; }

int MetricUnit::getExponent() const { return std::max(exponent, 1); }

void MetricUnit::setPrefix(Prefix prefix) { this->prefix = prefix; }

MetricUnit::Prefix MetricUnit::getPrefix() const { return prefix; }

MetricUnit::operator bool() const { return getBaseType() != BaseType::None; }
