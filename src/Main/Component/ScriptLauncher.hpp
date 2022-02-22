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

#pragma once

#include <VoxieBackend/Component/ExtensionLauncher.hpp>

#include <QtCore/QProcess>

#if QT_VERSION < QT_VERSION_CHECK(5, 6, 0)
// Workaround for missing Q_DECLARE_METATYPE(QProcess::ExitStatus) in Qt 5.5
// Note: The returned string will be '', but at least the code
// QVariant(process->exitStatus()).toString() will compile
Q_DECLARE_METATYPE(QProcess::ExitStatus)
#endif

// TODO: Rename to ExtensionLauncherImpl?

namespace vx {
class IDirectoryManager;

class ScriptLauncher : public QObject, public ExtensionLauncher {
  Q_OBJECT

  quint64 currentScriptExecId = 0;

 public:
  explicit ScriptLauncher(QObject* parent = nullptr);
  virtual ~ScriptLauncher();

  // TODO: startScript() should return a QSharedPointer to an object idenfiying
  // a running script
  QProcess* startScript(
      const QString& scriptFile, const QString* executable = nullptr,
      const QStringList& arguments = QStringList(),
      QProcess* process = new QProcess(),
      const QSharedPointer<QString>& output = QSharedPointer<QString>(),
      bool setPythonLibDir = true) override;
  static void setupEnvironment(IDirectoryManager* directoryManager,
                               QProcess* process, bool setPythonLibDir = true);
  void setupEnvironment(QProcess* process,
                        bool setPythonLibDir = true) override;

  QObject* startDebug(const QString& program, const QStringList& args,
                      QObject* callbackObject,
                      const std::function<void()>& runNormallyCallback,
                      const std::function<void()>& cancelCallback) override;

 private:
  void printBufferToConsole(QProcess* process, int id,
                            const QSharedPointer<QString>& buffer,
                            const QSharedPointer<QString>& output);
};

}  // namespace vx
