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

// QDBusConnection should be included as early as possible:
// https://bugreports.qt.io/browse/QTBUG-48351
#include <QtDBus/QDBusConnection>

#include "ScriptLauncher.hpp"

#include <VoxieClient/ObjectExport/BusManager.hpp>

#include <VoxieBackend/Component/Extension.hpp>

#include <Voxie/Util.hpp>

#include <Main/DirectoryManager.hpp>
#include <Main/Root.hpp>

#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>

#include <QtCore/QFileInfo>

using namespace vx;

ScriptLauncher::ScriptLauncher(QObject* parent) : QObject(parent) {}
ScriptLauncher::~ScriptLauncher() {}

void ScriptLauncher::setupEnvironment(IDirectoryManager* directoryManager,
                                      QProcess* process, bool setPythonLibDir) {
  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

#if defined(Q_OS_WIN)
  // Add voxie directory (which contains e.g. the Qt dlls) to PATH
  QString path = directoryManager->baseDir();
  if (env.contains("PATH")) path += ";" + env.value("PATH");
  env.insert("PATH", path);
#endif

  if (setPythonLibDir) {
    QString pythonLibDir = directoryManager->pythonLibDir();

    // Add pythonLibDir to the Python search path
    QString pythonPath = pythonLibDir;
    for (const auto& dir : directoryManager->additionalPythonLibDirs())
      pythonPath += ":" + dir;
    if (env.contains("PYTHONPATH")) pythonPath += ":" + env.value("PYTHONPATH");
    env.insert("PYTHONPATH", pythonPath);

    // Set the current directory to pythonLibDir, needed for windows (where the
    // embedded version of python ignores PYTHONPATH, see
    // https://stackoverflow.com/questions/897792/where-is-pythons-sys-path-initialized-from/38403654#38403654)
    // Not needed because python is called with -c to set the PYTHONPATH
    // process->setWorkingDirectory(pythonLibDir);
  }

  process->setProcessEnvironment(env);
}
void ScriptLauncher::setupEnvironment(QProcess* process, bool setPythonLibDir) {
  setupEnvironment(vx::Root::instance()->directoryManager(), process,
                   setPythonLibDir);
}

// TODO: Handle output from scripts in more places?

// TODO: should this automatically use pythonExecutable when needed?
QProcess* ScriptLauncher::startScript(const QString& scriptFile,
                                      const QString* executable,
                                      const QStringList& arguments,
                                      QProcess* process,
                                      const QSharedPointer<QString>& output,
                                      bool setPythonLibDir) {
  // qDebug() << "ScriptLauncher::startScript" << scriptFile << executable
  //          << arguments << process << output << setPythonLibDir;

  currentScriptExecId++;
  quint64 id = currentScriptExecId;

  auto service = Root::instance()->mainDBusService();
  if (!service) {
    QMessageBox(
        QMessageBox::Critical, Root::instance()->mainWindow()->windowTitle(),
        QString("Cannot run external script because DBus is not available"),
        QMessageBox::Ok, Root::instance()->mainWindow())
        .exec();
    return nullptr;
  }

  QFileInfo fileInfo(scriptFile);

  QString pythonExecutable =
      vx::Root::instance()->directoryManager()->pythonExecutable();

  // Note: Callers should rely on this instead of setting pythonExecutable
  // themselves
  if (!executable && scriptFile.endsWith(".py")) executable = &pythonExecutable;

  QStringList args;
#if defined(Q_OS_WIN)
  // TODO: This is an ugly hack to detect whether python is being executed.
  // Probably this function should use pythonExecutable automatically.
  if (setPythonLibDir && executable && *executable == pythonExecutable) {
    // This does not work with python 3.7 anymore
    /*
    args.append("-m");
    args.append("voxie.run_with_pythonlib_helper");
    */

    // Still working with python 3.7 and does not require the current directory
    // to be changed
    args << "-c";

    QString escapedPythonLibDirs = escapePythonStringArray(
        vx::Root::instance()->directoryManager()->allPythonLibDirs());

    args << "import sys; sys.path = " + escapedPythonLibDirs + " + sys.path; " +
                "import voxie.run_with_pythonlib_helper; "
                "voxie.run_with_pythonlib_helper.main()";
  }
#endif
#if defined(Q_OS_MACOS)
  // Hack to make sure python scripts are executed using the builtin python on
  // MacOS (TODO: to this?)
  if (!executable && scriptFile.endsWith(".py")) executable = &pythonExecutable;
#endif
  if (executable) args.append(scriptFile);
  for (const auto& arg : arguments) args << arg;
  service->addArgumentsTo(args);
  setupEnvironment(process, setPythonLibDir);  // Set PYTHONPATH etc.
  process->setProcessChannelMode(QProcess::MergedChannels);
  if (executable)
    process->setProgram(*executable);
  else
    process->setProgram(scriptFile);
  process->setArguments(args);

  // qDebug() << "ScriptLauncher::startScript 2"
  //          << (executable ? *executable : scriptFile) << args
  //          << setPythonLibDir;

  auto isStarted = createQSharedPointer<bool>();
  connect<void (QProcess::*)(int, QProcess::ExitStatus),
          std::function<void(int, QProcess::ExitStatus)>>(
      process, &QProcess::finished, this,
      std::function<void(int, QProcess::ExitStatus)>(
          [process, id](int exitCode, QProcess::ExitStatus exitStatus) -> void {
            Root::instance()->log(
                QString("Script %1 finished with exit status %2 / exit code %3")
                    .arg(id)
                    .arg(exitStatusToString(exitStatus))
                    .arg(exitCode));
            process->deleteLater();
          }));
  /*
  connect(process, &QProcess::errorOccurred, this, [process,
  id](QProcess::ProcessError error) { Root::instance()->log(QString("Error
  occurred for script %1: %2").arg(id).arg(error)); process->deleteLater();
      });
  */
  connect(process, &QProcess::started, this,
          [isStarted]() { *isStarted = true; });
  connect(process, &QProcess::stateChanged, this,
          [process, id, isStarted](QProcess::ProcessState newState) {
            // Root::instance()->log(QString("State change occurred for script
            // %1: %2").arg(id).arg(newState));
            if (newState == QProcess::NotRunning && !*isStarted) {
              Root::instance()->log(
                  QString("Error occurred for script %1: %2")
                      .arg(id)
                      .arg(processErrorToString(process->error())));
              process->deleteLater();
            }
          });
  auto buffer = createQSharedPointer<QString>();
  connect(process, &QProcess::readyRead, this,
          [this, process, id, buffer, output]() -> void {
            this->printBufferToConsole(process, id, buffer, output);
          });
  connect(
      process, &QProcess::readChannelFinished, this,
      [this, process, id, buffer, output]() -> void {
        this->printBufferToConsole(process, id, buffer, output);
        if (*buffer != "") {
          Root::instance()->log(QString("Script %1: %2").arg(id).arg(*buffer));
          buffer->clear();
        }
      });
  process->start();
  // qDebug() << "started";

  Root::instance()->log(
      QString("Started execution of %1 with ID %2").arg(scriptFile).arg(id));

  return process;
}

void ScriptLauncher::printBufferToConsole(
    QProcess* process, int id, const QSharedPointer<QString>& buffer,
    const QSharedPointer<QString>& output) {
  int pos = 0;
  QString data = QString(process->readAll());
  if (output) *output += data;
  for (;;) {
    int index = data.indexOf('\n', pos);
    if (index == -1) break;
    Root::instance()->log(QString("Script %1: %2%3")
                              .arg(id)
                              .arg(*buffer)
                              .arg(data.mid(pos, index - pos)));
    buffer->clear();
    pos = index + 1;
  }
  *buffer += data.mid(pos);
}

QObject* ScriptLauncher::startDebug(
    const QString& program, const QStringList& args, QObject* callbackObject,
    const std::function<void()>& runNormallyCallback,
    const std::function<void()>& cancelCallback) {
  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  QStringList envVars;

  QString pythonLibDir = voxieRoot().directoryManager()->pythonLibDir();
  QString pythonPath = pythonLibDir;
  for (const auto& dir :
       voxieRoot().directoryManager()->additionalPythonLibDirs())
    pythonPath += ":" + dir;
  if (env.contains("PYTHONPATH")) pythonPath += ":" + env.value("PYTHONPATH");
  envVars << "PYTHONPATH=" + pythonPath;

  if (env.contains("LD_LIBRARY_PATH"))
    envVars << "LD_LIBRARY_PATH=" + env.value("LD_LIBRARY_PATH");

  QString envVarsStr;
  for (const auto& var : envVars) envVarsStr += escapeArgument(var) + "\n";

  QString debugger = "gdb --args";
  if (program.endsWith(".py"))  // TODO: This is a hack
    debugger = "pdb3";

  QString msg = QString(
                    "Please start the extension\n\n%1\n\nwith the "
                    "arguments\n\n%2\n\nand the environment "
                    "variables\n\n%3\n\nenv %4 %5 %1 %2")
                    .arg(escapeArgument(program))
                    .arg(escapeArguments(args))
                    .arg(envVarsStr)
                    .arg(escapeArguments(envVars))
                    .arg(debugger);

  auto box = new QMessageBox(
      QMessageBox::Information, "Voxie extension debug support", msg,
      QMessageBox::StandardButtons(), voxieRoot().mainWindow());
  box->setAttribute(Qt::WA_DeleteOnClose, true);
  box->setWindowModality(Qt::NonModal);
  auto cancel = box->addButton(QMessageBox::Cancel);
  auto runNormally = box->addButton("Run normally", QMessageBox::AcceptRole);

  connect(runNormally, &QPushButton::clicked, callbackObject,
          runNormallyCallback);
  connect(cancel, &QPushButton::clicked, callbackObject, cancelCallback);

  box->show();

  return box;
}
