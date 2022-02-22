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

#include "AboutLicenseTab.hpp"

#include <QtCore/QDebug>
#include <QtCore/QFile>

#include <QtWidgets/QHBoxLayout>

using namespace vx::gui::about;
using namespace vx;

AboutLicenseTab::AboutLicenseTab(QWidget* parent) : QWidget(parent) {
  setupElements(setupLayout());
}

QBoxLayout* AboutLicenseTab::setupLayout() {
  QBoxLayout* layout = new QHBoxLayout(this);
  return layout;
}

void AboutLicenseTab::setupElements(QBoxLayout* layout) {
  edit_license = new QTextEdit;

  QFile licenseFile(":/LICENSE");
  QString license;
  if (!licenseFile.open(QFile::ReadOnly | QFile::Text)) {
    qWarning() << "Failed to get license";
  } else {
    license = QString::fromUtf8(licenseFile.readAll());
  }

  edit_license->setReadOnly(true);
  edit_license->setText(license);
  edit_license->setAlignment(Qt::AlignAbsolute);

  // place elements
  layout->addWidget(this->edit_license);
}

AboutLicenseTab::~AboutLicenseTab() {}
