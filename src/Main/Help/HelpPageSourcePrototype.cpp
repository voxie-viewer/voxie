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

#include "HelpPageSourcePrototype.hpp"

#include <VoxieBackend/Component/Extension.hpp>

#include <Main/Help/CMark.hpp>
#include <Main/Help/CMarkUtil.hpp>
#include <Main/Help/HelpPage.hpp>
#include <Main/Help/HelpPageRegistry.hpp>

#include <Main/Root.hpp>

#include <Voxie/Component/HelpCommon.hpp>

#include <Voxie/Node/NodePrototype.hpp>

#include <VoxieClient/QtUtil.hpp>

#include <QtCore/QDir>

using namespace vx::help;

namespace vx {
namespace help {
class HelpPageSourcePrototype : public HelpPageSource {
  QSharedPointer<HelpPageRegistry> registry;

 public:
  HelpPageSourcePrototype(const QSharedPointer<HelpPageRegistry>& registry)
      : HelpPageSource("prototype"), registry(registry) {}
  ~HelpPageSourcePrototype() override {}

  QList<QSharedPointer<HelpPageInfo>> listPages() override {
    QList<QSharedPointer<HelpPageInfo>> list;

    QList<QSharedPointer<NodePrototype>> prototypes =
        Root::instance()->factories();
    std::sort(prototypes.begin(), prototypes.end(),
              [](const QSharedPointer<NodePrototype>& p1,
                 const QSharedPointer<NodePrototype>& p2) {
                return p1->name() < p2->name();
              });

    for (const auto& kind : allNodeKinds()) {
      for (const auto& prototype : prototypes) {
        if (prototype->nodeKind() != kind) continue;
        list << createQSharedPointer<HelpPageInfo>(
            uriForPrototype(prototype), prototype->displayName(),
            prototype->description(),
            QMap<QString, QString>{
                {"NodeKind", nodeKindToString(prototype->nodeKind())},
            });
        // TODO: add icon?
      }
    }

    return list;
  }

  QSharedPointer<HelpPage> render(const QString& suffix) override {
    if (!Root::instance()->prototypeMap().contains(suffix)) {
      qWarning() << "Attempt show help for invalid object" << suffix
                 << "from URL";
      return QSharedPointer<HelpPage>();
    }
    auto prototype = Root::instance()->prototypeMap()[suffix];

    auto title = prototype->displayName();

    // Generate markdown document
    auto doc = vx::cmark::Node::newNode(CMARK_NODE_DOCUMENT);

    addHeading(doc, 1, prototype->displayName());

    addLink(doc, uriForEditMarkdown(prototype), "Edit Markdown");

    auto extension =
        qSharedPointerDynamicCast<Extension>(prototype->container());
    if (extension && extension->offerShowSource())
      addLink(doc, uriForEditSource(prototype), "Edit Source");

    // Add description of the node (from prototype JSON)
    addTextParagraph(doc, prototype->description());

    // Add link to create the node in the node graph
    addLink(doc, uriForCreateNode(prototype),
            QString("Add %1").arg(prototype->displayName()));

    // Add help page content (if available)
    auto pageContent = registry->lookUpHelpPage(prototype);
    QString baseFilename = std::get<0>(pageContent);
    auto markdownSource = std::get<1>(pageContent);

    // Parse markdown document
    // https://github.com/commonmark/commonmark-spec/blob/master/CommonMark.dtd
    auto rootNode = vx::cmark::Node::parseDocument(markdownSource);

    // Move all nodes from rootNode to doc
    doc->appendAllChildrenOf(rootNode);

    return createQSharedPointer<HelpPage>(doc, baseFilename, title);
  }
};
}  // namespace help
}  // namespace vx

QSharedPointer<HelpPageSource> vx::help::getHelpPageSourcePrototype(
    const QSharedPointer<HelpPageRegistry>& registry) {
  return createQSharedPointer<HelpPageSourcePrototype>(registry);
}
