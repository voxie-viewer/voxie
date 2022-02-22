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

#include <VoxieBackend/Component/JsonUtil.hpp>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QException>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

static QByteArray fixLineEnding(QByteArray data, bool crlf) {
  data = data.replace("\r\n", "\n");
  if (crlf) data = data.replace("\n", "\r\n");
  return data;
}

static void copyLicenseFile(const QString& inputFilename,
                            const QString& outputFilename, bool crlf) {
  /*
  if (!QFile::copy(inputFilename, outputFilename)) {
    QString msg =
        "QFile::copy() failed from " + inputFilename + " to " + outputFilename;
    qCritical() << msg;
    throw QException();
  }
  */

  QFile inputFile(inputFilename);
  if (!inputFile.open(QFile::ReadOnly)) {
    qCritical() << "Error opening file" << inputFile.fileName();
    throw QException();
  }
  auto data = inputFile.readAll();
  if (data.size() == 0) {
    qCritical() << "readAll() returned 0 bytes";
    throw QException();
  }

  data = fixLineEnding(data, crlf);

  QFile file(outputFilename);
  if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
    qCritical() << "Error opening file" << file.fileName();
    throw QException();
  }
  file.write(data);
  if (file.error()) {
    qCritical() << "Error writing file" << file.fileName();
    throw QException();
  }
}

int main(int argc, char** argv) {
  if (argc < 3) {
    qCritical() << "Expected at least 2 arguments";
    return 1;
  }

  QString output = argv[1];

  int argNr = 2;
  bool crlf = false;
  if (argv[argNr] && QString(argv[argNr]) == "--license-crlf") {
    crlf = true;
    argNr++;
  }

  QList<QString> files;
  for (; argNr < argc; argNr++) {
    QString arg = argv[argNr];
    files << arg;
  }

  // TODO: Base this on --license-crlf?
  QString voxieLicenseFilename = "LICENSE";
  if (crlf) voxieLicenseFilename = "LICENSE.txt";

  auto outputFile = output + "/licenses/licenses.json";
  auto outputFileReadme = output + "/licenses/README.md";
  auto outputFileReadmeTxt = output + "/licenses/README.txt";

  QDir outputDir(output + "/licenses");

  // qDebug() << "CollectLicenses" << output << files;

  outputDir.removeRecursively();
  if (!outputDir.mkpath(".")) {
    qCritical() << "mkpath() failed";
    return 1;
  }

  QJsonArray result;
  QString readme;
  QString readmeTxt;

  readme += "Licenses\n";
  readme += "========\n";
  readme += "\n";
  readme += "Voxie itself is under the the [MIT/X11 license](../" +
            voxieLicenseFilename + ")\n";
  readme += "\n";
  readme +=
      "This copy of voxie also contains the following bundeled software:\n";
  readme += "\n";

  readmeTxt += "Licenses\n";
  readmeTxt += "========\n";
  readmeTxt += "\n";
  readmeTxt += "Voxie itself is under the the MIT/X11 license (see ../" +
               voxieLicenseFilename + ")\n";
  readmeTxt += "\n";
  readmeTxt +=
      "This copy of voxie also contains the following bundeled software:\n";

  for (const auto& file : files) {
    // TODO: Merge code from the two if branches
    if (file.endsWith(".jsonl")) {
      QFile fileS(file);
      if (!fileS.open(QIODevice::ReadOnly)) {
        qCritical() << "Failed to open" << file << fileS.errorString();
        return 1;
      }
      auto data = fileS.readAll();
      if (data.size() == 0) {
        qCritical() << "readAll() returned 0 bytes";
        return 1;
      }
      int i = 0;
      for (const auto& line : data.split('\n')) {
        i++;
        if (line.trimmed() == "") continue;
        auto doc =
            vx::parseJsonData(line, file + " line " + QString::number(i));
        // qDebug() << doc;

        auto obj = vx::expectObject(doc);
        auto name = vx::expectString(obj["Name"]);
        QString description = "";
        if (obj.contains("Description"))
          description = vx::expectString(obj["Description"]);
        auto licenseFilename = vx::expectString(obj["LicenseFilename"]);
        QString sourceCodeUri;
        if (obj.contains("SourceCodeURI"))
          sourceCodeUri = vx::expectString(obj["SourceCodeURI"]);

        // qDebug() << licenseFilename;

        QDir fileDir(file);
        fileDir.cdUp();

        QString resultFilename = name + ".txt";

        copyLicenseFile(fileDir.filePath(licenseFilename),
                        outputDir.filePath(resultFilename), crlf);

        QJsonObject val{
            {"Name", name},
            {"LicenseFilename", resultFilename},
        };
        if (description != "") val["Description"] = description;
        if (sourceCodeUri != "") val["SourceCodeURI"] = sourceCodeUri;
        result.append(val);

        QString nameDesc = name;
        if (description != "") nameDesc = description;

        // TODO: Escape nameDesc, resultFilename and sourceCodeUri for markdown?
        // readme += "\n";
        readme += "- " + nameDesc + ". The license can be found in [" +
                  resultFilename + "](" + resultFilename + ").";
        if (sourceCodeUri != "") {
          readme += " The source code can be downloaded from <" +
                    sourceCodeUri + ">.";
        }
        readme += "\n";

        readmeTxt += "\n";
        readmeTxt += nameDesc + ". The license can be found in \"" +
                     resultFilename + "\".";
        if (sourceCodeUri != "") {
          readmeTxt += " The source code can be downloaded from <" +
                       sourceCodeUri + ">.";
        }
        readmeTxt += "\n";
      }

    } else {
      // qDebug() << file;
      auto doc = vx::parseJsonFile(file);
      // qDebug() << doc;

      auto obj = vx::expectObject(doc);
      auto name = vx::expectString(obj["Name"]);
      QString description = "";
      if (obj.contains("Description"))
        description = vx::expectString(obj["Description"]);
      auto licenseFilename = vx::expectString(obj["LicenseFilename"]);
      // qDebug() << licenseFilename;

      if (!file.endsWith(".sw.json")) {
        qCritical() << "!file.endsWith(\".sw.json\")";
        return 1;
      }
      QDir fileDir(file.left(file.length() - strlen(".sw.json")));
      // qDebug() << fileDir.filePath(licenseFilename);

      QString resultFilename = name + ".txt";

      copyLicenseFile(fileDir.filePath(licenseFilename),
                      outputDir.filePath(resultFilename), crlf);

      QJsonObject val{
          {"Name", name},
          {"LicenseFilename", resultFilename},
      };
      if (description != "") val["Description"] = description;
      result.append(val);

      QString nameDesc = name;
      if (description != "") nameDesc = description;

      // TODO: Escape nameDesc, resultFilename and sourceCodeUri for markdown?
      // readme += "\n";
      readme += "- " + nameDesc + ". The license can be found in [" +
                resultFilename + "](" + resultFilename + ").";
      /*
      if (sourceCodeUri != "") {
        readme +=
            " The source code can be downloaded from <" + sourceCodeUri + ">.";
      }
      */
      readme += "\n";

      readmeTxt += "\n";
      readmeTxt += nameDesc + ". The license can be found in \"" +
                   resultFilename + "\".";
      /*
      if (sourceCodeUri != "") {
        readmeTxt +=
            " The source code can be downloaded from <" + sourceCodeUri + ">.";
      }
      */
      readmeTxt += "\n";
    }
  }

  {
    QFile file(outputFile);
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
      qCritical() << "Error opening file" << file.fileName();
      return 1;
    }
    file.write(QJsonDocument(result).toJson());
    if (file.error()) {
      qCritical() << "Error writing file" << file.fileName();
      return 1;
    }
  }

  {
    QFile file(outputFileReadme);
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
      qCritical() << "Error opening file" << file.fileName();
      return 1;
    }
    file.write(fixLineEnding(readme.toUtf8(), crlf));
    if (file.error()) {
      qCritical() << "Error writing file" << file.fileName();
      return 1;
    }
  }

  {
    QFile file(outputFileReadmeTxt);
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
      qCritical() << "Error opening file" << file.fileName();
      return 1;
    }
    file.write(fixLineEnding(readmeTxt.toUtf8(), crlf));
    if (file.error()) {
      qCritical() << "Error writing file" << file.fileName();
      return 1;
    }
  }

  return 0;
}
