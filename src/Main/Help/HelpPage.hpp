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

#include <QtCore/QMap>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>

namespace vx {

namespace cmark {
class Node;
}

namespace help {

class HelpPageInfo {
  QString url_;
  QString title_;
  QString shortDescription_;
  QMap<QString, QString> metaInformation_;

 public:
  HelpPageInfo(
      const QString& url, const QString& title, const QString& shortDescription,
      const QMap<QString, QString>& metaInformation = QMap<QString, QString>());
  ~HelpPageInfo();

  const QString& url() const { return url_; }
  const QString& title() const { return title_; }
  const QString& shortDescription() const { return shortDescription_; }
  const QMap<QString, QString>& metaInformation() const {
    return metaInformation_;
  }
};

// TODO: Should this contain a HelpPageInfo?
class HelpPage {
  QSharedPointer<vx::cmark::Node> doc_;
  QString baseFileName_;
  QString title_;

 public:
  HelpPage(const QSharedPointer<vx::cmark::Node>& doc,
           const QString& baseFileName, const QString& title);
  ~HelpPage();

  const QSharedPointer<vx::cmark::Node>& doc() const { return doc_; }
  const QString& baseFileName() const { return baseFileName_; }
  const QString& title() const { return title_; }

  QSharedPointer<vx::cmark::Node> docClone();
};

}  // namespace help
}  // namespace vx
