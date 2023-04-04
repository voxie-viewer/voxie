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

#include "HelpBrowserBackendImpl.hpp"

#include <VoxieClient/Exception.hpp>
#include <VoxieClient/QtUtil.hpp>

#include <Voxie/IVoxie.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>

#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>

#include <QtWidgets/QShortcut>

#include <QtWebEngineCore/QWebEngineUrlRequestJob>

#include <QtWebEngineWidgets/QWebEngineProfile>
#include <QtWebEngineWidgets/QWebEngineSettings>

using namespace vx;

VoxieUrlHandler::VoxieUrlHandler() {}

void VoxieUrlHandler::requestStarted(QWebEngineUrlRequestJob* request) {
  // https://doc.qt.io/qt-5/qwebengineurlrequestjob.html
  /*
  qDebug() << "requestStarted" << request << request->requestUrl()
           << request->requestMethod()
      //<< request->requestHeaders()
      //<< request->initiator()
      ;
  */

  if (request->requestMethod() != "GET" && request->requestMethod() != "HEAD") {
    request->fail(QWebEngineUrlRequestJob::RequestFailed);
    return;
  }

  auto url = request->requestUrl();

  auto reply = requestVoxieUrl(url);

  if (!reply) {
    qWarning() << "requestVoxieUrl returned nullptr for" << url;
    request->fail(QWebEngineUrlRequestJob::RequestFailed);
  } else if (reply->status() != RequestStatus::NoError) {
    switch (reply->status()) {
      case RequestStatus::UrlNotFound:
        request->fail(QWebEngineUrlRequestJob::UrlNotFound);
        break;
      case RequestStatus::UrlInvalid:
        request->fail(QWebEngineUrlRequestJob::UrlInvalid);
        break;
      case RequestStatus::RequestDenied:
        request->fail(QWebEngineUrlRequestJob::RequestDenied);
        break;
      case RequestStatus::RequestFailed:
        request->fail(QWebEngineUrlRequestJob::RequestFailed);
        break;
      default:
        qWarning() << "Got unknown value for RequestStatus:" << reply->status();
        request->fail(QWebEngineUrlRequestJob::RequestFailed);
        break;
    }
  } else {
    auto content = reply->takeContent();
    if (content) content->setParent(request);
    request->reply(reply->mimeType().toUtf8(), content);
  }
}

HelpWebPage::HelpWebPage(QWebEngineProfile* profile)
    : QWebEnginePage(profile) {}

bool HelpWebPage::acceptNavigationRequest(const QUrl& url, NavigationType type,
                                          bool) {
  if (type == QWebEnginePage::NavigationTypeLinkClicked) {
    Q_EMIT this->handleLink(url);
    return false;
  }
  if (type == QWebEnginePage::NavigationTypeReload) {
    Q_EMIT this->doReload();
    return false;
  }
  return true;
}

HelpBrowserBackendViewImpl::HelpBrowserBackendViewImpl() {
  // TODO: Use this?
  // auto handler = new VoxieUrlHandler;
  // QObject::connect(this, &QObject::destroyed, handler,
  // &QObject::deleteLater);

  // Create a new off-the-record profile (no need to store cookies etc. for the
  // help browser)
  auto profile = new QWebEngineProfile;
  QObject::connect(this, &QObject::destroyed, profile, &QObject::deleteLater);
  // profile->installUrlSchemeHandler("voxie", handler);

  // Automatically scale font sizes for High-DPI displays
  // https://doc.qt.io/qt-5.15/qtwebengine-overview.html#high-dpi-support
  // suggests to set Qt::AA_EnableHighDpiScaling, but this will cause problems
  // in other parts of voxie. Instead, change the default font sizes. Note that
  // this will not change the size of the scroll bars.

  if (0)
    qDebug()
        << "Font Sizes default"
        << profile->settings()->fontSize(QWebEngineSettings::MinimumFontSize)
        << profile->settings()->fontSize(
               QWebEngineSettings::MinimumLogicalFontSize)
        << profile->settings()->fontSize(QWebEngineSettings::DefaultFontSize)
        << profile->settings()->fontSize(
               QWebEngineSettings::DefaultFixedFontSize);
  double scaleFactor =
      1.0 / 96.0 * QGuiApplication::primaryScreen()->logicalDotsPerInchY();
  profile->settings()->setFontSize(QWebEngineSettings::MinimumFontSize,
                                   0 * scaleFactor);
  profile->settings()->setFontSize(QWebEngineSettings::MinimumLogicalFontSize,
                                   6 * scaleFactor);
  profile->settings()->setFontSize(QWebEngineSettings::DefaultFontSize,
                                   16 * scaleFactor);
  profile->settings()->setFontSize(QWebEngineSettings::DefaultFixedFontSize,
                                   13 * scaleFactor);
  if (0)
    qDebug()
        << "Font Sizes"
        << profile->settings()->fontSize(QWebEngineSettings::MinimumFontSize)
        << profile->settings()->fontSize(
               QWebEngineSettings::MinimumLogicalFontSize)
        << profile->settings()->fontSize(QWebEngineSettings::DefaultFontSize)
        << profile->settings()->fontSize(
               QWebEngineSettings::DefaultFixedFontSize);

  auto page = new HelpWebPage(profile);
  QObject::connect(this, &QObject::destroyed, page, &QObject::deleteLater);
  QObject::connect(
      page, &HelpWebPage::handleLink, this,
      [this](const QUrl& url) {
        // After a navigation request, call setHtml() again,
        // otherwise the next setHtml() call seems to be
        // sometimes(?) dropped. (It seems like the QWebEnginePage
        // expects new data after a navigation request.)
        if (this->view) this->view->setHtml(this->lastHtml, this->lastBaseUrl);
        handleLink(url);
      },
      Qt::QueuedConnection);
  QObject::connect(
      page, &HelpWebPage::doReload, this,
      [this]() {
        // After a navigation request, call setHtml() again,
        // otherwise the next setHtml() call seems to be
        // sometimes(?) dropped. (It seems like the QWebEnginePage
        // expects new data after a navigation request.)
        if (this->view) this->view->setHtml(this->lastHtml, this->lastBaseUrl);
        doReload();
      },
      Qt::QueuedConnection);

  view = new QWebEngineView;
  QObject::connect(this, &QObject::destroyed, view, &QObject::deleteLater);

  // Add shortcut for reloading
  QShortcut* shortcut = new QShortcut(QKeySequence("Ctrl+r"), view);
  QObject::connect(shortcut, &QShortcut::activated, view,
                   &QWebEngineView::reload);

  view->setPage(page);
}
HelpBrowserBackendViewImpl::~HelpBrowserBackendViewImpl() {}

QWidget* HelpBrowserBackendViewImpl::widget() {
  if (!view) {
    qWarning()
        << "HelpBrowserBackendViewImpl::widget(): view has been destroyed";
    return nullptr;
  }
  return view;
}

void HelpBrowserBackendViewImpl::setHtml(const QString& html,
                                         const QUrl& baseUrl) {
  if (0) {
    QFile file("/tmp/foo.html");
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
      qCritical() << "Error opening file" << file.fileName();
    } else {
      file.write(html.toUtf8());
    }
  }

  if (!view) {
    qWarning()
        << "HelpBrowserBackendViewImpl::widget(): view has been destroyed";
    return;
  }
  // qDebug() << "Calling setHtml" << html.length();
  // qDebug() << baseUrl;
  QUrl fixedBaseUrl = baseUrl;
  if (baseUrl == QUrl(""))
    // Without this webengine will not allow access to file:/// URIs (which is
    // currently needed)
    fixedBaseUrl = QUrl("file:///");
  lastHtml = html;
  lastBaseUrl = fixedBaseUrl;
  view->setHtml(html, fixedBaseUrl);
}

HelpBrowserBackendImpl::HelpBrowserBackendImpl() {}
HelpBrowserBackendImpl::~HelpBrowserBackendImpl() {}

QSharedPointer<HelpBrowserBackendView> HelpBrowserBackendImpl::createView() {
  return makeSharedQObject<HelpBrowserBackendViewImpl>();
}

QByteArray HelpBrowserBackendImpl::renderHtmlToPdf(const QString& html,
                                                   const QUrl& baseUrl) {
#if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)
  throw vx::Exception(
      "de.uni_stuttgart.Voxie.Error",
      "Compile-time version of Qt to old, rendering to PDF not supported");
#else
  // QObject mainObj;

  // Create a new off-the-record profile (no need to store cookies etc. for the
  // help browser)
  auto profile = new QWebEngineProfile;
  // QObject::connect(mainObj, &QObject::destroyed, profile,
  // &QObject::deleteLater);
  // profile->installUrlSchemeHandler("voxie", handler);

  auto page = new QWebEnginePage(profile);
  // QObject::connect(mainObj, &QObject::destroyed, page,
  // &QObject::deleteLater);

  bool haveResult = false;
  QByteArray result;

  // TODO: Clean up main loop related code here.

  QObject::connect(
      page, &QWebEnginePage::loadFinished, this,
      [page, baseUrl, &haveResult, &result](bool ok) {
        // qDebug() << "Loading finished" << ok << baseUrl;
        if (ok)
          page->printToPdf([&haveResult, &result](const QByteArray& res) {
            // qDebug() << "Printing finished";
            vx::checkOnMainThread("printToPdf callback");
            result = res;
            haveResult = true;
            qApp->quit();
          });
        else
          qApp->quit();
      });

  page->setHtml(html, baseUrl);

  qApp->exec();

  if (!haveResult)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error", "Failed to print PDF");

  return result;
#endif
}
