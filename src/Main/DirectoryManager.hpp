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

#include <QtCore/QObject>
#include <QtCore/QString>

#include <Voxie/IVoxie.hpp>

namespace vx {
/**
 * @brief A class for finding plugins and scripts.
 */
class DirectoryManager : public QObject, public IDirectoryManager {
  Q_OBJECT

  QString baseDir_;
  QList<QString> pluginPath_;
  QList<QString> scriptPath_;
  QList<QString> extensionPath_;
  QString pythonLibDir_;
  QList<QString> additionalPythonLibDirs_;
  QList<QString> allPythonLibDirs_;
  QString pythonExecutable_;
  QString docPrototypePath_;
  QString docTopicPath_;
  QString licensesPath_;
  QString katexPath_;

 public:
  DirectoryManager(QObject* parent = nullptr);
  virtual ~DirectoryManager();

  const QString& baseDir() const override { return baseDir_; }
  const QList<QString>& pluginPath() const override { return pluginPath_; }
  const QList<QString>& scriptPath() const override { return scriptPath_; }
  const QList<QString>& extensionPath() const override {
    return extensionPath_;
  }
  const QString& pythonLibDir() const override { return pythonLibDir_; }
  const QList<QString>& additionalPythonLibDirs() const override {
    return additionalPythonLibDirs_;
  }
  const QList<QString>& allPythonLibDirs() const override {
    return allPythonLibDirs_;
  }
  const QString& pythonExecutable() const override { return pythonExecutable_; }
  const QString& docPrototypePath() const override { return docPrototypePath_; }
  const QString& docTopicPath() const override { return docTopicPath_; }
  const QString& licensesPath() const override { return licensesPath_; }
  const QString& katexPath() const override { return katexPath_; }

  void dump() const;
};

}  // namespace vx
