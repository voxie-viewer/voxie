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

#include <VoxieClient/Vector.hpp>

#include <QtCore/QCoreApplication>

#include <QtTest/QtTest>

#include <sstream>

class VectorTest : public QObject {
  Q_OBJECT

 public:
  VectorTest() {}
  ~VectorTest() {}

 private:
  vx::Vector<int, 2> foo() { return {2, 5}; }

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
    vx::Vector<double, 2> vector1;
    (void)vector1;

    vx::Vector<int, 3> vector2 = {1, 3, 3};

    // vx::Vector<int, 3> vector3 = {1, 3, 3.0}; // Should fail to compile

    std::array<int, 3> array1 = {1, 3, 3};
    vx::Vector<int, 3> vector4(vx::toVector(array1));

    vx::Vector<double, 3> vector5 = vx::vectorCast<double>(vector2);

    vx::Vector<int, 0> vector6;

    Q_UNUSED(vector5);
    Q_UNUSED(vector6);

    vx::Vector<vx::Vector<double, 3>, 2> vector7 = {{3.4, 5.6, 1.2},
                                                    {3.4, 5.6, 0.2}};

    // qDebug() << toQString(vector7);
    QVERIFY(toQString(vector7) == "((3.4, 5.6, 1.2), (3.4, 5.6, 0.2))");
  }

  void testEqual() {
    vx::Vector<int, 2> vector1 = {1, 2};
    vx::Vector<int, 2> vector2 = {1, 3};
    vx::Vector<int, 2> vector3 = {2, 3};

    QVERIFY(vector1 == vector1);
    QVERIFY(vector2 == vector2);
    QVERIFY(vector3 == vector3);
    QVERIFY(!(vector1 != vector1));
    QVERIFY(!(vector2 != vector2));
    QVERIFY(!(vector3 != vector3));

    QVERIFY(vector1 != vector2);
    QVERIFY(vector1 != vector3);
    QVERIFY(vector2 != vector3);
    QVERIFY(!(vector1 == vector2));
    QVERIFY(!(vector1 == vector3));
    QVERIFY(!(vector2 == vector3));
  }

  void testArith() {
    vx::Vector<int, 2> vector1 = {1, 2};
    vx::Vector<int, 2> vector2 = {1, 3};
    vx::Vector<int, 2> vector3 = {2, 5};
    vx::Vector<int, 2> vector2n = {-1, -3};

    QVERIFY(vector1 + vector2 == vector3);
    QVERIFY(vector2 + vector1 == vector3);
    auto vector4 = vector1 += vector2;
    QVERIFY(vector4 == vector3);
    QVERIFY(vector1 == vector3);
    QVERIFY(vector2 != vector3);

    auto vector5 = vector1 -= vector2;
    QVERIFY(vector1 + vector2 == vector3);
    QVERIFY(vector5 == vector1);

    auto scalar6 = dotProduct(vector1, vector2);
    QVERIFY(scalar6 == 7);

    QVERIFY(+vector2 == vector2);
    QVERIFY(-vector2 == vector2n);

    vector2 *= -1;
    QVERIFY(vector2 == vector2n);
    QVERIFY(vector2 * (-1) == (-1) * vector2n);
    // QVERIFY(vector2 / (-1) == (-1) * vector2n);
    QVERIFY(vx::vectorCast<double>(vector2) / (-1) ==
            (-1) * vx::vectorCast<double>(vector2n));
  }

  void testConv() {
    std::array<int, 3> array1 = {1, 2};
    vx::Vector<int, 3> vector1 = {1, 2, 0};
    QVERIFY(vx::toVector(array1) == vector1);

    vx::Vector<double, 3> vector2 = {1, 2, 0};
    QVERIFY(vx::vectorCast<double>(vector1) == vector2);
  }

  void testCross() {
    vx::Vector<int, 3> v1 = {1, 0, 0};
    vx::Vector<int, 3> v2 = {0, 1, 0};
    vx::Vector<int, 3> v3 = {0, 0, 1};
    QVERIFY(crossProduct(v1, v2) == v3);
  }
};

int execVectorTest(int argc, char** argv);
int execVectorTest(int argc, char** argv) {
  VectorTest test;
  return QTest::qExec(&test, argc, argv);
}

// Instantiate some templates
template class vx::Vector<int, 3>;
template class vx::Vector<int, 0>;

#include "VectorTest.moc"
