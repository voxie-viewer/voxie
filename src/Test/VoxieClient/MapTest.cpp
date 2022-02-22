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

#include <VoxieClient/Map.hpp>

#include <QtCore/QCoreApplication>

#include <QtTest/QtTest>

#include <sstream>

class MapTest : public QObject {
  Q_OBJECT

 public:
  MapTest() {}
  ~MapTest() {}

 private Q_SLOTS:
  void initTestCase() {}
  void cleanupTestCase() {}

  void test1() {
    vx::Matrix<int, 2> rotMatrix = {
        {0, 1},
        {-1, 0},
    };
    auto rotate = vx::createLinearMap(rotMatrix);

    vx::Vector<int, 2> v = {4, 5};
    vx::Vector<int, 2> vRot = {5, -4};
    QVERIFY(rotate.map(v) == vRot);

    // qDebug() << rotate;
    // qDebug() << rotate.linearMatrix();
    // qDebug() << rotate.affineMatrix();
    vx::Matrix<int, 2, 3> rotMatrixAff = {
        {0, 1, 0},
        {-1, 0, 0},
    };
    QVERIFY(rotate.linearMatrix() == rotMatrix);
    QVERIFY(rotate.affineMatrix() == rotMatrixAff);

    auto rotate2 = rotate * rotate;
    rotate2 *= rotate2;
    // qDebug() << rotate2;
    QVERIFY(rotate2 == (vx::identityMap<int, 2>()));

    vx::Matrix<int, 2> m1 = {
        {4, 2},
        {66, 43},
    };
    auto map1 = vx::createLinearMap(m1);
    // qDebug() << map1 << inverse(map1);
    auto map2 = vx::mapCast<double>(map1);
    auto map2i = inverse(map2);
    // qDebug() << map2 * map2i << map2i * map2;
    // qDebug() << squaredNorm((map2 * map2i).linearMatrix() -
    //                         vx::identityMatrix<double, 2>());
    QVERIFY(squaredNorm((map2 * map2i).linearMatrix() -
                        vx::identityMatrix<double, 2>()) < 1e-20);
    QVERIFY(squaredNorm((map2i * map2).linearMatrix() -
                        vx::identityMatrix<double, 2>()) < 1e-20);

    vx::AffineMap<int, 2> rotAff = rotate;
    // qDebug() << rotAff;
    QVERIFY(rotAff.affineMatrix() == rotMatrixAff);

    vx::Vector<int, 2> v1 = {4, 9};
    vx::Vector<int, 2> v2 = {9, -4};
    QVERIFY(rotate.map(v1) == v2);
    QVERIFY(rotate.mapVector(v1) == v2);
    QVERIFY(rotAff.map(v1) == v2);
    QVERIFY(rotAff.mapVector(v1) == v2);

    vx::Vector<int, 2> vTrans = {4, 5};
    auto trans = vx::createTranslation(vTrans);
    auto transRot = rotate * trans;
    auto rotTrans = trans * rotate;
    // qDebug() << trans;
    // qDebug() << trans * rotate;
    vx::Vector<int, 2> vt1 = {8, 14};
    vx::Vector<int, 2> vt2 = {14, -8};
    vx::Vector<int, 2> vt3 = {13, 1};
    // qDebug() << trans.map(v1);
    // qDebug() << transRot.map(v1);
    // qDebug() << rotTrans.map(v1);
    QVERIFY(trans.map(v1) == vt1);
    QVERIFY(transRot.map(v1) == vt2);
    QVERIFY(rotTrans.map(v1) == vt3);
    // qDebug() << trans.mapVector(v1);
    // qDebug() << transRot.mapVector(v1);
    // qDebug() << rotTrans.mapVector(v1);
    QVERIFY(trans.mapVector(v1) == v1);
    QVERIFY(transRot.mapVector(v1) == rotate.mapVector(v1));
    QVERIFY(rotTrans.mapVector(v1) == rotate.mapVector(v1));

    vx::Matrix<int, 3, 4> m3 = {
        {1, 5, 4, 4},
        {5, 4, 1, 8},
        {6, 1, 4, 2},
    };
    auto map3 = createAffineMap(m3);
    // qDebug() << inverse(map3);
    auto map4 = vx::mapCast<double>(map3);
    // qDebug() << inverse(map4);
    auto map4i = inverse(map4);
    /*
    qDebug() << (map4 * map4i).affineMatrix();
    qDebug() << squaredNorm((map4 * map4i).affineMatrix() -
                            concatColumns(vx::identityMatrix<double, 3>(),
                                          vx::Matrix<double, 3, 1>::zero()));
    */
    QVERIFY(squaredNorm((map4 * map4i).affineMatrix() -
                        concatColumns(vx::identityMatrix<double, 3>(),
                                      vx::Matrix<double, 3, 1>::zero())) <
            1e-20);
    QVERIFY(squaredNorm((map4i * map4).affineMatrix() -
                        concatColumns(vx::identityMatrix<double, 3>(),
                                      vx::Matrix<double, 3, 1>::zero())) <
            1e-20);
  }

  void testProj() {
    vx::Matrix<int, 3, 4> m1 = {
        {1, 2, 3, 4},
        {5, 6, 7, 8},
        {5, 1, 5, 2},
    };
    auto map1 = createProjectiveMap(m1);
    auto map2 = vx::mapCast<double>(map1);
    // qDebug() << map1 << map2;
    vx::Vector<int, 3> v1 = {5, 6, 7};
    auto v2 = vx::vectorCast<double>(v1);
    vx::Vector<double, 2> v3 = {0.61764706, 1.73529412};
    // qDebug() << map1.map(v1); // Should not work with integer values
    // qDebug() << map2.map(v2);
    // qDebug() << squaredNorm(map2.map(v2) - v3);
    QVERIFY(squaredNorm(map2.map(v2) - v3) < 1e-10);

    QVERIFY(map1 == map1);
    QVERIFY(map1 == (map1 * vx::identityMap<int, 3>()));
  }
};

int execMapTest(int argc, char** argv);
int execMapTest(int argc, char** argv) {
  MapTest test;
  return QTest::qExec(&test, argc, argv);
}

// Instantiate some templates
template class vx::LinearMap<int, 3, 3>;
template class vx::LinearMap<int, 3, 4>;
template class vx::LinearMap<int, 3, 0>;
template class vx::LinearMap<int, 0, 2>;
template class vx::LinearMap<int, 0, 0>;

template class vx::AffineMap<int, 3, 3>;
template class vx::AffineMap<int, 3, 4>;
template class vx::AffineMap<int, 3, 0>;
template class vx::AffineMap<int, 0, 2>;
template class vx::AffineMap<int, 0, 0>;

template class vx::ProjectiveMap<int, 3, 3>;
template class vx::ProjectiveMap<int, 3, 4>;
template class vx::ProjectiveMap<int, 3, 0>;
template class vx::ProjectiveMap<int, 0, 2>;
template class vx::ProjectiveMap<int, 0, 0>;

template class vx::LinearMap<double, 3, 3>;
template class vx::LinearMap<double, 3, 4>;
template class vx::LinearMap<double, 3, 0>;
template class vx::LinearMap<double, 0, 2>;
template class vx::LinearMap<double, 0, 0>;

template class vx::AffineMap<double, 3, 3>;
template class vx::AffineMap<double, 3, 4>;
template class vx::AffineMap<double, 3, 0>;
template class vx::AffineMap<double, 0, 2>;
template class vx::AffineMap<double, 0, 0>;

template class vx::ProjectiveMap<double, 3, 3>;
template class vx::ProjectiveMap<double, 3, 4>;
template class vx::ProjectiveMap<double, 3, 0>;
template class vx::ProjectiveMap<double, 0, 2>;
template class vx::ProjectiveMap<double, 0, 0>;

#include "MapTest.moc"
