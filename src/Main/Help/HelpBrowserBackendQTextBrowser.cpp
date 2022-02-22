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

#include "HelpBrowserBackendQTextBrowser.hpp"

#include <VoxieClient/Exception.hpp>
#include <VoxieClient/QtUtil.hpp>

#include <Voxie/IVoxie.hpp>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

using namespace vx;

TextBrowser::TextBrowser() { this->setOpenLinks(false); }
TextBrowser::~TextBrowser() {}

QVariant TextBrowser::loadResource(int type, const QUrl& url) {
  (void)type;
  // qDebug() << "loadResource" << type << url;

  if (url.scheme() != "voxie") {
    auto res = QTextBrowser::loadResource(type, url);
    // qDebug() << "loadResource result base" << res;
    return res;
    // return QVariant();
  }

  auto reply = requestVoxieUrl(url);

  if (reply->status() == RequestStatus::NoError && reply->content()) {
    auto data = reply->content()->readAll();
    // qDebug() << "loadResource result" << data.length();
    return data;
  } else {
    return QVariant();
  }
}

HelpBrowserBackendViewQTextBrowser::HelpBrowserBackendViewQTextBrowser() {
  view = new TextBrowser;
  QObject::connect(this, &QObject::destroyed, view, &QObject::deleteLater);

  QObject::connect(view, &QTextBrowser::anchorClicked, this, [this](QUrl url) {
    // qDebug() << "anchorClicked" << url;

    if (url.isRelative() && url.authority().isEmpty() && url.path().isEmpty() &&
        !url.hasQuery() && url.hasFragment()) {
      // qDebug() << "fragment" << url.fragment();
      TextBrowser* view = this->view;
      if (view) {
        view->scrollToAnchor(url.fragment());
      }
    } else {
      Q_EMIT handleLink(url);
    }
  });
}
HelpBrowserBackendViewQTextBrowser::~HelpBrowserBackendViewQTextBrowser() {}

QWidget* HelpBrowserBackendViewQTextBrowser::widget() {
  if (!view) {
    qWarning() << "HelpBrowserBackendViewQTextBrowser::widget(): view has been "
                  "destroyed";
    return nullptr;
  }
  return view;
}

void HelpBrowserBackendViewQTextBrowser::setHtml(const QString& html,
                                                 const QUrl& baseUrl) {
  if (!view) {
    qWarning() << "HelpBrowserBackendViewQTextBrowser::widget(): view has been "
                  "destroyed";
    return;
  }
  // qDebug() << "Calling setHtml" << html.length();
  lastHtml = html;
  lastBaseUrl = baseUrl;

  QStringList searchPaths;
  if (baseUrl.scheme() == "file")
    searchPaths << QFileInfo(baseUrl.path()).dir().path();
  // qDebug() << "searchPaths" << searchPaths;
  view->setSearchPaths(searchPaths);

  view->setHtml(html);
}

HelpBrowserBackendQTextBrowser::HelpBrowserBackendQTextBrowser() {}
HelpBrowserBackendQTextBrowser::~HelpBrowserBackendQTextBrowser() {}

QSharedPointer<HelpBrowserBackendView>
HelpBrowserBackendQTextBrowser::createView() {
  return makeSharedQObject<HelpBrowserBackendViewQTextBrowser>();
}

QByteArray HelpBrowserBackendQTextBrowser::renderHtmlToPdf(
    const QString& html, const QUrl& baseUrl) {
  (void)html;
  (void)baseUrl;

  throw vx::Exception(
      "de.uni_stuttgart.Voxie.Error",
      "Rendering to PDF not supported by HelpBrowserBackendQTextBrowser");
}
