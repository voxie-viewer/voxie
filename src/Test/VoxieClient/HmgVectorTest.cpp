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

#include <VoxieClient/HmgVector.hpp>

#include <QtCore/QCoreApplication>

#include <QtTest/QtTest>

#include <sstream>

class HmgVectorTest : public QObject {
  Q_OBJECT

 public:
  HmgVectorTest() {}
  ~HmgVectorTest() {}

 private:
  vx::HmgVector<int, 2> foo() {
    return vx::createHmgVector(vx::Vector<int, 3>({2, 5, 1}));
  }

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
    vx::HmgVector<double, 2> vector1;
    (void)vector1;

    vx::HmgVector<int, 3> vector2 = {{1, 3, 3}, 9};
    vx::Vector<int, 4> vector2_ = {1, 3, 3, 9};
    auto vector2_2 = vx::createHmgVector(vector2_);
    QVERIFY(vector2.hmgVectorData() == vector2_2.hmgVectorData());

    // Should fail to compile
    // vx::HmgVector<int, 3> vector3 = {{1, 3, 3.0}, 4};
    // vx::HmgVector<int, 3> vector3_1 = {{1, 3, 3}, 4.0};

    std::array<int, 4> array1 = {1, 3, 3, 22};
    vx::HmgVector<int, 3> vector4(vx::createHmgVector(vx::toVector(array1)));

    vx::HmgVector<double, 3> vector5 = vx::vectorCastNarrow<double>(vector2);

    vx::HmgVector<int, 0> vector6;

    Q_UNUSED(vector6);

    QVERIFY(vector5.hmgVectorData() == (+vector5).hmgVectorData());
    vx::HmgVector<double, 3> vector5m = {{-1, -3, -3}, 9};
    QVERIFY(vector5m.hmgVectorData() == (-vector5).hmgVectorData());

    // qDebug() << toQString(vector4);
    QVERIFY(toQString(vector4) == "(1, 3, 3 | 22)");

    // qDebug() << toQString(
    //     normalizeSpherically(vx::vectorCastNarrow<double>(vector4)));

    auto vector7 = toVectorFinite(vector5);
    // qDebug() << vector7;
    Q_UNUSED(vector7);
  }

  void testConv() {
    std::array<int, 3> array1 = {1, 2};
    vx::HmgVector<int, 3> vector1 = {{1, 2, 0}, 1};
    QVERIFY(vx::toHmgVector(vx::toVector(array1)).hmgVectorData() ==
            vector1.hmgVectorData());

    vx::HmgVector<double, 3> vector2 = {{1, 2, 0}, 1};
    QVERIFY(vx::vectorCastNarrow<double>(vector1).hmgVectorData() ==
            vector2.hmgVectorData());
  }
};

int execHmgVectorTest(int argc, char** argv);
int execHmgVectorTest(int argc, char** argv) {
  HmgVectorTest test;
  return QTest::qExec(&test, argc, argv);
}

// Instantiate some templates
template class vx::HmgVector<int, 3>;
template class vx::HmgVector<int, 0>;

#include "HmgVectorTest.moc"
