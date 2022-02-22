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

#include "HelpWindow.hpp"

#include <Main/Help/HelpPage.hpp>
#include <Main/Root.hpp>

#include <Voxie/Node/NodePrototype.hpp>

#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>

using namespace vx;
using namespace vx::gui;

HelpWindow::HelpWindow(QWidget* parent)
    : QDialog(parent),
      linkHandler(this),
      pageGenerator(Root::instance()->helpRegistry()) {
  auto backend = Root::instance()->helpBrowserBackend();
  if (!backend) {
    qWarning() << "No help browser backend found";
  }
  if (backend) {
    backendView = backend->createView();
    QObject::connect(
        backendView.data(), &HelpBrowserBackendView::handleLink, this,
        [this](const QUrl& url) { this->linkHandler.handleLink(url); });
  }

  this->setWindowTitle("Voxie help");

  auto layout = new QVBoxLayout;
  layout->setContentsMargins(QMargins(0, 0, 0, 0));
  if (backendView) {
    layout->addWidget(backendView->widget());
  } else {
    auto label = new QLabel("No help browser backend found");
    label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    layout->addWidget(label);
  }
  this->setLayout(layout);
  this->setWindowFlags(Qt::Window);

  resize(1280, 720);
}

void HelpWindow::openHelpForUri(const QString& uri) {
  auto data = pageGenerator.generateHelpPage(uri, false, nullptr);
  // qDebug() << "setHtml" << this->backendView << std::get<0>(data)
  //         << std::get<1>(data).length();
  if (this->backendView) {
    this->backendView->setHtml(
        std::get<1>(data),
        std::get<0>(data)
            ? QUrl::fromLocalFile(std::get<0>(data)->baseFileName())
            : QUrl(""));
  }
  this->setWindowTitle(std::get<0>(data)
                           ? (std::get<0>(data)->title() + " - Voxie help")
                           : "Voxie help");
  this->show();
}
