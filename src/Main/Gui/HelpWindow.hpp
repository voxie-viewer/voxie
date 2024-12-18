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

#include <Main/Help/HelpLinkHandler.hpp>
#include <Main/Help/HelpPageGenerator.hpp>
#include <Main/Help/HelpPageRegistry.hpp>

#include <Voxie/Gui/HelpBrowserBackend.hpp>

#include <QtCore/QSharedPointer>
#include <QtCore/QUrl>

#include <QtWidgets/QDialog>

namespace vx {

class NodePrototype;

namespace gui {

class HelpWindow : public QDialog {
  Q_OBJECT
 public:
  explicit HelpWindow(QWidget* parent = nullptr);

  void openHelpForUri(const QString& uri);

  void openHtmlString(const QString& uri, const QString& title,
                      const QString& html, const QUrl& baseUrl);

 Q_SIGNALS:
  void customLinkActivated(QString query);

 private:
  QSharedPointer<HelpBrowserBackendView> backendView;

  help::HelpLinkHandler linkHandler;
  help::HelpPageGenerator pageGenerator;

  QString lastUri;
};

}  // namespace gui
}  // namespace vx
