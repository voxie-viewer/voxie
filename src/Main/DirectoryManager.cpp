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

#include "DirectoryManager.hpp"

#include <VoxieClient/Exception.hpp>

#include <VoxieClient/JsonUtil.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QStandardPaths>

using namespace vx;

DirectoryManager::DirectoryManager(QObject* parent) : QObject(parent) {
#if !defined(Q_OS_WIN)
  QString split = ":";
#else
  QString split = ";";
#endif

  QDir binDir(QCoreApplication::applicationDirPath());
  auto pathConfigDoc =
      parseJsonFile(binDir.absoluteFilePath("voxie-path.json"));
  if (!pathConfigDoc.isObject())
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "voxie-path.json does not contain an object");
  auto pathConfig = pathConfigDoc.object();

  if (!pathConfig.contains("VoxieDir"))
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "voxie-path.json does not contain a 'VoxieDir' entry");
  if (!pathConfig["VoxieDir"].isString())
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "voxie-path.json 'VoxieDir' entry is not a string");
  auto voxieDir = QDir::cleanPath(
      binDir.absoluteFilePath(pathConfig["VoxieDir"].toString()));

  auto env = QProcessEnvironment::systemEnvironment();

  baseDir_ = QDir(voxieDir).absolutePath();
  pythonLibDir_ = QDir(baseDir_).absoluteFilePath("pythonlib");
  docPrototypePath_ = QDir(baseDir_).absoluteFilePath("doc/prototype");
  docTopicPath_ = QDir(baseDir_).absoluteFilePath("doc/topic");
  katexPath_ = QDir(baseDir_).absoluteFilePath("lib/katex-0.11.1");

  QString pluginPathStr = env.value("VOXIE_PLUGIN_PATH", "");
  QString scriptPathStr = env.value("VOXIE_SCRIPT_PATH", "");
  QString extensionPathStr = env.value("VOXIE_EXTENSION_PATH", "");

  if (env.contains("VOXIE_PYTHON_EXECUTABLE")) {
    pythonExecutable_ = env.value("VOXIE_PYTHON_EXECUTABLE");
  } else {
    if (pathConfig.contains("PythonExecutable"))
      pythonExecutable_ =
          binDir.absoluteFilePath(pathConfig["PythonExecutable"].toString());
    else
      pythonExecutable_ = "python3";
  }

  if (pathConfig.contains("LicensesPath"))
    licensesPath_ =
        binDir.absoluteFilePath(pathConfig["LicensesPath"].toString());
  else
    licensesPath_ = QDir(baseDir_).absoluteFilePath("licenses");

  if (pathConfig.contains("OldManualFile"))
    oldManualFile_ =
        binDir.absoluteFilePath(pathConfig["OldManualFile"].toString());
  else
    oldManualFile_ = "";

  for (QString path : pluginPathStr.split(split)) {
    if (path == "") continue;
    pluginPath_.push_back(path);
  }
  for (QString path : scriptPathStr.split(split)) {
    if (path == "") continue;
    scriptPath_.push_back(path);
  }
  for (QString path : extensionPathStr.split(split)) {
    if (path == "") continue;
    extensionPath_.push_back(path);
  }

#if !defined(Q_OS_WIN)
  for (QString path : QStandardPaths::standardLocations(
           QStandardPaths::GenericConfigLocation)) {
    QString configDir = path + "/voxie";
    pluginPath_.push_back(configDir + "/plugins");
    scriptPath_.push_back(configDir + "/scripts");
    extensionPath_.push_back(configDir + "/ext");      // TODO
    extensionPath_.push_back(configDir + "/filters");  // TODO
  }
#else
  QString configDir = QDir::homePath() + "/AppData/Roaming/voxie";
  pluginPath_.push_back(configDir + "/plugins");
  scriptPath_.push_back(configDir + "/scripts");
  extensionPath_.push_back(configDir + "/ext");      // TODO
  extensionPath_.push_back(configDir + "/filters");  // TODO
#endif

  pluginPath_.push_back(QDir(baseDir_).absoluteFilePath("plugins"));
  scriptPath_.push_back(QDir(baseDir_).absoluteFilePath("scripts"));
  extensionPath_.push_back(QDir(baseDir_).absoluteFilePath("ext"));
  extensionPath_.push_back(QDir(baseDir_).absoluteFilePath("filters"));

  if (pathConfig.contains("AdditionalPluginParentDirs")) {
    for (auto parentDirName :
         pathConfig["AdditionalPluginParentDirs"].toArray()) {
      QDir parentDir(binDir.absoluteFilePath(parentDirName.toString()));
      for (QString dir : parentDir.entryList(
               QStringList("Plugin*"),
               QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable)) {
        pluginPath_.push_back(parentDir.absoluteFilePath(dir));
      }
    }
  }

  if (pathConfig.contains("AdditionalScriptParentDirs")) {
    for (auto parentDirName :
         pathConfig["AdditionalScriptParentDirs"].toArray()) {
      QDir parentDir(binDir.absoluteFilePath(parentDirName.toString()));
      for (QString dir : parentDir.entryList(
               QStringList("Script*"),
               QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable)) {
        scriptPath_.push_back(parentDir.absoluteFilePath(dir));
      }
    }
  }

  if (pathConfig.contains("AdditionalExtParentDirs")) {
    for (auto parentDirName : pathConfig["AdditionalExtParentDirs"].toArray()) {
      QDir parentDir(binDir.absoluteFilePath(parentDirName.toString()));
      for (QString dir : parentDir.entryList(
               QStringList("Ext*"),
               QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable)) {
        extensionPath_.push_back(parentDir.absoluteFilePath(dir));
      }
    }
  }

  if (pathConfig.contains("AdditionalPythonLibDirs")) {
    for (auto parentDirName : pathConfig["AdditionalPythonLibDirs"].toArray()) {
      QDir dir(binDir.absoluteFilePath(parentDirName.toString()));
      additionalPythonLibDirs_.push_back(dir.absolutePath());
    }
  }

  if (env.value("VOXIE_DEBUG_DUMP_DIRECTORIES", "") == "1") dump();

  additionalPythonLibDirs_.push_back(
      QDir(baseDir_).absoluteFilePath("python-extra"));

  allPythonLibDirs_ << pythonLibDir_;
  allPythonLibDirs_ << additionalPythonLibDirs_;
}

void DirectoryManager::dump() const {
#define DUMP(value) qDebug() << #value << this->value()
  DUMP(baseDir);
  DUMP(pluginPath);
  DUMP(scriptPath);
  DUMP(extensionPath);
  DUMP(pythonLibDir);
  DUMP(pythonExecutable);
  DUMP(docPrototypePath);
  DUMP(docTopicPath);
  DUMP(katexPath);
  DUMP(oldManualFile);
#undef DUMP
}

DirectoryManager::~DirectoryManager() {}
