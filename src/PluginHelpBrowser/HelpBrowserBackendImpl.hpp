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

#include <Voxie/Gui/HelpBrowserBackend.hpp>

#include <QtCore/QPointer>

#include <QtWebEngineCore/QWebEngineUrlSchemeHandler>

#include <QtWebEngineWidgets/QWebEnginePage>
#include <QtWebEngineWidgets/QWebEngineView>

class VoxieUrlHandler : public QWebEngineUrlSchemeHandler {
  Q_OBJECT

 public:
  VoxieUrlHandler();

  void requestStarted(QWebEngineUrlRequestJob* request) override;
};

class HelpWebPage : public QWebEnginePage {
  Q_OBJECT

 public:
  HelpWebPage(QWebEngineProfile* profile);

  bool acceptNavigationRequest(const QUrl& url, NavigationType type,
                               bool isMainFrame) override;

 Q_SIGNALS:
  void handleLink(const QUrl& url);
  void doReload();
};

class HelpBrowserBackendViewImpl : public vx::HelpBrowserBackendView {
  Q_OBJECT

  QPointer<QWebEngineView> view;

  QString lastHtml;
  QUrl lastBaseUrl;

 public:
  HelpBrowserBackendViewImpl();
  ~HelpBrowserBackendViewImpl();

  QWidget* widget() override;

  void setHtml(const QString& html, const QUrl& baseUrl) override;
};

class HelpBrowserBackendImpl : public vx::HelpBrowserBackend {
  Q_OBJECT

 public:
  HelpBrowserBackendImpl();
  ~HelpBrowserBackendImpl();

  QSharedPointer<vx::HelpBrowserBackendView> createView() override;

  QByteArray renderHtmlToPdf(const QString& html, const QUrl& baseUrl) override;
};
