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

#include <QMap>
#include <QSharedPointer>
#include <QString>
#include <QUrl>

#include <functional>

namespace vx {
namespace cmark {
class Node;
}

class NodePrototype;

namespace help {

class HelpPage;
class HelpPageRegistry;

class HelpPageGenerator {
 public:
  HelpPageGenerator(const QSharedPointer<help::HelpPageRegistry>& registry);

  QSharedPointer<HelpPage> getHelpPage(const QString& url) const;

  std::tuple<QSharedPointer<HelpPage>, QString> generateHelpPage(
      const QString& url, bool useRelativeUrls,
      const std::function<QString(const QString&, const QString&)>& fixupUrls)
      const;

  QSharedPointer<vx::cmark::Node> createTOC(
      QSharedPointer<vx::cmark::Node>& root) const;

  void clearRenderCache();

 private:
  QSharedPointer<help::HelpPageRegistry> registry;

  QString pageTemplate;

  mutable QMap<QString, QSharedPointer<HelpPage>> helpPageCache;
  mutable QMap<QString, std::tuple<QSharedPointer<HelpPage>, QString>>
      renderedHelpPageCache;
};

}  // namespace help
}  // namespace vx
