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

#include "InformationTab.hpp"

#include <VoxieClient/JsonUtil.hpp>

#include <Main/Version.hpp>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>

using namespace vx;
using namespace vx::gui::about;

InformationTab::InformationTab(QWidget* parent) : QWidget(parent) {
  setupElements(setupLayout());
}

QGridLayout* InformationTab::setupLayout() {
  QGridLayout* layout = new QGridLayout(this);

  // column spacing
  layout->setColumnMinimumWidth(1, 8);
  layout->setColumnMinimumWidth(2, 20);

  // row spacing
  layout->setRowMinimumHeight(1, 10);
  layout->setRowMinimumHeight(2, 10);
  layout->setRowMinimumHeight(3, 10);
  layout->setRowMinimumHeight(4, 10);
  return layout;
}

void InformationTab::setupElements(QGridLayout* layout) {
  lbl_name = new QLabel("Name: ");
  lbl_version = new QLabel("Version: ");
  lbl_author = new QLabel("Authors: ");
  lbl_homepage = new QLabel("Homepage: ");
  lbl_documentation = new QLabel("Dokumentation: ");

  auto json = expectObject(vx::parseJsonFile(":/doc/software.json"));
  auto homepage = expectString(json["Homepage"]);
  QList<std::tuple<QString, QString>> authors;
  for (const QJsonValue& value : expectArray(json["Authors"]))
    authors << std::make_tuple(expectString(expectArray(value)[0]),
                               expectString(expectArray(value)[1]));
  std::sort(authors.begin(), authors.end(), [](const auto& v1, const auto& v2) {
    if (std::get<1>(v1) < std::get<1>(v2)) return true;
    if (std::get<1>(v1) > std::get<1>(v2)) return false;

    return std::get<0>(v1) < std::get<0>(v2);
  });
  QString authorsStr;
  for (int i = 0; i < authors.size(); i++) {
    if (i != 0) {
      authorsStr += ",";
      if (i % 4 == 0)
        authorsStr += "\n";
      else
        authorsStr += " ";
    }

    authorsStr += std::get<0>(authors[i]) + " " + std::get<1>(authors[i]);
  }

  lbl_name_val = new QLabel("Voxie");
  lbl_version_val = new QLabel(vx::getVersionString());
  lbl_author_val = new QLabel(authorsStr);
  lbl_homepage_val = new QLabel;
  lbl_homepage_val->setTextInteractionFlags(Qt::LinksAccessibleByKeyboard |
                                            Qt::LinksAccessibleByMouse);
  lbl_homepage_val->setOpenExternalLinks(true);
  lbl_homepage_val->setText("<a href=\"" + homepage.toHtmlEscaped() + "\">" +
                            homepage.toHtmlEscaped() + "</a>");
  layout->addWidget(lbl_name, 0, 0);
  layout->addWidget(lbl_version, 1, 0);
  layout->addWidget(lbl_author, 2, 0);
  layout->addWidget(lbl_homepage, 3, 0);

  layout->addWidget(lbl_name_val, 0, 1);
  layout->addWidget(lbl_version_val, 1, 1);
  layout->addWidget(lbl_author_val, 2, 1);
  layout->addWidget(lbl_homepage_val, 3, 1);
}

InformationTab::~InformationTab() {}
