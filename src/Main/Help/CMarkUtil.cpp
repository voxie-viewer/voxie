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

#include "CMarkUtil.hpp"

#include <Main/Help/CMark.hpp>

#include <QtCore/QDebug>

using namespace vx::cmark;

void vx::cmark::addTextParagraph(const QSharedPointer<Node>& parent,
                                 const QString& text) {
  auto textPar = vx::cmark::Node::newNode(CMARK_NODE_PARAGRAPH);
  auto textText = vx::cmark::Node::newNode(CMARK_NODE_TEXT);
  textText->setLiteral(text);
  textPar->appendChild(textText);
  parent->appendChild(textPar);
}

void vx::cmark::addHeading(const QSharedPointer<Node>& parent, int level,
                           const QString& text) {
  auto headingElement = vx::cmark::Node::newNode(CMARK_NODE_HEADING);
  headingElement->setHeadingLevel(level);
  auto headingText = vx::cmark::Node::newNode(CMARK_NODE_TEXT);
  headingText->setLiteral(text);
  headingElement->appendChild(headingText);
  parent->appendChild(headingElement);
}

void vx::cmark::addLink(const QSharedPointer<Node>& parent, const QString& uri,
                        const QString& text) {
  auto linkPar = vx::cmark::Node::newNode(CMARK_NODE_PARAGRAPH);
  auto link = vx::cmark::Node::newNode(CMARK_NODE_LINK);
  link->setUrl(uri);
  auto linkText = vx::cmark::Node::newNode(CMARK_NODE_TEXT);
  linkText->setLiteral(text);
  link->appendChild(linkText);
  linkPar->appendChild(link);
  parent->appendChild(linkPar);
}

void vx::cmark::shiftHeadings(QSharedPointer<Node>& root, int addToLevel) {
  walk(root, [&](const QSharedPointer<Node>& node) {
    if (node->type() != CMARK_NODE_HEADING) return;
    int oldLevel = node->getHeadingLevel();
    int newLevel = oldLevel + addToLevel;

    if (newLevel < 1) {
      qWarning() << "Cannot change heading level from" << oldLevel << "to"
                 << newLevel;
      newLevel = 1;
    }
    if (newLevel > 6) {
      qWarning() << "Cannot change heading level from" << oldLevel << "to"
                 << newLevel;
      newLevel = 6;
    }

    node->setHeadingLevel(newLevel);
  });
}
