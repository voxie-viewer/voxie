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

#include <cmark.h>

// C++ wrappers for CMark

// https://manpages.debian.org/testing/libcmark-dev/cmark.3.en.html

#include <QtCore/QSharedPointer>

namespace vx {
namespace cmark {

class String {
  Q_DISABLE_COPY(String)

  char* data_;

 public:
  explicit String(char* data);
  // TODO: Does this work properly under windows? (i.e. will this use the same c
  // library as cmark?)
  ~String();

  char* data() { return data_; }

  QString createQString();
};

class Node {
  cmark_node* data_;
  bool owned_;

  Node(cmark_node* data, bool owned);

 public:
  ~Node();

  static QSharedPointer<Node> newOwned(cmark_node* data);
  static QSharedPointer<Node> newUnowned(cmark_node* data);
  static QSharedPointer<Node> newUnownedAllowNull(cmark_node* data);

  static QSharedPointer<Node> newNode(cmark_node_type type);

  static QSharedPointer<Node> parseDocument(const QString& buffer,
                                            int options = CMARK_OPT_DEFAULT);

  cmark_node* data() const { return data_; }
  bool owned() const { return owned_; }

  cmark_node* steal();

  cmark_node_type type() const;

  QSharedPointer<Node> previous() const;
  QSharedPointer<Node> next() const;

  void* getUserData() const;
  void setUserData(void* data) const;

  QString getLiteral() const;
  void setLiteral(const QString& str) const;

  cmark_list_type getListType() const;
  void setListType(cmark_list_type type) const;

  cmark_delim_type getListDelim() const;
  void setListDelim(cmark_delim_type delim) const;

  int getListStart() const;
  void setListStart(int start) const;

  bool getListTight() const;
  void setListTight(bool tight) const;

  QString getOnEnter() const;
  void setOnEnter(const QString& str) const;
  QString getOnExit() const;
  void setOnExit(const QString& str) const;

  QString getUrl() const;
  void setUrl(const QString& str) const;

  QString getTitle() const;
  void setTitle(const QString& str) const;

  QString getFenceInfo() const;
  void setFenceInfo(const QString& str) const;

  int getHeadingLevel() const;
  void setHeadingLevel(int level) const;

  void unlink();

  QSharedPointer<Node> firstChild() const;

  void prependChild(const QSharedPointer<Node>& child) const;
  void appendChild(const QSharedPointer<Node>& child) const;

  void insertBefore(const QSharedPointer<Node>& silbing) const;
  void insertAfter(const QSharedPointer<Node>& silbing) const;

  // Take all children of oldParent and move them to the end of *this.
  void appendAllChildrenOf(const QSharedPointer<Node>& oldParent);

  QString renderHtml(int options = CMARK_OPT_DEFAULT) const;
  QString renderXml(int options = CMARK_OPT_DEFAULT) const;

  // TODO: Return a proxy object here?
  QList<QSharedPointer<Node>> children() const;

  QSharedPointer<Node> cloneDeep() const;
};

template <typename Fun>
void walk(const QSharedPointer<Node>& node, const Fun& fun) {
  for (cmark_node* child = cmark_node_first_child(node->data()); child;
       child = cmark_node_next(child))
    walk<Fun>(Node::newUnowned(child), fun);

  fun(node);
}

template <typename Fun>
void walkReplace(QSharedPointer<Node>& node, const Fun& fun) {
  for (cmark_node* child = cmark_node_first_child(node->data()); child;
       child = cmark_node_next(child)) {
    auto childNode = Node::newUnowned(child);

    walkReplace<Fun>(childNode, fun);

    if (child != childNode->data()) {
      cmark_node_replace(child, childNode->steal());
      cmark_node_free(child);
      child = childNode->data();
    }
  }

  fun(node);
}

}  // namespace cmark
}  // namespace vx
