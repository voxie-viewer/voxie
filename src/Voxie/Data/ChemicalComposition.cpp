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

#include "ChemicalComposition.hpp"

#include <VoxieClient/Exception.hpp>
#include <VoxieClient/Format.hpp>
#include <VoxieClient/JsonUtil.hpp>
#include <VoxieClient/QtUtil.hpp>

#include <QtCore/QJsonArray>

#include <array>

vx::ChemicalComposition::ChemicalComposition() {}
vx::ChemicalComposition::~ChemicalComposition() {}

QSharedPointer<vx::ChemicalComposition> vx::ChemicalComposition::parse(
    const QJsonValue& json) {
  auto array = vx::expectArray(json);
  if (array.size() == 0)
    throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "Got empty array in ChemicalComposition");
  auto type = vx::expectString(array[0]);
  if (type == "element") {
    return ChemicalCompositionElement::parseImpl(array);
  } else if (type == "molecule") {
    return ChemicalCompositionMolecule::parseImpl(array);
  } else if (type == "compound") {
    return ChemicalCompositionCompound::parseImpl(array);
  } else if (type == "meta") {
    return ChemicalCompositionMeta::parseImpl(array);
  } else {
    throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "Invalid ChemicalComposition type: `" + type + "`");
  }
}

vx::ChemicalCompositionElement::ChemicalCompositionElement(
    uint64_t atomicNumber, bool haveNucleonNumber, uint64_t nucleonNumber)
    : atomicNumber_(atomicNumber),
      haveNucleonNumber_(haveNucleonNumber),
      nucleonNumber_(nucleonNumber) {
  // TODO: Checks
}
vx::ChemicalCompositionElement::~ChemicalCompositionElement() {}

QSharedPointer<vx::ChemicalCompositionElement>
vx::ChemicalCompositionElement::parseImpl(const QJsonArray& array) {
  if (array.size() < 2)
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InvalidArgument",
        "Not enough values in array for ChemicalCompositionElement");
  uint64_t atomicNumber = vx::expectUnsignedInt(array[1]);

  bool haveNucleonNumber = false;
  uint64_t nucleonNumber = 0;
  if (array.size() >= 3 && !vx::isNull(array[2])) {
    haveNucleonNumber = true;
    nucleonNumber = vx::expectUnsignedInt(array[2]);
  }

  return createQSharedPointer<ChemicalCompositionElement>(
      atomicNumber, haveNucleonNumber, nucleonNumber);
}
QJsonArray vx::ChemicalCompositionElement::toJson() const {
  QJsonArray res{
      "element",
      qint64(atomicNumber()),
  };
  if (haveNucleonNumber()) res << qint64(nucleonNumber());
  return res;
}

static const char* symbols[] = {
    "H",  "He", "Li", "Be", "B",  "C",  "N",  "O",  "F",  "Ne", "Na", "Mg",
    "Al", "Si", "P",  "S",  "Cl", "Ar", "K",  "Ca", "Sc", "Ti", "V",  "Cr",
    "Mn", "Fe", "Co", "Ni", "Cu", "Zn", "Ga", "Ge", "As", "Se", "Br", "Kr",
    "Rb", "Sr", "Y",  "Zr", "Nb", "Mo", "Tc", "Ru", "Rh", "Pd", "Ag", "Cd",
    "In", "Sn", "Sb", "Te", "I",  "Xe", "Cs", "Ba", "La", "Ce", "Pr", "Nd",
    "Pm", "Sm", "Eu", "Gd", "Tb", "Dy", "Ho", "Er", "Tm", "Yb", "Lu", "Hf",
    "Ta", "W",  "Re", "Os", "Ir", "Pt", "Au", "Hg", "Tl", "Pb", "Bi", "Po",
    "At", "Rn", "Fr", "Ra", "Ac", "Th", "Pa", "U",  "Np", "Pu", "Am", "Cm",
    "Bk", "Cf", "Es", "Fm", "Md", "No", "Lr", "Rf", "Db", "Sg", "Bh", "Hs",
    "Mt", "Ds", "Rg", "Cn", "Nh", "Fl", "Mc", "Lv", "Ts", "Og",
};

template <typename T>
static QString numToString(T value, std::array<const char*, 11> digits) {
  QString s = QString::number(value);

  QString res;
  for (const auto& c : s) {
    if (c >= '0' && c <= '9')
      res += digits[c.unicode() - '0'];
    else if (c == '.')
      res += digits[10];
    else
      res += c;
  }
  return res;
}

QString vx::ChemicalCompositionElement::toString() const {
  if (atomicNumber() < 1)
    throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "atomicNumber < 1");
  size_t pos = atomicNumber() - 1;
  if (pos >= sizeof(symbols) / sizeof(*symbols))
    throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "atomicNumber too large");
  QString symbol = symbols[pos];

  if (haveNucleonNumber())
    return numToString(nucleonNumber(), {"⁰", "¹", "²", "³", "⁴", "⁵", "⁶", "⁷",
                                         "⁸", "⁹", "."}) +
           symbol;
  else
    return symbol;
}
QString vx::ChemicalCompositionElement::description() const {
  return toString();
}

vx::ChemicalCompositionMolecule::Member::Member(
    double fraction, const QSharedPointer<ChemicalComposition>& composition)
    : fraction_(fraction), composition_(composition) {}
vx::ChemicalCompositionMolecule::Member::~Member() {}

vx::ChemicalCompositionMolecule::Member
vx::ChemicalCompositionMolecule::Member::parse(const QJsonArray& array) {
  if (array.size() < 2)
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InvalidArgument",
        "Not enough values in array for ChemicalCompositionMolecule::Member");

  double fraction = vx::expectDouble(array[0]);
  auto composition = vx::ChemicalComposition::parse(array[1]);
  return Member(fraction, composition);
}
QJsonArray vx::ChemicalCompositionMolecule::Member::toJson() const {
  return QJsonArray{
      this->fraction(),
      this->composition()->toJson(),
  };
}

vx::ChemicalCompositionMolecule::ChemicalCompositionMolecule(
    const QList<Member>& members)
    : members_(members) {
  // TODO: Checks
}
vx::ChemicalCompositionMolecule::~ChemicalCompositionMolecule() {}

QSharedPointer<vx::ChemicalCompositionMolecule>
vx::ChemicalCompositionMolecule::parseImpl(const QJsonArray& array) {
  if (array.size() < 2)
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InvalidArgument",
        "Not enough values in array for ChemicalCompositionMolecule");

  QList<Member> members;
  for (const auto& member : vx::expectArray(array[1]))
    members.append(Member::parse(vx::expectArray(member)));

  return createQSharedPointer<ChemicalCompositionMolecule>(members);
}
QJsonArray vx::ChemicalCompositionMolecule::toJson() const {
  QJsonArray membersJson;
  for (const auto& member : members()) membersJson << member.toJson();
  return QJsonArray{
      "molecule",
      membersJson,
  };
}

QString vx::ChemicalCompositionMolecule::toString() const {
  QString res;
  for (const auto& member : this->members()) {
    if (qSharedPointerDynamicCast<vx::ChemicalCompositionElement>(
            member.composition())) {
      res += member.composition()->toString();
    } else if (qSharedPointerDynamicCast<vx::ChemicalCompositionMolecule>(
                   member.composition())) {
      res += "(" + member.composition()->toString() + ")";
    } else {
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.Error",
          "Invalid member in ChemicalCompositionMolecule::toString()");
    }
    if (member.fraction() != 1)
      res += numToString(member.fraction(), {"₀", "₁", "₂", "₃", "₄", "₅", "₆",
                                             "₇", "₈", "₉", "."});
  }
  return res;
}
QString vx::ChemicalCompositionMolecule::description() const {
  return toString();
}

vx::ChemicalCompositionCompound::Member::Member(
    double fraction, const QSharedPointer<ChemicalComposition>& composition)
    : fraction_(fraction), composition_(composition) {}
vx::ChemicalCompositionCompound::Member::~Member() {}

vx::ChemicalCompositionCompound::Member
vx::ChemicalCompositionCompound::Member::parse(const QJsonArray& array) {
  if (array.size() < 2)
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InvalidArgument",
        "Not enough values in array for ChemicalCompositionCompound::Member");

  double fraction = vx::expectDouble(array[0]);
  auto composition = vx::ChemicalComposition::parse(array[1]);
  return Member(fraction, composition);
}
QJsonArray vx::ChemicalCompositionCompound::Member::toJson() const {
  return QJsonArray{
      this->fraction(),
      this->composition()->toJson(),
  };
}

vx::ChemicalCompositionCompound::ChemicalCompositionCompound(
    const QString& typeString, const QList<Member>& members)
    : typeString_(typeString), members_(members) {
  // TODO: Checks (e.g. type)
}
vx::ChemicalCompositionCompound::~ChemicalCompositionCompound() {}

QSharedPointer<vx::ChemicalCompositionCompound>
vx::ChemicalCompositionCompound::parseImpl(const QJsonArray& array) {
  if (array.size() < 3)
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InvalidArgument",
        "Not enough values in array for ChemicalCompositionCompound");

  auto type = vx::expectString(array[1]);

  QList<Member> members;
  for (const auto& member : vx::expectArray(array[2]))
    members.append(Member::parse(vx::expectArray(member)));

  return createQSharedPointer<ChemicalCompositionCompound>(type, members);
}
QJsonArray vx::ChemicalCompositionCompound::toJson() const {
  QJsonArray membersJson;
  for (const auto& member : this->members()) membersJson << member.toJson();
  return QJsonArray{
      "compound",
      this->typeString(),
      membersJson,
  };
}

QString vx::ChemicalCompositionCompound::toString() const {
  QString res;
  res += "Compound " + this->typeString() + "[";
  bool first = true;
  for (const auto& member : this->members()) {
    if (!first) res += " + ";
    if (member.fraction() != 1) res += QString::number(member.fraction());
    res += member.composition()->toString();
    first = false;
  }
  res += "]";
  return res;
}
QString vx::ChemicalCompositionCompound::description() const {
  return toString();
}

vx::ChemicalCompositionMeta::ChemicalCompositionMeta(
    const QSharedPointer<ChemicalComposition>& member,
    const QJsonObject& information)
    : member_(member), information_(information) {
  // TODO: Checks
}
vx::ChemicalCompositionMeta::~ChemicalCompositionMeta() {}

QSharedPointer<vx::ChemicalCompositionMeta>
vx::ChemicalCompositionMeta::parseImpl(const QJsonArray& array) {
  if (array.size() < 3)
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InvalidArgument",
        "Not enough values in array for ChemicalCompositionMeta");

  auto member = vx::ChemicalComposition::parse(array[1]);

  auto information = vx::expectObject(array[2]);

  return createQSharedPointer<ChemicalCompositionMeta>(member, information);
}
QJsonArray vx::ChemicalCompositionMeta::toJson() const {
  return QJsonArray{
      "meta",
      this->member()->toJson(),
      this->information(),
  };
}

QString vx::ChemicalCompositionMeta::toString() const {
  QString str = "<" + this->member()->toString() + ">";
  if (this->information().contains("Density")) {
    str += vx::format(
        //" ρ={} g/cm³",
        " ρ={}", vx::expectDouble(this->information()["Density"]) / 1000);
  }
  if (this->information().contains("Description")) {
    str += " / " + vx::expectString(this->information()["Description"]);
  }
  return str;
}
QString vx::ChemicalCompositionMeta::description() const {
  return "<" + this->member()->toString() + "> " +
         QJsonDocument(this->information()).toJson();
}
