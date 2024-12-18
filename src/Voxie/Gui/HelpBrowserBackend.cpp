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

#include "HelpBrowserBackend.hpp"

#include <VoxieClient/QtUtil.hpp>

#include <Voxie/IVoxie.hpp>

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QSharedPointer>
#include <QtCore/QUrl>

using namespace vx;

RequestResponse::RequestResponse(RequestStatus status)
    : status_(status), mimeType_(""), content_(nullptr) {}
RequestResponse::RequestResponse(RequestStatus status, QString mimeType,
                                 QIODevice* content)
    : status_(status), mimeType_(mimeType), content_(content) {}
RequestResponse::~RequestResponse() {
  if (content_) delete content_;
}

QIODevice* RequestResponse::takeContent() {
  auto res = content_;
  content_ = nullptr;
  return res;
}

QSharedPointer<RequestResponse> vx::requestVoxieUrl(const QUrl& url) {
  if (url.scheme() != "voxie" || url.authority() != "") {
    return createQSharedPointer<RequestResponse>(RequestStatus::RequestFailed);
  }

  auto path = url.path();
  // qDebug() << path;

  // TODO
  if (path.startsWith("/help/static/lib/katex-0.11.1/") ||
      path.startsWith("/help/static/lib/simple.css/")) {
    bool isKatex = path.startsWith("/help/static/lib/katex-0.11.1/");
    auto relPath = isKatex ? path.mid(strlen("/help/static/lib/katex-0.11.1/"))
                           : path.mid(strlen("/help/static/lib/simple.css/"));
    for (const auto& part : relPath.split('/')) {
      if (part == "" || part == "." || part == "..") {
        qWarning() << "Request for" << url
                   << "denied because of invalid path component" << part;
        return createQSharedPointer<RequestResponse>(
            RequestStatus::RequestDenied);
      }
      for (const auto& c : part) {
        bool valid = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                     (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.';
        if (!valid) {
          qWarning() << "Request for" << url
                     << "denied because of invalid path char" << c;
          return createQSharedPointer<RequestResponse>(
              RequestStatus::RequestDenied);
        }
      }
    }

    // https://www.iana.org/assignments/media-types/media-types.xhtml
    QString type = "application/octet-stream";
    if (relPath.endsWith(".png"))
      type = "image/png";
    else if (relPath.endsWith(".jpeg") || relPath.endsWith(".jpg"))
      type = "image/jpeg";
    else if (relPath.endsWith(".js"))
      type = "application/javascript; charset=utf-8";
    else if (relPath.endsWith(".css"))
      type = "text/css; charset=utf-8";
    else if (relPath.endsWith(".ttf"))
      type = "font/ttf";
    else if (relPath.endsWith(".woff"))
      type = "font/woff";
    else if (relPath.endsWith(".woff2"))
      type = "font/woff2";

    QString sourceFilename =
        (isKatex ? voxieRoot().directoryManager()->katexPath()
                 : voxieRoot().directoryManager()->simpleCssPath()) +
        "/" + relPath;
    QFile* file = new QFile(sourceFilename);
    if (!file->open(QIODevice::ReadOnly)) {
      qWarning() << "Request for" << url << "failed: Cannot open"
                 << sourceFilename;
      delete file;
      return createQSharedPointer<RequestResponse>(RequestStatus::UrlNotFound);
    }
    return createQSharedPointer<RequestResponse>(RequestStatus::NoError, type,
                                                 file);
  }

  return createQSharedPointer<RequestResponse>(RequestStatus::UrlNotFound);
}

vx::HelpBrowserBackendView::HelpBrowserBackendView() {}
vx::HelpBrowserBackendView::~HelpBrowserBackendView() {}

vx::HelpBrowserBackend::HelpBrowserBackend() {}
vx::HelpBrowserBackend::~HelpBrowserBackend() {}
