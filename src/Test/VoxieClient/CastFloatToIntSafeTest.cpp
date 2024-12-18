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

#include <VoxieClient/CastFloatToIntSafe.hpp>
#include <VoxieClient/Format.hpp>

#include <QtCore/QCoreApplication>

#include <QtTest/QtTest>

#include <sstream>

class CastFloatToIntSafeTest : public QObject {
  Q_OBJECT

 public:
  CastFloatToIntSafeTest() {}
  ~CastFloatToIntSafeTest() {}

 private:
  template <typename T>
  static QString toQString(const T& t) {
    std::stringstream out;
    out << t;
    return QString::fromStdString(out.str());
  }

  template <typename IntType, typename FloatType>
  void checkValid(FloatType f) {
    vx::Optional<IntType> i_opt = vx::castFloatToIntSafe<IntType>(f);
    QVERIFY(i_opt.has_value());
    IntType i = i_opt.value();
    // qDebug() << toQString(i) << toQString(f);
    QVERIFY(i == std::trunc(f));
  }

  template <typename IntType, typename FloatType>
  void checkInvalid(FloatType f) {
    vx::Optional<IntType> i_opt = vx::castFloatToIntSafe<IntType>(f);
    QVERIFY(!i_opt.has_value());
  }

 private Q_SLOTS:
  void initTestCase() {}
  void cleanupTestCase() {}

  // TODO: Add more tests

  void testDouble64() {
    using Limits = vx::intern::CastFloatToIntSafeLimits<qint64, double>;
    // qDebug() << vx::format("{:.99g}", Limits::lowerLimitFun());
    // qDebug() << vx::format("{:.99g}", Limits::upperLimitFun());

    static_assert(Limits::lowerLimitFun() == -9223372036854775808.0, "");
    static_assert(Limits::upperLimitFun() == 9223372036854774784.0, "");

    double int64_min_double = -9223372036854775808.0;
    double int64_max_plus_double = 9223372036854775808.0;

    checkInvalid<qint64, double>(std::nexttoward(
        int64_min_double, -std::numeric_limits<double>::infinity()));
    checkValid<qint64, double>(int64_min_double);
    checkValid<qint64, double>(std::nexttoward(int64_min_double, 0));

    checkValid<qint64, double>(-1.0);
    checkValid<qint64, double>(0.0);
    checkValid<qint64, double>(1.0);

    checkValid<qint64, double>(std::nexttoward(int64_max_plus_double, 0));
    checkInvalid<qint64, double>(int64_max_plus_double);
  }

  void testDoubleU64() {
    using Limits = vx::intern::CastFloatToIntSafeLimits<quint64, double>;
    // qDebug() << vx::format("{:.99g}", Limits::lowerLimitFun());
    // qDebug() << vx::format("{:.99g}", Limits::upperLimitFun());

    static_assert(Limits::lowerLimitFun() == -9.9999999999999989e-1, "");
    static_assert(Limits::upperLimitFun() == 18446744073709549568.0, "");
    double uint64_min_double =
        -0.99999999999999988897769753748434595763683319091796875;
    double uint64_max_double = 18446744073709549568.0;
    double uint64_max_plus_double = 18446744073709551616.0;

    checkValid<quint64, double>(uint64_max_double);
    checkInvalid<quint64, double>(uint64_max_plus_double);
    checkInvalid<quint64, double>(-1.0);
    checkValid<quint64, double>(-0.999);
    checkValid<quint64, double>(uint64_min_double);
  }
};

/*
  Generated machine code can be checked with:

  objdump -d
  build/src/Test/VoxieClient/VoxieClientTest.p/CastFloatToIntSafeTest.cpp.o

  Probably -fsanitize=float-cast-overflow should be disabled.
*/

vx::Optional<std::int64_t> castToInt64D(double value);
vx::Optional<std::int64_t> castToInt64D(double value) {
  return vx::castFloatToIntSafe<std::int64_t>(value);
}
vx::Optional<std::int64_t> castToInt64F(float value);
vx::Optional<std::int64_t> castToInt64F(float value) {
  return vx::castFloatToIntSafe<std::int64_t>(value);
}

vx::Optional<std::int32_t> castToInt32D(double value);
vx::Optional<std::int32_t> castToInt32D(double value) {
  return vx::castFloatToIntSafe<std::int32_t>(value);
}
vx::Optional<std::int32_t> castToInt32F(float value);
vx::Optional<std::int32_t> castToInt32F(float value) {
  return vx::castFloatToIntSafe<std::int32_t>(value);
}

int execCastFloatToIntSafeTest(int argc, char** argv);
int execCastFloatToIntSafeTest(int argc, char** argv) {
  CastFloatToIntSafeTest test;
  return QTest::qExec(&test, argc, argv);
}

#include "CastFloatToIntSafeTest.moc"
