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

#include <VoxieClient/Array.hpp>
#include <VoxieClient/DBusClient.hpp>
#include <VoxieClient/DBusProxies.hpp>
#include <VoxieClient/VoxieDBus.hpp>

#include <QtCore/QCommandLineParser>
#include <QtCore/QDebug>
#include <QtCore/QString>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusError>
#include <QtDBus/QDBusPendingReply>

#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>

#include <QtGui/QWindow>

typedef float Voxel;

struct Error {};

WId voxieWindowID = 0;
static void setTransientParent(QWidget* widget) {
  // Does not seem to work on MacOS (QWindow::fromWinId(voxieWindowID) crashes)
  Q_UNUSED(widget);
#ifndef Q_OS_MACOS
  auto voxieWindow =
      voxieWindowID ? QWindow::fromWinId(voxieWindowID) : nullptr;
  widget->winId();  // Trigger creation of native window
  auto window = widget->windowHandle();
  if (voxieWindow && window) window->setTransientParent(voxieWindow);
#endif
}

static void error(const QString& str) {
  QTextStream(stderr) << str << endl << flush;

  auto box = new QMessageBox(QMessageBox::Critical, "ScriptGetAverage", str);
  setTransientParent(box);
  box->setText(str);
  box->exec();

  throw Error();
}

static void msg(const QString& str) {
  QTextStream(stderr) << str << endl << flush;

  auto box = new QMessageBox(QMessageBox::Information, "ScriptGetAverage", str);
  setTransientParent(box);
  box->setText(str);
  box->exec();
}

int main(int argc, char* argv[]) {
  try {
    if (argc < 1) error("argc is smaller than 1");

    QCommandLineParser parser;
    parser.setApplicationDescription("Voxie script for getting the average");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addOptions(vx::DBusClient::options());

    QStringList args;
    for (char** arg = argv; *arg; arg++) args.push_back(*arg);
    int argc0 = 1;
    char* args0[2] = {argv[0], NULL};
    // QCoreApplication app(argc0, args0);
    QApplication app(argc0, args0);
    parser.process(args);

    vx::initDBusTypes();

    vx::DBusClient dbusClient(parser);

    QDBusObjectPath guiPath = dbusClient->gui();
    if (dbusClient.instance()->lastError().isValid())
      error("Error while getting gui path: " +
            dbusClient.instance()->lastError().name() + ": " +
            dbusClient.instance()->lastError().message());
    if (guiPath == QDBusObjectPath("/")) error("No remote GUI object found");
    de::uni_stuttgart::Voxie::Gui gui(dbusClient.uniqueName(), guiPath.path(),
                                      dbusClient.connection());
    if (!gui.isValid())
      error("Error while getting gui object: " + gui.lastError().name() + ": " +
            gui.lastError().message());

    voxieWindowID =
        HANDLEDBUSPENDINGREPLY(gui.GetMainWindowID(vx::emptyOptions()));

    QList<QDBusObjectPath> selectedObjectPaths = gui.selectedObjects();
    if (gui.lastError().isValid())
      error("Error while getting selectedObjects paths: " +
            gui.lastError().name() + ": " + gui.lastError().message());
    if (selectedObjectPaths.size() == 0) error("No object selected");
    QDBusObjectPath selectedObjectPath = selectedObjectPaths[0];

    de::uni_stuttgart::Voxie::DataObject dataSet(dbusClient.uniqueName(),
                                                 selectedObjectPath.path(),
                                                 dbusClient.connection());
    if (!dataSet.isValid())
      error("Error while getting dataSet object: " +
            dataSet.lastError().name() + ": " + dataSet.lastError().message());

    QDBusObjectPath voxelDataPath = dataSet.data();
    if (dataSet.lastError().isValid())
      error("Error while getting voxelData path: " +
            dataSet.lastError().name() + ": " + dataSet.lastError().message());
    de::uni_stuttgart::Voxie::VolumeDataVoxel voxelData(
        dbusClient.uniqueName(), voxelDataPath.path(), dbusClient.connection());
    if (!voxelData.isValid())
      error("Error while getting voxelData object: " +
            voxelData.lastError().name() + ": " +
            voxelData.lastError().message());

    vx::Array3<const Voxel> array(HANDLEDBUSPENDINGREPLY(
        voxelData.GetDataReadonly(QMap<QString, QDBusVariant>())));

    size_t count = array.size<0>() * array.size<1>() * array.size<2>();

    double sum = 0;
    for (size_t z = 0; z < array.size<2>(); z++)
      for (size_t y = 0; y < array.size<1>(); y++)
        for (size_t x = 0; x < array.size<0>(); x++) sum += array(x, y, z);
    double avg = sum / count;

    msg(QString("The average of %1 values is %2").arg(count).arg(avg));

    return 0;
  } catch (Error& error) {
    return 1;
  }
}
