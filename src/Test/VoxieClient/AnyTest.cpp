/*
 * Copyright (c) 2014-2025 The Voxie Authors
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

#include <VoxieClient/Any.hpp>
#include <VoxieClient/Exception.hpp>

#include <QtCore/QCoreApplication>

#include <QtTest/QtTest>

class ThrowOnCopy {
 public:
  ThrowOnCopy() {}
  ThrowOnCopy(ThrowOnCopy&&) {}

  // Note: This constructor is needed when the vx::Any is copied
  // ThrowOnCopy(const ThrowOnCopy&) = delete;
  ThrowOnCopy(const ThrowOnCopy&) { throw 0; }
};

class AnyTest : public QObject {
  Q_OBJECT

 public:
  AnyTest() {}
  ~AnyTest() {}

 private Q_SLOTS:
  void initTestCase() {}
  void cleanupTestCase() {}

  void test1() {
    vx::Any anyEmpty;
    QCOMPARE(anyEmpty.has_value(), false);
    QCOMPARE(anyEmpty.type(), typeid(void));

    // vx::Any anyI = 4;
    vx::Any anyI = vx::make_any<int>(4);
    QVERIFY_EXCEPTION_THROWN(vx::any_cast<std::string>(anyI), vx::Exception);
    QCOMPARE(vx::any_cast<int>(anyI), 4);
    QCOMPARE(anyI.has_value(), true);
    QCOMPARE(anyI.type(), typeid(int));

    ThrowOnCopy toc;
    // vx::Any anyTOC = std::move(toc);
    vx::Any anyTOC = vx::make_any<ThrowOnCopy>(std::move(toc));
    QVERIFY_EXCEPTION_THROWN(vx::any_cast<std::string>(anyTOC), vx::Exception);
    vx::any_cast<ThrowOnCopy>(anyTOC);
    QCOMPARE(anyTOC.has_value(), true);
    QCOMPARE(anyTOC.type(), typeid(ThrowOnCopy));

    QVERIFY_EXCEPTION_THROWN(anyEmpty = anyTOC, int);

    anyEmpty = anyI;
    anyTOC = anyI;
    QCOMPARE(anyEmpty.has_value(), true);
    QCOMPARE(anyTOC.has_value(), true);
    QCOMPARE(anyEmpty.type(), typeid(int));
    QCOMPARE(anyTOC.type(), typeid(int));
    QVERIFY(&vx::any_cast<int>(anyI) != &vx::any_cast<int>(anyTOC));
  }
};

int execAnyTest(int argc, char** argv);
int execAnyTest(int argc, char** argv) {
  AnyTest test;
  return QTest::qExec(&test, argc, argv);
}

#include "AnyTest.moc"
