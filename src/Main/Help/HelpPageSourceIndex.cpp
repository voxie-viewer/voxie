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

#include "HelpPageSourceIndex.hpp"

#include <Main/Root.hpp>

#include <Main/Help/CMark.hpp>
#include <Main/Help/CMarkUtil.hpp>
#include <Main/Help/HelpPage.hpp>
#include <Main/Help/HelpPageSourceAll.hpp>
#include <Main/Help/HelpPageSourceDBusInterface.hpp>
#include <Main/Help/HelpPageSourcePrototype.hpp>
#include <Main/Help/HelpPageSourceTopic.hpp>

#include <Voxie/Component/HelpCommon.hpp>

#include <VoxieClient/QtUtil.hpp>

#include <QtCore/QDebug>
#include <QtCore/QDir>

using namespace vx::cmark;
using namespace vx::help;

namespace vx {
namespace help {
class HelpPageSourceIndex : public HelpPageSource {
  QSharedPointer<HelpPageRegistry> registry;

  void addEntry(const QSharedPointer<vx::cmark::Node>& node,
                const QSharedPointer<HelpPageInfo>& info) {
    addLink(node, info->url(), info->title());
    if (info->shortDescription() != "")
      // TODO: formatting
      addTextParagraph(node, info->shortDescription());
  }
  void addList(const QSharedPointer<vx::cmark::Node>& node,
               const QList<QSharedPointer<HelpPageInfo>> list) {
    for (const auto& info : list) addEntry(node, info);
  }

 public:
  HelpPageSourceIndex(const QSharedPointer<HelpPageRegistry>& registry)
      : HelpPageSource("index"), registry(registry) {}
  ~HelpPageSourceIndex() override {}

  QList<QSharedPointer<HelpPageInfo>> listPages() override {
    return {
        createQSharedPointer<HelpPageInfo>(urlPrefix(), "Index", ""),
    };
  }

  QSharedPointer<HelpPage> render(const QString& suffix) override {
    if (suffix != "") {
      qWarning() << "Attempt to look up index path with suffix:" << suffix;
      return QSharedPointer<HelpPage>();
    }

    // Generate markdown document
    auto doc = vx::cmark::Node::newNode(CMARK_NODE_DOCUMENT);

    // TODO: Should the result of the getHelpPageSource*() calls and the
    // listPages() calls be cached somewhere?

    addHeading(doc, 1, "List of help pages");
    addList(doc, getHelpPageSourceTopic()->listPages());

    addHeading(doc, 1, "List of special help pages");
    addList(doc, getHelpPageSourceIndex(registry)->listPages());
    addList(doc, getHelpPageSourceAll(registry)->listPages());
    // Add more lists if needed

    addHeading(doc, 1, "List of DBus interface pages");
    addList(doc, getHelpPageSourceDBusInterface()->listPages());

    addHeading(doc, 1, "List of prototypes");
    auto pages = getHelpPageSourcePrototype(registry)->listPages();
    for (const auto& kind : allNodeKinds()) {
      addHeading(doc, 2, nodeKindToString(kind));
      for (const auto& info : pages) {
        if (info->metaInformation()["NodeKind"] != nodeKindToString(kind))
          continue;
        addEntry(doc, info);
      }
    }

    return createQSharedPointer<HelpPage>(doc, "Index");
  }
};
}  // namespace help
}  // namespace vx

QSharedPointer<HelpPageSource> vx::help::getHelpPageSourceIndex(
    const QSharedPointer<HelpPageRegistry>& registry) {
  return createQSharedPointer<HelpPageSourceIndex>(registry);
}
