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

#include "ExportHelpPages.hpp"

#include <QtCore/QDir>
#include <QtCore/QSet>

#include <Voxie/Component/HelpCommon.hpp>

#include <Voxie/Gui/HelpBrowserBackend.hpp>

#include <Main/Help/HelpPage.hpp>
#include <Main/Help/HelpPageGenerator.hpp>
#include <Main/Help/HelpPageSource.hpp>

#include <Main/Root.hpp>

using namespace vx::help;

bool vx::help::exportHelpPages(Root* root, HelpPageGenerator* pageGenerator,
                               const QString& dir) {
  // TODO: Handle conflicting files in multiple baseDirs
  QSet<QString> copiedFiles;
  bool fail = false;

  auto sources = getHelpPageSources(root->helpRegistry());
  QList<QSharedPointer<HelpPageInfo>> pages;
  for (const auto& source : sources) pages << source->listPages();

  for (const auto& info : pages) {
    QString prefix = Protocol + ":" + vx::help::commands::Help;
    if (!info->url().startsWith(prefix)) {
      qWarning() << "Got help page URI not starting with" << prefix << ":"
                 << info->url();
      continue;
    }
    QString name = info->url().mid(prefix.size());
    // TODO: Sanity check for filename

    int depth = name.count("/");
    QString rootPath = "";
    for (int i = 0; i < depth; i++) rootPath += "../";

    auto data = pageGenerator->generateHelpPage(
        info->url(), true,
        [&](const QString& url, const QString& baseDir) -> QString {
          // qDebug() << "fixupUrls" << url << baseDir;
          if (url == "") return url;
          QUrl parsedUrl(url);
          if (parsedUrl.scheme() == vx::help::Protocol) {
            if (("//" + parsedUrl.path())
                    .startsWith(vx::help::commands::HelpStatic)) {
              auto pname = ("//" + parsedUrl.path())
                               .mid(vx::help::commands::HelpStatic.length());
              return rootPath + "static/" + pname;
            }
            if (("//" + parsedUrl.path())
                    .startsWith(vx::help::commands::Help)) {
              auto pname = ("//" + parsedUrl.path())
                               .mid(vx::help::commands::Help.length());
              return rootPath + pname + ".html";
            }
            // TODO: What should be done with voxie:action URLs?
            return url;
          } else if (parsedUrl.scheme() == "doi") {
            return "https://dx.doi.org/" +
                   QString::fromUtf8(QUrl::toPercentEncoding(parsedUrl.path()));
          }

          if (!parsedUrl.isRelative()) return url;
          if (parsedUrl.toEncoded(QUrl::RemoveFragment) == "") return url;

          // qDebug() << "url" << parsedUrl;
          // TODO: sanity check of filename
          // TODO: filename vs. URI escaping
          QString filename = parsedUrl.path();
          QString pathPrefix = "files/";
          bool ignore = false;
          QString modifiedFilename = filename;
          if (QFileInfo(filename).isAbsolute()) {
            int pos = filename.indexOf("/./");
            if (pos == -1) {
              qWarning() << "Got absolute filename in URI" << filename
                         << baseDir;
            } else {
              modifiedFilename = filename.mid(pos + 3);
              // TODO: This is a hack
              // This relies on the fact that absolute filenames are produced
              // only by HelpPageSourceAll and all the files are copied
              // elsewhere
              ignore = true;
            }
          }
          if (!copiedFiles.contains(filename)) {
            copiedFiles.insert(filename);
            QString sourceFilename = QDir(baseDir).filePath(filename);
            QString targetFilename = dir + "/" + pathPrefix + modifiedFilename;
            // qDebug() << filename << targetFilename;
            if (!QFileInfo(targetFilename).dir().mkpath(".")) {
              qCritical() << "Failed to create directory";
              fail = true;
            }
            if (!ignore) {
              if (!QFile::copy(sourceFilename, targetFilename)) {
                qCritical() << "Copying" << sourceFilename << "to"
                            << targetFilename << "failed.";
                fail = true;
              }
            }
          }
          // TODO: Convert modifiedFilename to URL?
          return rootPath + pathPrefix + modifiedFilename;
        });

    QFile file(dir + "/" + name + ".html");
    if (!QFileInfo(file).dir().mkpath(".")) {
      qCritical() << "Failed to create directory for" << file.fileName();
      return false;
    }
    // qDebug() << "Writing" << file.fileName();
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
      qCritical() << "Error opening file" << file.fileName();
      return false;
    } else {
      file.write(std::get<1>(data).toUtf8());
    }

    // Note: This only works if headless is set to false (but seems to work
    // e.g. with Xvfb)
    if (0) {
      // Render without relative URLs
      pageGenerator->clearRenderCache();  // TODO: Should not be needed
      auto data2 = pageGenerator->generateHelpPage(info->url(), false, nullptr);
      auto hbb = root->helpBrowserBackend();
      if (!hbb)
        throw Exception("de.uni_stuttgart.Voxie.Error",
                        "No help browser backend found");
      auto pdfData = hbb->renderHtmlToPdf(
          std::get<1>(data2),
          std::get<0>(data2)
              ? QUrl::fromLocalFile(std::get<0>(data2)->baseFileName())
              : QUrl(""));
      QFile pdfFile(dir + "/" + name + ".pdf");
      // qDebug() << "Writing" << pdfFile.fileName();
      if (!pdfFile.open(QFile::WriteOnly | QFile::Truncate)) {
        qCritical() << "Error opening file" << pdfFile.fileName();
        return false;
      } else {
        pdfFile.write(pdfData);
      }
    }
  }

  // TODO: Also support this under windows
  QDir(dir).mkpath("static/lib");
  if (QProcess::execute("cp",
                        {"-R", "--", root->directoryManager()->katexPath(),
                         dir + "/static/lib/katex-0.11.1"})) {
    qWarning() << "cp failed";
    fail = true;
  }

  return !fail;
}
