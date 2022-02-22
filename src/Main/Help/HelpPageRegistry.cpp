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

#include <Voxie/Node/NodePrototype.hpp>

#include <QFile>

using namespace vx::help;
using namespace vx;

HelpPageRegistry::HelpPageRegistry() {}

void vx::help::HelpPageRegistry::registerHelpPageDirectory(QDir directory) {
  helpPageDirectories.append(directory);
}

void vx::help::HelpPageRegistry::registerHelpPage(QString name,
                                                  QString sourceFile,
                                                  QString markdownSource) {
  helpPages.insert(name, std::make_tuple(sourceFile, markdownSource));
}

void HelpPageRegistry::registerHelpPageFile(QString name, QString sourceFile) {
  QFile file(sourceFile);
  if (file.open(QFile::ReadOnly | QFile::Text)) {
    registerHelpPage(name, sourceFile, QString::fromUtf8(file.readAll()));
  }
}

std::tuple<QString, QString> vx::help::HelpPageRegistry::lookUpHelpPage(
    QString name) {
  if (helpPages.contains(name)) {
    return helpPages[name];
  } else {
    for (auto directory : helpPageDirectories) {
      QString fileName = directory.filePath(name + ".md");
      QFile helpFile(fileName);
      if (helpFile.open(QFile::ReadOnly | QFile::Text)) {
        return std::make_tuple(fileName, QString::fromUtf8(helpFile.readAll()));
      }
    }
    return std::make_tuple("", "");
  }
}
std::tuple<QString, QString> vx::help::HelpPageRegistry::lookUpHelpPage(
    const QSharedPointer<vx::NodePrototype> prototype) {
  return lookUpHelpPage(prototype->name());
}
