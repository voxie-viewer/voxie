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

#include "ThirdPartyTab.hpp"

#include <Voxie/Gui/ErrorMessage.hpp>
#include <Voxie/IVoxie.hpp>

#include <VoxieBackend/Component/JsonUtil.hpp>

#include <VoxieClient/Exception.hpp>

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QTextEdit>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>

using namespace vx::gui::about;
using namespace vx;

ThirdPartyTab::ThirdPartyTab(QWidget* parent) : QWidget(parent) {
  setupElements(setupLayout());
}

QBoxLayout* ThirdPartyTab::setupLayout() {
  QBoxLayout* layout = new QHBoxLayout(this);
  return layout;
}

void ThirdPartyTab::setupElements(QBoxLayout* layout) {
  try {
    auto path = vx::voxieRoot().directoryManager()->licensesPath();
    // qDebug() << "LIC" << path;
    auto doc = vx::parseJsonFile(path + "/licenses.json");

    QString info;
    info += "The following software is bundled with Voxie:<br>\n";

    for (QJsonValue lic_ : vx::expectArray(doc)) {
      // qDebug() << lic;
      auto lic = vx::expectObject(lic_);

      auto nameDesc = vx::expectString(lic["Name"]);
      if (lic.contains("Description"))
        nameDesc = vx::expectString(lic["Description"]);
      auto licenseFilename = vx::expectString(lic["LicenseFilename"]);
      QString sourceCodeUri;
      if (lic.contains("SourceCodeURI"))
        sourceCodeUri = vx::expectString(lic["SourceCodeURI"]);

      auto fileUrl =
          QUrl::fromLocalFile(QDir(path).absoluteFilePath(licenseFilename))
              .toString(QUrl::FullyEncoded);

      info += "<br>\n";
      info += "<b>" + nameDesc.toHtmlEscaped() +
              "</b>. The license can be found in <a href=\"" +
              fileUrl.toHtmlEscaped() + "\"><code>" +
              ("licenses/" + licenseFilename).toHtmlEscaped() + "</code></a>";
      if (sourceCodeUri != "") {
        info += " The source code can be downloaded from<br><a href=\"" +
                sourceCodeUri.toHtmlEscaped() + "\"><code>" +
                sourceCodeUri.toHtmlEscaped() + "</code></a>.";
      }
      info += "<br>\n";
    }

    /*
    auto infoWidget = new QTextEdit;
    infoWidget->setReadOnly(true);
    infoWidget->setHtml(info);
    */
    auto infoWidget = new QLabel;
    infoWidget->setAlignment(Qt::AlignAbsolute);
    infoWidget->setTextFormat(Qt::RichText);
    infoWidget->setTextInteractionFlags(Qt::LinksAccessibleByKeyboard |
                                        Qt::LinksAccessibleByMouse);
    infoWidget->setOpenExternalLinks(true);
    infoWidget->setText(info);
    // voxieRoot().connectLinkHandler(infoWidget);

    // place elements
    layout->addWidget(infoWidget);

  } catch (vx::Exception& e) {
    showErrorMessage("Error loading third-party licenses", e);
  }
}

ThirdPartyTab::~ThirdPartyTab() {}
