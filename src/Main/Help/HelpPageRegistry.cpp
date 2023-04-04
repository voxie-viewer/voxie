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

#include "HelpPageRegistry.hpp"

#include <Main/Help/HelpPage.hpp>

#include <Voxie/Node/NodePrototype.hpp>

#include <QFile>

using namespace vx::help;
using namespace vx;

HelpPageRegistry::HelpPageRegistry() {}

void vx::help::HelpPageRegistry::registerHelpPageDirectory(QDir directory) {
  helpPageDirectories.append(directory);
}

void HelpPageRegistry::registerHelpPageFile(QString name, QString sourceFile) {
  this->helpPagesAdditionalFiles[name] << sourceFile;
}

std::tuple<QString, QSharedPointer<HelpPageDependencies>, QString>
vx::help::HelpPageRegistry::lookUpHelpPage(QString name) {
  QList<std::tuple<QString, QDateTime>> files;

  if (helpPagesAdditionalFiles.contains(name)) {
    for (const auto& fileName : helpPagesAdditionalFiles[name]) {
      QFileInfo info(fileName);
      auto lastModified = info.lastModified();
      files << std::make_tuple(fileName, lastModified);

      QFile helpFile(fileName);
      if (helpFile.open(QFile::ReadOnly | QFile::Text)) {
        return std::make_tuple(
            fileName, createQSharedPointer<HelpPageDependencies>(files),
            QString::fromUtf8(helpFile.readAll()));
      }
    }
  }

  for (auto directory : helpPageDirectories) {
    QString fileName = directory.filePath(name + ".md");

    QFileInfo info(fileName);
    auto lastModified = info.lastModified();
    files << std::make_tuple(fileName, lastModified);

    QFile helpFile(fileName);
    if (helpFile.open(QFile::ReadOnly | QFile::Text)) {
      return std::make_tuple(fileName,
                             createQSharedPointer<HelpPageDependencies>(files),
                             QString::fromUtf8(helpFile.readAll()));
    }
  }

  return std::make_tuple("", createQSharedPointer<HelpPageDependencies>(files),
                         "");
}
std::tuple<QString, QSharedPointer<HelpPageDependencies>, QString>
vx::help::HelpPageRegistry::lookUpHelpPage(
    const QSharedPointer<vx::NodePrototype> prototype) {
  return lookUpHelpPage(prototype->name());
}
