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

#include <VoxieClient/Matrix.hpp>

#include <QtCore/QCoreApplication>

#include <QtTest/QtTest>

#include <sstream>

class MatrixTest : public QObject {
  Q_OBJECT

 public:
  MatrixTest() {}
  ~MatrixTest() {}

 private Q_SLOTS:
  void initTestCase() {}
  void cleanupTestCase() {}

  void test1() {
    vx::Matrix<double, 2, 3> m1 = {
        {1, 2, 3.4},
        {3, 2, 1.2},
    };

    (void)m1;
    // qDebug() << m1;

    vx::Matrix<int, 2, 2> left = {
        {1, 2},
        {3, 4},
    };
    vx::Matrix<int, 2, 2> right = {
        {5, 6},
        {7, 8},
    };
    vx::Matrix<int, 2, 2> result = {
        {19, 22},
        {43, 50},
    };
    // qDebug() << left * right;
    QVERIFY(left * right == result);

    vx::Matrix<int, 2, 3> left2 = {
        {1, 2, 3},
        {9, 8, 7},
    };
    vx::Matrix<int, 3, 4> right2 = {
        {4, 5, 6, 7},
        {8, 9, 10, 11},
        {12, 13, 14, 15},
    };
    vx::Matrix<int, 2, 4> result2 = {
        {56, 62, 68, 74},
        {184, 208, 232, 256},
    };
    // qDebug() << left2 * right2;
    QVERIFY(left2 * right2 == result2);
    vx::Vector<int, 4> v2 = {9, 5, 1, 4};
    vx::Vector<int, 2> r2 = {1178, 3952};
    // qDebug() << );
    QVERIFY(result2 * v2 == r2);

    vx::Matrix<int, 4, 1> m41 = {{1}, {2}, {3}, {4}};
    vx::Matrix<int, 4, 1> m41_2 = {1, 2, 3, 4};
    auto m41_3 = vx::Matrix<int, 4, 1>({1, 2, 3, 4});
    QVERIFY(m41 == m41_2);
    QVERIFY(m41 == m41_3);
    vx::Matrix<int, 1, 4> m14 = {{1, 2, 3, 4}};
    QVERIFY(m14 == m41.transposed());

    vx::Matrix<int, 1, 1> m11_1 = {{77}};
    vx::Matrix<int, 1, 1> m11_2 = {77};
    auto m11_3 = (vx::Matrix<int, 1, 1>)77;
    // auto m11_4 = vx::Matrix<int, 1, 1>({77}); // Doesn't seem to work
    QVERIFY(m11_1 == m11_2);
    QVERIFY(m11_1 == m11_3);

    vx::Matrix<int, 2, 5> mzero = vx::Matrix<int, 2, 5>::zero();
    // qDebug() << mzero;
    for (std::size_t row = 0; row < 2; row++)
      for (std::size_t col = 0; col < 5; col++) QVERIFY(mzero(row, col) == 0);

    vx::Matrix<int, 3, 3> mid = {
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1},
    };
    auto midentity = vx::identityMatrix<int, 3>();
    // qDebug() << midentity;
    QVERIFY(midentity == mid);
  }

  void testDet() {
    vx::Matrix<int, 2> m1 = {
        {1, 2},
        {3, 4},
    };
    vx::Matrix<int, 3> m2 = {
        {1, 2, 1},
        {5, 6, 7},
        {9, 8, 7},
    };
    vx::Matrix<int, 4> m3 = {
        {8, 1, 2, 4},
        {6, 8, 2, 4},
        {2, 7, 0, 5},
        {5, 5, 4, 0},
    };
    QVERIFY(determinant(m1) == -2);
    QVERIFY(determinant(m2) == 28);
    QVERIFY(determinant(m3) == -262);
    // qDebug() << determinant(m1);
    // qDebug() << determinant(m2);
    // qDebug() << determinant(m3);
    // QVERIFY(determinant(vx::Matrix<int, 1>({55})) == 55);

    auto m3D = vx::matrixCastNarrow<double>(m3);
    double det;
    auto m3DI = inverse(m3D, &det);
    // qDebug() << m3DI;
    auto m3DI_err1 = m3D * m3DI - vx::identityMatrix<double, 4>();
    auto m3DI_err2 = m3DI * m3D - vx::identityMatrix<double, 4>();
    // qDebug() << m3DI_err1;
    // qDebug() << m3DI_err2;
    // qDebug() << det << squaredNorm(m3DI_err1) << squaredNorm(m3DI_err2);
    QVERIFY(squaredNorm(m3DI_err1) < 1e-20);
    QVERIFY(squaredNorm(m3DI_err2) < 1e-20);
  }
};

// See class MatrixBase
void msvcTest1(const vx::Matrix<quint64, 3, 4>& value);
void msvcTest1(const vx::Matrix<quint64, 3, 4>& value) {
  auto lambda = []() {
    vx::Matrix<quint64, 3, 4> mat;
    mat(0, 1) = 0;
  };
  lambda();
  (void)value(0, 2);
}

int execMatrixTest(int argc, char** argv);
int execMatrixTest(int argc, char** argv) {
  MatrixTest test;
  return QTest::qExec(&test, argc, argv);
}

// Instantiate some templates
template class vx::Matrix<int, 3, 3>;
template class vx::Matrix<int, 3, 4>;
template class vx::Matrix<int, 3, 0>;
template class vx::Matrix<int, 0, 2>;
template class vx::Matrix<int, 0, 0>;

#include "MatrixTest.moc"
