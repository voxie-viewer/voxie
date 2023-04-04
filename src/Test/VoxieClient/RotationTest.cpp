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

#include <VoxieClient/Rotation.hpp>

#include <Voxie/MathQt.hpp>

#include <QtCore/QCoreApplication>

#include <QtTest/QtTest>

class RotationTest : public QObject {
  Q_OBJECT

 public:
  RotationTest() {}
  ~RotationTest() {}

 private Q_SLOTS:
  void initTestCase() {}
  void cleanupTestCase() {}

  void test1() {
    auto rot1 = vx::identityRotation<double>();
    // qDebug() << rot1;
    // vx::Rotation<double, 3> rot;
    QVERIFY((rot1.asQuaternion() == vx::Quaternion<double>{1, 0, 0, 0}));
    QVERIFY(std::abs(angularDifference(rot1, rot1)) < 1e-20);

    double angle2 = 1.023;
    vx::Vector<double, 3> v0 = {1, 2, 3};
    auto rot2 = rotationFromAxisAngle(v0, angle2);
    // qDebug() << rot2;
    QVERIFY(std::abs(rot2.angle() - angle2) < 1e-10);
    QVERIFY(std::abs(angularDifference(rot2, rot2)) < 1e-10);
    QVERIFY(std::abs(angularDifference(rot1, rot2) - angle2) < 1e-10);
    QVERIFY(std::abs(angularDifference(rot2, rot1) - angle2) < 1e-10);

    QVERIFY(angularDifference(rot2, rot1 * rot2) < 1e-10);
    QVERIFY(angularDifference(rot2, rot2 * rot1) < 1e-10);
    QVERIFY(angularDifference(rot2, rot1 * rot2 * rot1) < 1e-10);

    auto rot3 = rot2 * rot2;
    QVERIFY(std::abs(rot3.angle() - 2 * angle2) < 1e-10);
    auto rot4 = rot3;
    rot4 *= rot2;
    QVERIFY(std::abs(rot4.angle() - 3 * angle2) < 1e-10);

    vx::LinearMap<double, 3> map2 = rot2;
    vx::AffineMap<double, 3> map3 = rot3;
    vx::ProjectiveMap<double, 3> map4 = rot4;
    vx::Vector<double, 3> v1 = {12.5, -51.2, 41.5};
    vx::Vector<double, 3> v2 = {-0.41, 41.2, 4.5};
    // qDebug() << rot2.map(v1) << map2.map(v1);
    // qDebug() << squaredNorm(rot2.map(v1) - map2.map(v1));
    QVERIFY(squaredNorm(rot2.map(v1) - map2.map(v1)) < 1e-20);
    QVERIFY(squaredNorm(rot3.map(v1) - map3.map(v1)) < 1e-20);
    QVERIFY(squaredNorm(rot4.map(v1) - map4.map(v1)) < 1e-20);
    QVERIFY(squaredNorm(rot2.map(v2) - map2.map(v2)) < 1e-20);
    QVERIFY(squaredNorm(rot3.map(v2) - map3.map(v2)) < 1e-20);
    QVERIFY(squaredNorm(rot4.map(v2) - map4.map(v2)) < 1e-20);

    auto unused = map2 * rot2;
    unused = rot2 * map2;
    auto unused2 = map3 * rot2;
    unused2 = rot2 * map3;
    auto unused3 = map4 * rot2;
    unused3 = rot2 * map4;
    (void)unused;
    (void)unused2;
    (void)unused3;
    map2 *= rot2;
    map3 *= rot2;
    map4 *= rot2;

    float angleDeg = 15;
    auto v3 = vx::vectorCastNarrow<float>(v0);
    auto rot_vx = rotationFromAxisAngleDeg(v3, angleDeg);
    auto rot_qt =
        vx::toRotation(QQuaternion::fromAxisAndAngle(toQVector(v3), angleDeg));
    // TODO: What should the accuracy here be?
    // qDebug() << angularDifference(rot_vx, rot_qt);
    QVERIFY(angularDifference(rot_vx, rot_qt) < 1e-4);

    vx::Vector<float, 3> vectors[] = {
        {0, 0, 0},           {0, 0, 3e-23},
        {5e-23, 0, 3e-23},   {3e-23, 0, 3e-23},
        {0, 0, 5e-23},       {0, 0, 1e-22},
        {0, 2.5e-22, 1e-22}, {1.2e-22, 2.5e-22, 1e-22},
        {3, 4, 5},
    };
    for (const auto& vec : vectors) {
      auto rot_0 = rotationFromAxisAngle(vec, 0.0f);
      auto rot_1 = rotationFromAxisAngle(vec, 1.0f);
      // qDebug() << vec << rot_0 << rot_1;
      QVERIFY(!std::isnan(rot_0.asQuaternion().a()));
      QVERIFY(!std::isnan(rot_0.asQuaternion().b()));
      QVERIFY(!std::isnan(rot_0.asQuaternion().c()));
      QVERIFY(!std::isnan(rot_0.asQuaternion().d()));
      QVERIFY(!std::isnan(rot_1.asQuaternion().a()));
      QVERIFY(!std::isnan(rot_1.asQuaternion().b()));
      QVERIFY(!std::isnan(rot_1.asQuaternion().c()));
      QVERIFY(!std::isnan(rot_1.asQuaternion().d()));
      QVERIFY(rot_0.asQuaternion() ==
              (vx::Quaternion<float>{1.0f, 0.0f, 0.0f, 0.0f}));
    }
  }
};

int execRotationTest(int argc, char** argv);
int execRotationTest(int argc, char** argv) {
  RotationTest test;
  return QTest::qExec(&test, argc, argv);
}

// Instantiate some templates
template class vx::Rotation<double, 3>;
template class vx::Rotation<float, 3>;

#include "RotationTest.moc"
