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

#include <VoxieBackend/VoxieBackend.hpp>

#include <QtCore/QProcess>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>

#include <functional>

namespace vx {
/**
 * Interface for launching extension processes.
 */
class VOXIEBACKEND_EXPORT ExtensionLauncher {
 public:
  ExtensionLauncher();
  virtual ~ExtensionLauncher();

  virtual QProcess* startScript(
      const QString& scriptFile, const QString* executable = nullptr,
      const QStringList& arguments = QStringList(),
      QProcess* process = new QProcess(),
      const QSharedPointer<QString>& output = QSharedPointer<QString>(),
      bool setPythonLibDir = true) = 0;

  virtual void setupEnvironment(QProcess* process,
                                bool setPythonLibDir = true) = 0;

  /**
   * Will ask the user to start program with args in the debugger.
   *
   * The user can press either "Run normally" or "Cancel", in which case the
   * correspronding callback will be called. The callback will be called on
   * callbackObject's thread and only if callbackObject is still alive.
   *
   * When the returned object is destroyed, the dialog will disappear.
   * The returned object will be valid until the code returns to the main loop.
   */
  virtual QObject* startDebug(const QString& program, const QStringList& args,
                              QObject* callbackObject,
                              const std::function<void()>& runNormallyCallback,
                              const std::function<void()>& cancelCallback) = 0;
};
}  // namespace vx
