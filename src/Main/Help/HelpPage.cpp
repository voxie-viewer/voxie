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

#include "HelpPage.hpp"

#include <VoxieClient/QtUtil.hpp>

#include <Main/DebugOptions.hpp>

#include <Main/Help/CMark.hpp>

#include <QtCore/QDebug>
#include <QtCore/QFileInfo>

using namespace vx::help;

HelpPageInfo::HelpPageInfo(const QString& url, const QString& title,
                           const QString& shortDescription,
                           const QMap<QString, QString>& metaInformation)
    : url_(url),
      title_(title),
      shortDescription_(shortDescription),
      metaInformation_(metaInformation) {}
HelpPageInfo::~HelpPageInfo() {}

HelpPageDependencies::HelpPageDependencies(
    const QList<std::tuple<QString, QDateTime>>& files)
    : files_(files) {}
HelpPageDependencies::~HelpPageDependencies() {}

bool HelpPageDependencies::isUpToDate() {
  if (vx::debug_option::Log_HelpPageCache()->get())
    qDebug() << "HelpPageDependencies::isUpToDate";

  for (const auto& entry : this->files()) {
    auto fn = std::get<0>(entry);
    auto lastModified = std::get<1>(entry);

    QFileInfo info(fn);
    auto modified = info.lastModified();
    auto upToDate = modified == lastModified;

    if (vx::debug_option::Log_HelpPageCache()->get())
      qDebug() << "  =>" << fn << lastModified << modified << upToDate;

    if (!upToDate) return false;
  }

  return true;
}

QSharedPointer<HelpPageDependencies> HelpPageDependencies::none() {
  // TODO: Cache?
  return createQSharedPointer<HelpPageDependencies>(
      QList<std::tuple<QString, QDateTime>>());
}
QSharedPointer<HelpPageDependencies> HelpPageDependencies::fromSingleFile(
    const QString& fileName) {
  QFileInfo info(fileName);
  auto lastModified = info.lastModified();

  QList<std::tuple<QString, QDateTime>> files{
      std::make_tuple(fileName, lastModified),
  };
  return createQSharedPointer<HelpPageDependencies>(files);
}

HelpPage::HelpPage(const QSharedPointer<vx::cmark::Node>& doc,
                   const QString& title)
    : HelpPage(doc, "", HelpPageDependencies::none(), title) {}
HelpPage::HelpPage(const QSharedPointer<vx::cmark::Node>& doc,
                   const QString& baseFileName,
                   const QSharedPointer<HelpPageDependencies>& dependencies,
                   const QString& title)
    : doc_(doc),
      baseFileName_(baseFileName),
      dependencies_(dependencies),
      title_(title) {}
HelpPage::~HelpPage() {}

QSharedPointer<vx::cmark::Node> HelpPage::docClone() {
  return doc()->cloneDeep();
}
