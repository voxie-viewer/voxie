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

#include <VoxieClient/DBusUtil.hpp>
#include <VoxieClient/DBusValue.hpp>
#include <VoxieClient/ToString.hpp>

#include <QtCore/QCoreApplication>

#include <QtTest/QtTest>

#include <sstream>

class DBusTest : public QObject {
  Q_OBJECT

 public:
  DBusTest() {}
  ~DBusTest() {}

 private Q_SLOTS:
  void initTestCase() {}
  void cleanupTestCase() {}

  void test1() {
    // Note: A QDBusArgument is either used for marshalling or for demarshalling
    // and cannot do both
    QDBusArgument arg0;
    arg0 << "23";
    qDebug() << arg0.currentType();
    QDBusVariant var0(QVariant::fromValue(arg0));
    QDBusArgument arg1 = var0.variant().value<QDBusArgument>();
    qDebug() << arg0.currentType() << arg1.currentType();

    qint64 val1 = 44;
    QDBusVariant var1 = vx::dbusMakeVariant<qint64>(val1);
    qDebug() << var1.variant();
    // Does not work
    // QVERIFY(vx::dbusGetVariantValue<qint64>(var1) == val1);
  }

  void testDBusValue() {
    auto val1 = vx::DBusValue::createBasic<uint8_t>(44);
    qDebug() << val1.signature().signature();
    qDebug() << val1.getBasicValue<uint8_t>();
    qDebug() << vx::toString(val1);
  }
};

int execDBusTest(int argc, char** argv);
int execDBusTest(int argc, char** argv) {
  DBusTest test;
  return QTest::qExec(&test, argc, argv);
}

#include "DBusTest.moc"
