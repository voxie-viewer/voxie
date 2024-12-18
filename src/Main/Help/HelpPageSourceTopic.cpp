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

#include "HelpPageSourceTopic.hpp"

#include <Main/Root.hpp>

#include <Main/Help/CMark.hpp>
#include <Main/Help/HelpPage.hpp>

#include <Voxie/Component/HelpCommon.hpp>

#include <VoxieClient/QtUtil.hpp>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QDirIterator>

using namespace vx::help;

namespace vx {
namespace help {
class HelpPageSourceTopic : public HelpPageSource {
 public:
  HelpPageSourceTopic() : HelpPageSource("topic") {}
  ~HelpPageSourceTopic() override {}

  QList<QSharedPointer<HelpPageInfo>> listPages() override {
    QList<QSharedPointer<HelpPageInfo>> list;

    /*
    for (const auto& file :
         QDir(Root::instance()->directoryManager()->docTopicPath())
             .entryList(QStringList("*.md"), QDir::Files | QDir::Readable)) {
      auto name = file;
    */
    QDirIterator it(Root::instance()->directoryManager()->docTopicPath(),
                    QStringList("*.md"), QDir::Files,
                    QDirIterator::Subdirectories);
    // TODO: Check that QDir::separator() is the right thing to do under windows
    auto prefix = it.path() + QDir::separator();
    while (it.hasNext()) {
      auto name = it.next();
      if (!name.startsWith(prefix)) {
        qWarning() << "Path" << name << "does not start with" << prefix;
        continue;
      }
      name = name.mid(prefix.length());

      if (name.endsWith(".md")) name = name.left(name.length() - 3);

      // TODO: Use content of first top-level heading in file as title instead
      // of name
      list << createQSharedPointer<HelpPageInfo>(uriForHelpTopic(name), name,
                                                 "");
    }

    std::sort(list.begin(), list.end(),
              [](auto hp1, auto hp2) { return hp1->title() < hp2->title(); });

    return list;
  }

  QSharedPointer<HelpPage> render(const QString& suffix) override {
    auto title = suffix;

    // TODO: Sanity check for fileName?
    auto fileName = Root::instance()->directoryManager()->docTopicPath() + "/" +
                    suffix + ".md";

    auto dependencies = HelpPageDependencies::fromSingleFile(fileName);

    QFile helpFile(fileName);
    if (!helpFile.open(QFile::ReadOnly | QFile::Text)) {
      qWarning() << "Failed to open file" << fileName;
      return QSharedPointer<HelpPage>();
    }

    auto rootNode = vx::cmark::parseDocumentWithExtensions(
        QString::fromUtf8(helpFile.readAll()));

    return createQSharedPointer<HelpPage>(rootNode, fileName, dependencies,
                                          title);
  }
};
}  // namespace help
}  // namespace vx

QSharedPointer<HelpPageSource> vx::help::getHelpPageSourceTopic() {
  static QSharedPointer<HelpPageSource> source =
      createQSharedPointer<HelpPageSourceTopic>();
  return source;
}
