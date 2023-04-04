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

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

int main(int argc, char** argv) {
  if (argc < 6) {
    qCritical() << "Expected at least 5 arguments";
    return 1;
  }

  QString output = argv[1];
  QString srcDir = argv[2];
  QString binDir = argv[3];
  QString licenses = argv[4];

  QList<QString> additionalPythonLibDirs;
  for (int i = 6; i < argc; i++) {
    QString arg = argv[i];
    if (!arg.startsWith("--additional-python-lib-dir=")) {
      qCritical() << "Unexpected argument:" << arg;
      return 1;
    }
    QString path = arg.mid(strlen("--additional-python-lib-dir="));
    // qDebug() << "additional-python-lib-dir" << path;
    additionalPythonLibDirs << path;
  }

  // qDebug() << output << srcDir << binDir;

  QDir outputDir(output);
  outputDir.cdUp();

  QJsonArray binDirs{
      outputDir.relativeFilePath(binDir),
  };

  QJsonObject result;
  result["VoxieDir"] = outputDir.relativeFilePath(srcDir);
  result["LicensesPath"] = outputDir.relativeFilePath(licenses);
  result["AdditionalPluginParentDirs"] = binDirs;
  result["AdditionalScriptParentDirs"] = binDirs;
  result["AdditionalExtParentDirs"] = binDirs;
  if (additionalPythonLibDirs.size() != 0) {
    QJsonArray array;
    for (const auto& dir : additionalPythonLibDirs)
      array << outputDir.relativeFilePath(dir);
    result["AdditionalPythonLibDirs"] = array;
  }

  QFile file(output);
  if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
    qCritical() << "Error opening file" << file.fileName();
    return 1;
  }
  file.write(QJsonDocument(result).toJson());
  if (file.error()) {
    qCritical() << "Error writing file" << file.fileName();
    return 1;
  }

  return 0;
}
