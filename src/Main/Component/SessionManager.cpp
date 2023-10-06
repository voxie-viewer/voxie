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

#include "SessionManager.hpp"

#include <Voxie/Node/NodeGroup.hpp>
#include <Voxie/Util.hpp>

#include <Main/Root.hpp>

#include <Main/Gui/CoreWindow.hpp>

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QIODevice>
#include <QtCore/QTextStream>

#include <QtWidgets/QMessageBox>

using namespace vx;
using namespace vx::gui;
using namespace vx::io;

SessionManager::SessionManager() {}

// TODO: When the saveSession() script output some text this should be displayed
// (might be warnings)

// TODO: Make SessionManager inherit from QObject and use this instead of
// Root::instance()?

/**
 * @brief Collects all objects that are currently loaded and saves them into a
 * script file
 */
void SessionManager::saveSession(QString filename) {
  // TODO: Should this be done in C++ instead of in python?

  QString escapedPythonLibDirs = escapePythonStringArray(
      vx::Root::instance()->directoryManager()->allPythonLibDirs());
  QString code =
      "import sys; sys.path = " + escapedPythonLibDirs + " + sys.path; " +
      "import voxie, voxie.serialize_state; args = "
      "voxie.parser.parse_args(); "
      "context = voxie.VoxieContext(args); instance = "
      "context.createInstance(); voxie.serialize_state.serialize(instance, "
      "open(" +
      vx::escapePythonString(filename) + ", 'w')); context.client.destroy()";

  QStringList args;
  args << "-c";
  args << code;
  auto output = createQSharedPointer<QString>();
  auto process = Root::instance()->scriptLauncher()->startScript(
      Root::instance()->directoryManager()->pythonExecutable(), nullptr, args,
      new QProcess(), output);

  // TODO: Move parts to ScriptLauncher?
  // TODO: This should be done before the process is started
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
  QObject::connect(process, &QProcess::errorOccurred, Root::instance(),
                   [](QProcess::ProcessError error) {
                     QMessageBox(
                         QMessageBox::Critical, "Error while saving project",
                         QString() + "Error while saving project: " +
                             QVariant::fromValue(error).toString(),
                         QMessageBox::Ok, Root::instance()->mainWindow())
                         .exec();
                   });
#endif
  QObject::connect(
      process,
      static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(
          &QProcess::finished),
      Root::instance(),
      [output](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitStatus != QProcess::NormalExit || exitCode != 0) {
          QString scriptOutputString = *output;
          if (scriptOutputString != "")
            scriptOutputString = "\n\nScript output:\n" + scriptOutputString;
          QMessageBox(QMessageBox::Critical, "Error while saving project",
                      QString() + "Error while saving project: " +
                          QVariant::fromValue(exitStatus).toString() +
                          ", code = " + QString::number(exitCode) +
                          scriptOutputString,
                      QMessageBox::Ok, Root::instance()->mainWindow())
              .exec();
        } else {
          QString scriptOutputString = *output;
          if (scriptOutputString != "") {
            QMessageBox(QMessageBox::Warning, "Warnings while saving project",
                        QString() + "Warnings while saving project:\n" +
                            scriptOutputString,
                        QMessageBox::Ok, Root::instance()->mainWindow())
                .exec();
          }
        }
      });

  // Wait until the process has exited (needed esp. when saving on exit)
  // process->waitForFinished(-1); // does not work because DBus calls would not
  // be handled here
  QEventLoop loop;
  QObject::connect(process, &QObject::destroyed, &loop,
                   [&loop]() { loop.exit(); });
  // qDebug() << "Waiting for save";
  loop.exec();
  qDebug() << "Save operation finished";
}

void SessionManager::saveNodeGroup(NodeGroup* nodeGroup, QString filename) {
  // TODO: Should this be done in C++ instead of in python?

  QString escapedPythonLibDirs = escapePythonStringArray(
      vx::Root::instance()->directoryManager()->allPythonLibDirs());
  QString code =
      "import sys; sys.path = " + escapedPythonLibDirs + " + sys.path; " +
      "import voxie, voxie.serialize_state; args = "
      "voxie.parser.parse_args(); "
      "context = voxie.VoxieContext(args); instance = "
      "context.createInstance(); voxie.serialize_state.serialize(instance, "
      "open(" +
      vx::escapePythonString(filename) + ", 'w'), dbus.ObjectPath(\"" +
      nodeGroup->getPath().path() + "\")); context.client.destroy()";

  QStringList args;
  args << "-c";
  args << code;
  auto output = createQSharedPointer<QString>();
  auto process = Root::instance()->scriptLauncher()->startScript(
      Root::instance()->directoryManager()->pythonExecutable(), nullptr, args,
      new QProcess(), output);

  // TODO: Move parts to ScriptLauncher?
  // TODO: This should be done before the process is started
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
  QObject::connect(process, &QProcess::errorOccurred, Root::instance(),
                   [](QProcess::ProcessError error) {
                     QMessageBox(
                         QMessageBox::Critical, "Error while saving node group",
                         QString() + "Error while saving node group: " +
                             QVariant::fromValue(error).toString(),
                         QMessageBox::Ok, Root::instance()->mainWindow())
                         .exec();
                   });
#endif
  QObject::connect(
      process,
      static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(
          &QProcess::finished),
      Root::instance(),
      [output](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitStatus != QProcess::NormalExit || exitCode != 0) {
          QString scriptOutputString = *output;
          if (scriptOutputString != "")
            scriptOutputString = "\n\nScript output:\n" + scriptOutputString;
          QMessageBox(QMessageBox::Critical, "Error while saving project",
                      QString() + "Error while saving project: " +
                          QVariant::fromValue(exitStatus).toString() +
                          ", code = " + QString::number(exitCode) +
                          scriptOutputString,
                      QMessageBox::Ok, Root::instance()->mainWindow())
              .exec();
        } else {
          QString scriptOutputString = *output;
          if (scriptOutputString != "") {
            QMessageBox(QMessageBox::Warning, "Warnings while saving project",
                        QString() + "Warnings while saving project:\n" +
                            scriptOutputString,
                        QMessageBox::Ok, Root::instance()->mainWindow())
                .exec();
          }
        }
      });

  // Wait until the process has exited (needed esp. when saving on exit)
  // process->waitForFinished(-1); // does not work because DBus calls would not
  // be handled here
  QEventLoop loop;
  QObject::connect(process, &QObject::destroyed, &loop,
                   [&loop]() { loop.exit(); });
  // qDebug() << "Waiting for save";
  loop.exec();
  qDebug() << "Saving node group finished";
}

/**
 * @brief Loads a previously saved session from the selected script file
 * @param filename
 */
void SessionManager::loadSession(QString filename) {
  auto output = createQSharedPointer<QString>();
  auto process = Root::instance()->scriptLauncher()->startScript(
      filename, &Root::instance()->directoryManager()->pythonExecutable(),
      QStringList(), new QProcess(), output);

  // TODO: Move parts to ScriptLauncher?
  // TODO: This should be done before the process is started
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
  QObject::connect(process, &QProcess::errorOccurred, Root::instance(),
                   [](QProcess::ProcessError error) {
                     QMessageBox(QMessageBox::Critical,
                                 "Error while loading project",
                                 QString() + "Error while loading project: " +
                                     QVariant::fromValue(error).toString(),
                                 QMessageBox::Ok)
                         .exec();
                   });
#endif
  QObject::connect(
      process,
      static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(
          &QProcess::finished),
      Root::instance(),
      [output](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitStatus != QProcess::NormalExit || exitCode != 0) {
          QString scriptOutputString = *output;
          if (scriptOutputString != "")
            scriptOutputString = "\n\nScript output:\n" + scriptOutputString;
          QMessageBox(QMessageBox::Critical, "Error while loading project",
                      QString() + "Error while loading project: " +
                          QVariant::fromValue(exitStatus).toString() +
                          ", code = " + QString::number(exitCode) +
                          scriptOutputString,
                      QMessageBox::Ok)
              .exec();
        } else {
          QString scriptOutputString = *output;
          if (scriptOutputString != "") {
            QMessageBox(QMessageBox::Warning, "Warnings while loading project",
                        QString() + "Warnings while loading project:\n" +
                            scriptOutputString,
                        QMessageBox::Ok)
                .exec();
          }
        }
      });
}
