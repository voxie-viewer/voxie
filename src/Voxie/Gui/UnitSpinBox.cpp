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

#include "UnitSpinBox.hpp"

#include <QtCore/QDebug>

#include <cmath>

using namespace vx;

struct PrefixInfo {
  const char* prefix;
  double value;
};
static const PrefixInfo prefixes[]{
    // https://en.wikipedia.org/wiki/Metric_prefix#List_of_SI_prefixes
    // https://en.wikipedia.org/w/index.php?title=Metric_prefix&oldid=925982941#List_of_SI_prefixes
    {"Y", 1e+24},
    {"Z", 1e+21},
    {"E", 1e+18},
    {"P", 1e+15},
    {"T", 1e+12},
    {"G", 1e+09},
    {"M", 1e+06},
    {"k", 1e+03},
    //{ "h", 1e+02 },
    //{ "da", 1e+01 },
    {"", 1e+00},
    //{"d", 1e-01},
    //{"c", 1e-02},
    {"m", 1e-03},
    {"µ", 1e-06},
    {"n", 1e-09},
    {"p", 1e-12},
    {"f", 1e-15},
    {"a", 1e-18},
    {"z", 1e-21},
    {"y", 1e-24},
};

static void stripSuffix(const QString& text, const QString& unitSuffix,
                        QString& outText, double& outPrefixValue) {
  QString text2 = text;
  if (text.endsWith(unitSuffix))
    text2 = text.left(text.length() - unitSuffix.length());

  if (text2.length() > 0 && text2[text2.length() - 1] == ' ')
    text2 = text2.left(text2.length() - 1);

  outPrefixValue = 1;
  int matchLength = 0;

  for (const auto& prefix : prefixes) {
    if (!text2.endsWith(prefix.prefix)) continue;

    QString prefixText = prefix.prefix;
    if (prefixText.length() <= matchLength) continue;

    matchLength = prefixText.length();
    outPrefixValue = prefix.value;
  }

  // Also strip space between number and prefix
  while (matchLength < text2.length() &&
         text2[text2.length() - matchLength - 1] == ' ')
    matchLength++;

  outText = text2.left(text2.length() - matchLength);
}

UnitSpinBox::UnitSpinBox(QWidget* parent) : QDoubleSpinBox(parent) {
  this->realDecimals = 3;  // TODO

  this->setDecimals(1000);
}
UnitSpinBox::~UnitSpinBox() {}

QValidator::State UnitSpinBox::validate(QString& text, int& pos) const {
  // qDebug() << "UnitSpinBox::validate" << text << pos;
  // auto state = QDoubleSpinBox::validate(text, pos);

  QString textNumber;
  double prefixValue;
  stripSuffix(text, this->suffix(), textNumber, prefixValue);
  int realPos = pos > textNumber.length() ? pos : -1;
  textNumber += this->suffix();
  auto state = QDoubleSpinBox::validate(textNumber, pos);
  if (pos == textNumber.length() && realPos != -1) pos = realPos;
  // TODO: forward changes of textNumber to text?

  // qDebug() << "UnitSpinBox::validate res" << text << pos << state;
  return state;
}
void UnitSpinBox::fixup(QString& input) const {
  // qDebug() << "UnitSpinBox::fixup" << input;
  // TODO
  QDoubleSpinBox::fixup(input);
  // qDebug() << "UnitSpinBox::fixup res" << input;
}

QString UnitSpinBox::textFromValue(double value) const {
  double svalue;
  const char* usedPrefix;
  if (value == 0) {
    svalue = value;
    usedPrefix = "";
  } else {
    for (const auto& prefix : prefixes) {
      usedPrefix = prefix.prefix;
      svalue = value / prefix.value;

      if (std::abs(svalue) >= 1) break;
    }
  }

  // return QDoubleSpinBox::textFromValue(svalue) + " " + usedPrefix;
  return this->locale().toString(svalue, 'f', realDecimals) + " " + usedPrefix;
}

double UnitSpinBox::valueFromText(const QString& text) const {
  QString textNumber;
  double prefixValue;
  stripSuffix(text, this->suffix(), textNumber, prefixValue);

  return QDoubleSpinBox::valueFromText(textNumber + this->suffix()) *
         prefixValue;
}

// TODO: cursor, allow more editing
// TODO: allow selecting and removing the unit?

static double getStepSize(double currentValue, bool forward) {
  int minStepExp = -24;
  int maxStepExp = 24;

  double absValue = std::abs(currentValue);
  bool isDown = forward ^ (currentValue >= 0);

  // TODO: work on rounding, it seems like 100µm is increased to 101µm

  int exp = minStepExp;
  // While the next larger step size would still be smaller than 1/10 of the
  // value, increase the step size
  while (exp < maxStepExp && std::pow(10, exp + 2) <= absValue) exp++;
  // If a step downwards would step across the 10^x line, use the smaller step
  // size
  if (isDown && exp > minStepExp &&
      !(std::pow(10, exp + 1) <= absValue - std::pow(10, exp)))
    exp--;

  // qDebug() << "getStepSize" << currentValue << forward << exp;
  return std::pow(10, exp);
}

void UnitSpinBox::stepBy(int steps) {
  // qDebug() << "UnitSpinBox::stepBy" << steps;
  // QDoubleSpinBox::stepBy(steps);

  double newValue = this->value();
  if (steps > 0) {
    for (int i = 0; i < steps; i++) newValue += getStepSize(newValue, true);
  } else if (steps < 0) {
    for (int i = 0; i > steps; i--) newValue -= getStepSize(newValue, false);
  }
  setValue(newValue);
}
