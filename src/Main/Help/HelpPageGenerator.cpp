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

#include "HelpPageGenerator.hpp"

#include <Main/Help/CMark.hpp>
#include <Main/Help/HelpPage.hpp>
#include <Main/Help/HelpPageRegistry.hpp>
#include <Main/Help/HelpPageSource.hpp>

#include <Main/Root.hpp>

#include <Voxie/Component/HelpCommon.hpp>

#include <Voxie/Node/NodePrototype.hpp>

using namespace vx;
using namespace vx::help;

HelpPageGenerator::HelpPageGenerator(
    const QSharedPointer<help::HelpPageRegistry>& registry)
    : registry(registry), pageTemplate("%1") {
  QFile templateFile(":/Help/template.html");
  if (templateFile.open(QFile::ReadOnly | QFile::Text)) {
    pageTemplate = QString::fromUtf8(templateFile.readAll());
  } else {
    qWarning() << "Missing HTML help template";
  }
}

/*
static const char* mathHeaderMathJaxCDN = R"(
  <script>
    MathJax = {
        startup: {
            elements: [ '.math_tex_inline', '.math_tex_block' ]
        },
        tex: {
            inlineMath: [['<math_tex_inline>', '</math_tex_inline>']],
            blockMath: [['<math_tex_block>', '</math_tex_block>']],
            processEscapes: false,
            processRefs: false,
            processEnvironments: false
        }
  };
  </script>
  <script type="text/javascript" id="MathJax-script" async
          src="https://cdn.jsdelivr.net/npm/mathjax@3/es5/tex-chtml.js">
  </script>
)";
*/

static const char* mathHeader = R"(
  <link rel="stylesheet" href="%1katex.min.css">
  <script type="text/javascript" src="%1katex.min.js">
  </script>
  <script>
    function renderMath() {
      // https://katex.org/docs/options.html
      var elementsInline = document.getElementsByClassName("math_tex_inline");
      for (let i=0; i<elementsInline.length; i++) {
        var element = elementsInline[i];
        //console.log(elementsInline[i]);
        var text = element.innerText;
        katex.render(text, element, {
            displayMode: false,
            throwOnError: false
        });
      }
      var elementsBlock = document.getElementsByClassName("math_tex_block");
      for (let i=0; i<elementsBlock.length; i++) {
        var element = elementsBlock[i];
        //console.log(elementsBlock[i]);
        var text = element.innerText;
        katex.render(text, element, {
            displayMode: true,
            throwOnError: false
        });
      }
    }
    window.addEventListener('load', renderMath);
  </script>
)";

QSharedPointer<HelpPage> HelpPageGenerator::getHelpPage(
    const QString& url) const {
  if (helpPageCache.contains(url)) return helpPageCache[url];

  QSharedPointer<HelpPage> page;

  QUrl qurl(url);
  QString hostAndPath = "//" + qurl.authority() + qurl.path();
  if (qurl.scheme() != Protocol) {
    qWarning() << "HelpPageGenerator: Unknown url scheme:" << qurl.scheme();
  } else if (!hostAndPath.startsWith(commands::Help)) {
    qWarning() << "HelpPageGenerator: Unknown url:" << url;
  } else {
    auto path = hostAndPath.mid(commands::Help.length());
    int index = path.indexOf('/');
    QString prefix, suffix;
    if (index == -1) {
      prefix = path;
      suffix = "";
    } else {
      prefix = path.left(index);
      suffix = path.mid(index + 1);
    }

    auto sources = getHelpPageSources(registry);
    if (!sources.contains(prefix)) {
      qWarning() << "HelpPageGenerator: Unknown help prefix" << prefix
                 << "for url" << url;
    } else {
      page = sources[prefix]->render(suffix);
    }
  }

  helpPageCache[url] = page;

  return page;
}

std::tuple<QSharedPointer<HelpPage>, QString>
HelpPageGenerator::generateHelpPage(
    const QString& url, bool useRelativeUrls,
    const std::function<QString(const QString&, const QString&)>& fixupUrls)
    const {
  // Cache help pages instead of re-generating them everytime
  if (renderedHelpPageCache.contains(url)) return renderedHelpPageCache[url];

  auto page = getHelpPage(url);

  if (!page) {
    renderedHelpPageCache[url] =
        std::make_tuple(QSharedPointer<HelpPage>(), "");
    return renderedHelpPageCache[url];
  }

  auto doc = page->docClone();

  QString baseFileName = page->baseFileName();
  if (baseFileName == "")
    baseFileName = voxieRoot().directoryManager()->docPrototypePath() + "/";
  QString baseDir = QFileInfo(baseFileName).dir().path();

  bool haveMath = false;

  /*
  // Remove all HTML nodes / replace them by code blocks
  walkReplace(doc, [&](QSharedPointer<vx::cmark::Node>& node) {
    auto type = node->type();
    if (type != CMARK_NODE_HTML_INLINE) return;
    auto newNode = vx::cmark::Node::newNode(CMARK_NODE_CODE);
    newNode->setLiteral(node->getLiteral());
    node = newNode;
  });
  walkReplace(doc, [&](QSharedPointer<vx::cmark::Node>& node) {
    auto type = node->type();
    if (type != CMARK_NODE_HTML_BLOCK) return;
    auto newNode = vx::cmark::Node::newNode(CMARK_NODE_CODE_BLOCK);
    newNode->setLiteral(node->getLiteral());
    node = newNode;
  });
  */
  bool enableMath = true;
  // Gitlab-like math support:
  // https://docs.gitlab.com/ce/user/markdown.html#math
  if (enableMath) {
    walkReplace(doc, [&](QSharedPointer<vx::cmark::Node>& node) {
      auto type = node->type();
      // qDebug() << (void*)node << type;
      if (type != CMARK_NODE_CODE_BLOCK) return;
      QString info = node->getFenceInfo();
      if (info != "math") return;
      QString content = node->getLiteral();
      // qDebug() << "content" << content;

      QString html;
      html += "<div class=\"math_tex_block\">";
      // html += "&lt;math_tex_block&gt;";
      html += content.toHtmlEscaped();
      // html += "&lt;/math_tex_block&gt;";
      html += "</div>";
      // qDebug() << "html" << html;
      auto newNode = vx::cmark::Node::newNode(CMARK_NODE_CUSTOM_BLOCK);
      newNode->setOnEnter(html);
      // newNode->setOnExit(" ");
      haveMath = true;
      node = newNode;
    });
    walkReplace(doc, [&](QSharedPointer<vx::cmark::Node>& node) {
      auto type = node->type();
      // qDebug() << (void*)node << type;
      if (type != CMARK_NODE_CODE) return;
      auto prev = node->previous();
      if (!prev || prev->type() != CMARK_NODE_TEXT) return;
      auto next = node->next();
      if (!next || next->type() != CMARK_NODE_TEXT) return;

      // TODO: This should detect whether the '$' is escaped and not consider
      // this as math then. (i.e. \$`1+2`$ or $`1+2`\$ should be left alone.)
      // cmark does not seem to have this information in the AST however.
      QString prevText = prev->getLiteral();
      if (!prevText.endsWith("$")) return;
      QString nextText = next->getLiteral();
      if (!nextText.startsWith("$")) return;

      QString content = node->getLiteral();
      // qDebug() << "content" << content;

      prevText.remove(prevText.length() - 1, 1);
      nextText.remove(0, 1);
      prev->setLiteral(prevText);
      next->setLiteral(nextText);

      QString html;
      html += "<span class=\"math_tex_inline\">";
      // html += "&lt;math_tex_inline&gt;";
      html += content.toHtmlEscaped();
      // html += "&lt;/math_tex_inline&gt;";
      html += "</span>";
      // qDebug() << "html" << html;
      auto newNode = vx::cmark::Node::newNode(CMARK_NODE_CUSTOM_INLINE);
      newNode->setOnEnter(html);
      // newNode->setOnExit(" ");
      haveMath = true;
      node = newNode;
    });
  }
  if (fixupUrls) {
    walk(doc, [&](const QSharedPointer<vx::cmark::Node>& node) {
      auto type = node->type();
      // qDebug() << (void*)node << type;
      if (type != CMARK_NODE_LINK && type != CMARK_NODE_IMAGE) return;
      QString childUrl = node->getUrl();
      // qDebug() << "url" << url;
      QString newChildUrl = fixupUrls(childUrl, baseDir);
      if (childUrl != newChildUrl) node->setUrl(newChildUrl);
    });
  }

  auto toc = createTOC(doc);
  /*
  auto headingNew =
      doc->firstChild();  // Note: createTOC might change heading
  if (headingNew) headingNew->insertAfter(toc);
  */
  // addLinkPar->insertAfter(toc);
  doc->prependChild(toc);

  auto html = doc->renderHtml();
  // qDebug() << doc->renderXml().toUtf8().data();

  QString header = "";
  if (haveMath) {
    QString katexBase;
    // TODO: Use fixupUrls instead of useRelativeUrls?
    if (useRelativeUrls) {
      katexBase = "voxie:///help/static/lib/katex-0.11.1/";
      // katexBase = "lib/katex-0.11.1/";
    } else {
      // katexBase = "voxie:///help/static/lib/katex-0.11.1/";
      katexBase =
          QUrl::fromLocalFile(voxieRoot().directoryManager()->katexPath())
              .toString() +
          "/";
    }
    if (fixupUrls) katexBase = fixupUrls(katexBase, baseDir);

    header += QString(mathHeader).arg(katexBase);
  }

  renderedHelpPageCache[url] =
      std::make_tuple(page, pageTemplate.arg(html, header));
  return renderedHelpPageCache[url];
}

static QString getInnerText(const QSharedPointer<vx::cmark::Node>& node) {
  QString res;
  walk(node, [&](const QSharedPointer<vx::cmark::Node>& child) {
    res += child->getLiteral();
  });
  return res;
}

QSharedPointer<vx::cmark::Node> HelpPageGenerator::createTOC(
    QSharedPointer<vx::cmark::Node>& root) const {
  // TODO: Nicer anchor IDs?
  uint64_t lastId = 0;
  QList<std::tuple<int, QString, QString>> headings;
  walkReplace(root, [&](QSharedPointer<vx::cmark::Node>& node) {
    if (node->type() != CMARK_NODE_HEADING) return;

    lastId++;
    auto id = "toc-" + QString::number(lastId);

    headings << std::make_tuple(node->getHeadingLevel(), id,
                                getInnerText(node));

    auto newNode = vx::cmark::Node::newNode(CMARK_NODE_CUSTOM_BLOCK);
    newNode->setOnEnter("<h" + QString::number(node->getHeadingLevel()) +
                        " id=\"" + id + "\">");
    newNode->setOnExit("</h" + QString::number(node->getHeadingLevel()) + ">");
    newNode->appendAllChildrenOf(node);
    node = newNode;
  });

  QString html;
  html += "<h2>Contents</h2>\n";
  int curLevel = 0;
  for (const auto& entry : headings) {
    int level = std::get<0>(entry);
    QString id = std::get<1>(entry);
    QString text = std::get<2>(entry);

    while (curLevel > level) {
      html += "</li>\n</ul>\n";
      curLevel--;
    }
    while (curLevel < (level - 1)) {
      html += "<ul>\n<li>\n";
      curLevel++;
    }
    if (curLevel < level) {
      html += "<ul>\n<li>\n";
      curLevel++;
    } else {
      html += "</li>\n<li>\n";
    }

    html += "<a href=\"#" + id.toHtmlEscaped() + "\">";
    html += text.toHtmlEscaped();
    html += "</a>";
    html += "</a>";
  }
  while (curLevel > 0) {
    html += "</li>\n</ul>\n";
    curLevel--;
  }

  auto tocNode = vx::cmark::Node::newNode(CMARK_NODE_CUSTOM_BLOCK);
  tocNode->setOnEnter(html);
  return tocNode;
}

void HelpPageGenerator::clearRenderCache() { renderedHelpPageCache.clear(); }
