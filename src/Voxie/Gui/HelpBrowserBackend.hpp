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

#include <Voxie/Voxie.hpp>

#include <QtCore/QIODevice>
#include <QtCore/QObject>

namespace vx {
enum RequestStatus : quint32 {
  NoError,
  UrlNotFound,
  UrlInvalid,
  RequestDenied,
  RequestFailed,
};

class VOXIECORESHARED_EXPORT RequestResponse {
  RequestStatus status_;
  QString mimeType_;
  QIODevice* content_;

 public:
  RequestResponse(RequestStatus status);
  RequestResponse(RequestStatus status, QString mimeType, QIODevice* content);
  ~RequestResponse();

  RequestStatus status() { return status_; }
  const QString& mimeType() { return mimeType_; }
  QIODevice* content() { return content_; }

  /**
   * Return the content and take over responsibility for destroying the object.
   */
  QIODevice* takeContent();
};

VOXIECORESHARED_EXPORT QSharedPointer<RequestResponse> requestVoxieUrl(
    const QUrl& url);

class VOXIECORESHARED_EXPORT HelpBrowserBackendView : public QObject {
  Q_OBJECT

 public:
  HelpBrowserBackendView();
  ~HelpBrowserBackendView();

  virtual QWidget* widget() = 0;

  virtual void setHtml(const QString& html, const QUrl& baseUrl) = 0;

 Q_SIGNALS:
  void handleLink(const QUrl& url);
  void doReload();
};

class VOXIECORESHARED_EXPORT HelpBrowserBackend : public QObject {
  Q_OBJECT

 public:
  HelpBrowserBackend();
  ~HelpBrowserBackend();

  virtual QSharedPointer<HelpBrowserBackendView> createView() = 0;

  virtual QByteArray renderHtmlToPdf(const QString& html,
                                     const QUrl& baseUrl) = 0;
};
}  // namespace vx
