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

#include <QDir>
#include <QList>
#include <QMap>
#include <QString>

namespace vx {
class NodePrototype;

namespace help {

class HelpPageRegistry {
 public:
  HelpPageRegistry();

  /**
   * Registers a directory containing markdown files with the following naming
   * scheme: <fully-qualified-prototype-name>.md
   * (Example: de.uni_stuttgart.Voxie.ExtFilterLocalThickness.md)
   *
   * Files from this directory will be included when looking up the markdown
   *source of a help page.
   */
  void registerHelpPageDirectory(QDir directory);

  /**
   * Registers a help page with the specified name. The second parameter is
   * expected to contain the source code of a valid Markdown document.
   */
  void registerHelpPage(QString name, QString sourceFile,
                        QString markdownSource);

  /**
   * Registers a help page with the specified name, loading the help content
   * from a filename. The second parameter is expected to contain the name of a
   * file containing source code of a valid Markdown document.
   */
  void registerHelpPageFile(QString name, QString sourceFile);

  /**
   * Returns the the filename containing the markdown source code and the
   * markdown source code associated with the specified help page name, or an
   * empty string if no help content was found for the specified name.
   */
  std::tuple<QString, QString> lookUpHelpPage(QString name);
  std::tuple<QString, QString> lookUpHelpPage(
      const QSharedPointer<vx::NodePrototype> prototype);

 private:
  QList<QDir> helpPageDirectories;
  QMap<QString, std::tuple<QString, QString>> helpPages;
};

}  // namespace help
}  // namespace vx
