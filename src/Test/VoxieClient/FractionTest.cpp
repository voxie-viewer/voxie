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

#include <VoxieClient/Fraction.hpp>

#include <QtCore/QCoreApplication>

#include <QtTest/QtTest>

#include <sstream>

class FractionTest : public QObject {
  Q_OBJECT

 public:
  FractionTest() {}
  ~FractionTest() {}

 private:
  template <typename T>
  static QString toQString(const T& t) {
    std::stringstream out;
    out << t;
    return QString::fromStdString(out.str());
  }

 private Q_SLOTS:
  void initTestCase() {}
  void cleanupTestCase() {}

  void test1() {
    vx::Fraction<double> fraction1;
    (void)fraction1;

    vx::Fraction<int> fraction2 = {3, 4};
    // qDebug() << fraction2;
    QVERIFY(toQString(fraction2) == "3 | 4");
    vx::Fraction<int> fraction2_2 = vx::toFraction(3) / vx::toFraction(4);
    QVERIFY(fraction2.asHmgVector().hmgVectorData() ==
            fraction2_2.asHmgVector().hmgVectorData());

    vx::Fraction<double> fraction3 = vx::fractionCastNarrow<double>(fraction2);
    vx::Fraction<int> fraction4 = vx::fractionCastNarrow<int>(fraction3);
    QVERIFY(fraction2.asHmgVector().hmgVectorData() ==
            fraction4.asHmgVector().hmgVectorData());

    QVERIFY(toRealNumber(fraction3) == 0.75);

    vx::Fraction<double> fraction5 = normalizeSpherically(fraction3);
    vx::Fraction<double> fraction5_ref = {0.6, 0.8};
    QVERIFY(squaredNorm(fraction5.asHmgVector().hmgVectorData() -
                        fraction5_ref.asHmgVector().hmgVectorData()) < 1e-10);
  }
};

int execFractionTest(int argc, char** argv);
int execFractionTest(int argc, char** argv) {
  FractionTest test;
  return QTest::qExec(&test, argc, argv);
}

// Instantiate some templates
template class vx::Fraction<int>;
template class vx::Fraction<double>;

#include "FractionTest.moc"
