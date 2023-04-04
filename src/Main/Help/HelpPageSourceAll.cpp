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

#include "HelpPageSourceAll.hpp"

#include <Main/Root.hpp>

#include <Main/Help/CMark.hpp>
#include <Main/Help/CMarkUtil.hpp>
#include <Main/Help/HelpPage.hpp>
#include <Main/Help/HelpPageGenerator.hpp>
#include <Main/Help/HelpPageSourceDBusInterface.hpp>
#include <Main/Help/HelpPageSourceIndex.hpp>
#include <Main/Help/HelpPageSourcePrototype.hpp>
#include <Main/Help/HelpPageSourceTopic.hpp>

#include <Voxie/Component/HelpCommon.hpp>

#include <VoxieClient/QtUtil.hpp>

#include <QtCore/QDebug>
#include <QtCore/QDir>

using namespace vx::cmark;
using namespace vx::help;

// TODO: Merge partially with index?

namespace vx {
namespace help {
class HelpPageSourceAll : public HelpPageSource {
  QSharedPointer<HelpPageRegistry> registry;

  void addEntry(const QSharedPointer<vx::cmark::Node>& node,
                const QSharedPointer<HelpPageInfo>& info, int level) {
    auto anchorName = QString(QUrl::toPercentEncoding(info->url()));

    auto anchor = vx::cmark::Node::newNode(CMARK_NODE_CUSTOM_BLOCK);
    anchor->setOnEnter("<a name=\"" + anchorName.toHtmlEscaped() + "\"></a>");
    // anchor->setOnExit("");
    node->appendChild(anchor);

    addHeading(node, level, info->title());

    // Do not recursively include this page in itself
    if (info->url() == urlPrefix()) return;

    // TODO: Create somewhere else?
    auto pageGenerator = createQSharedPointer<HelpPageGenerator>(registry);

    auto page = pageGenerator->getHelpPage(info->url());
    if (!page) return;

    auto doc = page->docClone();

    // Turn links into anchors
    // TODO: Relative page URLs? Where should those be handled?
    walk(doc, [&](const QSharedPointer<vx::cmark::Node>& childNode) {
      auto type = childNode->type();
      // qDebug() << (void*)childNode << type;
      if (type != CMARK_NODE_LINK) return;
      QString url = childNode->getUrl();
      if (url.startsWith(Protocol + ":" + commands::Help))
        childNode->setUrl("#" + QUrl::toPercentEncoding(url));
      // qDebug() << "url" << url << childNode->getUrl();
    });

    // Update relative image URLs
    walk(doc, [&](const QSharedPointer<vx::cmark::Node>& childNode) {
      auto type = childNode->type();
      // qDebug() << (void*)childNode << type;
      if (type != CMARK_NODE_IMAGE) return;
      QString url = childNode->getUrl();
      if (QUrl(url).isRelative())
        // TODO: This is a hack, try to get rid of per-page baseFileName()?
        // TODO: exportHelpPage() does not work well with absolute URIs
        childNode->setUrl(QFileInfo(page->baseFileName()).dir().path() + "/./" +
                          url);
      // qDebug() << "url" << url << childNode->getUrl() <<
      // page->baseFileName();
    });

    shiftHeadings(doc, level);
    // Move all nodes from rootNode to doc
    node->appendAllChildrenOf(doc);
  }
  void addList(const QSharedPointer<vx::cmark::Node>& node,
               const QList<QSharedPointer<HelpPageInfo>> list, int level) {
    for (const auto& info : list) addEntry(node, info, level);
  }

 public:
  HelpPageSourceAll(const QSharedPointer<HelpPageRegistry>& registry)
      : HelpPageSource("all"), registry(registry) {}
  ~HelpPageSourceAll() override {}

  QList<QSharedPointer<HelpPageInfo>> listPages() override {
    return {
        createQSharedPointer<HelpPageInfo>(urlPrefix(), "All in one page", ""),
    };
  }

  QSharedPointer<HelpPage> render(const QString& suffix) override {
    if (suffix != "") {
      qWarning() << "Attempt to look up all path with suffix:" << suffix;
      return QSharedPointer<HelpPage>();
    }

    // Generate markdown document
    auto doc = vx::cmark::Node::newNode(CMARK_NODE_DOCUMENT);

    // TODO: Should the result of the getHelpPageSource*() calls and the
    // listPages() calls be cached somewhere?

    addHeading(doc, 1, "List of help pages");
    addList(doc, getHelpPageSourceTopic()->listPages(), 2);

    addHeading(doc, 1, "List of special help pages");
    addList(doc, getHelpPageSourceIndex(registry)->listPages(), 2);
    addList(doc, getHelpPageSourceAll(registry)->listPages(), 2);
    // Add more lists if needed

    addHeading(doc, 1, "List of DBus interface pages");
    addList(doc, getHelpPageSourceDBusInterface()->listPages(), 2);

    addHeading(doc, 1, "List of prototypes");
    auto pages = getHelpPageSourcePrototype(registry)->listPages();
    for (const auto& kind : allNodeKinds()) {
      addHeading(doc, 2, nodeKindToString(kind));
      for (const auto& info : pages) {
        if (info->metaInformation()["NodeKind"] != nodeKindToString(kind))
          continue;
        addEntry(doc, info, 3);
      }
    }

    return createQSharedPointer<HelpPage>(doc, "All in one page");
  }
};
}  // namespace help
}  // namespace vx

QSharedPointer<HelpPageSource> vx::help::getHelpPageSourceAll(
    const QSharedPointer<HelpPageRegistry>& registry) {
  return createQSharedPointer<HelpPageSourceAll>(registry);
}
