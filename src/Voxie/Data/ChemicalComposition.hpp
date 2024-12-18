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

#include <QtCore/QSharedPointer>

#include <QtCore/QJsonObject>

class QJsonValue;
class QJsonArray;

namespace vx {
class VOXIECORESHARED_EXPORT ChemicalComposition {
 public:
  ChemicalComposition();
  virtual ~ChemicalComposition();

  static QSharedPointer<ChemicalComposition> parse(const QJsonValue& json);
  virtual QJsonArray toJson() const = 0;

  virtual QString toString() const = 0;
  virtual QString description() const = 0;
};

class VOXIECORESHARED_EXPORT ChemicalCompositionElement
    : public ChemicalComposition {
  uint64_t atomicNumber_;
  bool haveNucleonNumber_;
  uint64_t nucleonNumber_;

 public:
  ChemicalCompositionElement(uint64_t atomicNumber, bool haveNucleonNumber,
                             uint64_t nucleonNumber);
  ~ChemicalCompositionElement() override;

  uint64_t atomicNumber() const { return atomicNumber_; }
  bool haveNucleonNumber() const { return haveNucleonNumber_; }
  uint64_t nucleonNumber() const { return nucleonNumber_; }

  static QSharedPointer<ChemicalCompositionElement> parseImpl(
      const QJsonArray& array);
  QJsonArray toJson() const override;

  QString toString() const override;
  QString description() const override;
};

class VOXIECORESHARED_EXPORT ChemicalCompositionMolecule
    : public ChemicalComposition {
 public:
  class Member {
    double fraction_;
    QSharedPointer<ChemicalComposition> composition_;

   public:
    Member(double fraction,
           const QSharedPointer<ChemicalComposition>& composition);
    ~Member();

    double fraction() const { return fraction_; }
    const QSharedPointer<ChemicalComposition>& composition() const {
      return composition_;
    }

    static Member parse(const QJsonArray& array);
    QJsonArray toJson() const;
  };

 private:
  QList<Member> members_;

 public:
  ChemicalCompositionMolecule(const QList<Member>& members);
  ~ChemicalCompositionMolecule() override;

  const QList<Member>& members() const { return members_; }

  static QSharedPointer<ChemicalCompositionMolecule> parseImpl(
      const QJsonArray& array);
  QJsonArray toJson() const override;

  QString toString() const override;
  QString description() const override;
};

class VOXIECORESHARED_EXPORT ChemicalCompositionCompound
    : public ChemicalComposition {
 public:
  // TODO: Merge with ChemicalCompositionMolecule::Member?
  class Member {
    double fraction_;
    QSharedPointer<ChemicalComposition> composition_;

   public:
    Member(double fraction,
           const QSharedPointer<ChemicalComposition>& composition);
    ~Member();

    double fraction() const { return fraction_; }
    const QSharedPointer<ChemicalComposition>& composition() const {
      return composition_;
    }

    static Member parse(const QJsonArray& array);
    QJsonArray toJson() const;
  };

 private:
  QString typeString_;
  QList<Member> members_;

 public:
  ChemicalCompositionCompound(const QString& typeString,
                              const QList<Member>& members);
  ~ChemicalCompositionCompound() override;

  const QString& typeString() const { return typeString_; }
  const QList<Member>& members() const { return members_; }

  static QSharedPointer<ChemicalCompositionCompound> parseImpl(
      const QJsonArray& array);
  QJsonArray toJson() const override;

  QString toString() const override;
  QString description() const override;
};

class VOXIECORESHARED_EXPORT ChemicalCompositionMeta
    : public ChemicalComposition {
 private:
  QSharedPointer<ChemicalComposition> member_;
  QJsonObject information_;

 public:
  ChemicalCompositionMeta(const QSharedPointer<ChemicalComposition>& member,
                          const QJsonObject& information);
  ~ChemicalCompositionMeta() override;

  const QSharedPointer<ChemicalComposition>& member() const { return member_; }
  const QJsonObject& information() const { return information_; }

  static QSharedPointer<ChemicalCompositionMeta> parseImpl(
      const QJsonArray& array);
  QJsonArray toJson() const override;

  QString toString() const override;
  QString description() const override;
};
}  // namespace vx
