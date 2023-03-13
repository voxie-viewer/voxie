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

#include "PreferencesWindow.hpp"

#include <Main/Root.hpp>

#include <Main/Gui/Preferences/OpenclPreferences.hpp>
#include <Main/Gui/Preferences/ScriptPreferences.hpp>

#include <Voxie/Component/Plugin.hpp>

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>

using namespace vx;
using namespace vx::gui;
using namespace vx::plugin;
using namespace vx::gui::preferences;

PreferencesWindow::PreferencesWindow(QWidget* parent) : QDialog(parent) {
  this->resize(500 / 96.0 * this->logicalDpiX(),
               450 / 96.0 * this->logicalDpiY());
  QVBoxLayout* layout = new QVBoxLayout();
  {
    QTabWidget* tabs = new QTabWidget(this);
    {
      tabs->addTab(new ScriptPreferences(), "Scripting");
      tabs->addTab(new OpenclPreferences(), "OpenCL");

      for (const auto& plugin : ::vx::Root::instance()->plugins()) {
        QWidget* page = plugin->preferencesWidget();
        if (page == nullptr) {
          continue;
        }
        tabs->addTab(page, plugin->name());
      }
    }
    layout->addWidget(tabs);
  }
  {
    QHBoxLayout* hbox = new QHBoxLayout();

    hbox->addSpacerItem(
        new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

    {
      QPushButton* cancelButton = new QPushButton("Cancel");
      connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
      hbox->addWidget(cancelButton);
    }
    {
      QPushButton* okButton = new QPushButton("Ok");
      connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
      hbox->addWidget(okButton);
    }

    layout->addLayout(hbox);
  }
  this->setLayout(layout);
}

PreferencesWindow::~PreferencesWindow() {}
