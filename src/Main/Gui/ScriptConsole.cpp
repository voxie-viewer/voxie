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

#include "ScriptConsole.hpp"

#include <iostream>

#include <Main/Gui/ScriptLineEdit.hpp>
#include <Main/Root.hpp>

#include <QtGui/QFontDatabase>

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

using namespace vx;
using namespace vx::gui;

ScriptConsole::ScriptConsole(QWidget* parent, const QString& title)
    : QDialog(parent), snippetEdit(nullptr), scriptLog(nullptr) {
  this->setWindowTitle(title);
  this->resize(800 / 96.0 * this->logicalDpiX(),
               500 / 96.0 * this->logicalDpiY());

  this->scriptLog = new QTextEdit(this);
  this->scriptLog->setReadOnly(true);
  this->scriptLog->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

  this->snippetEdit = new ScriptLineEdit(this);
  this->snippetEdit->setFocus();
  this->snippetEdit->setFont(
      QFontDatabase::systemFont(QFontDatabase::FixedFont));

  QPushButton* execButton = new QPushButton(this);
  execButton->setText("Run!");
  connect(execButton, &QPushButton::clicked, this,
          &ScriptConsole::executeScript);

  QHBoxLayout* hlayout = new QHBoxLayout();
  hlayout->addWidget(this->snippetEdit);
  hlayout->addWidget(execButton);

  QVBoxLayout* vlayout = new QVBoxLayout();
  vlayout->addWidget(this->scriptLog);
  vlayout->addLayout(hlayout);
  this->setLayout(vlayout);

  connect(Root::instance(), &Root::logEmitted, this,
          &ScriptConsole::appendLine);

  for (const auto& msg : Root::getBufferedMessages()) this->appendLine(msg);
}

void ScriptConsole::executeScript() {
  executeCode(this->snippetEdit->text());
  this->snippetEdit->setText("");
}

void ScriptConsole::append(const QString& log) {
  this->scriptLog->moveCursor(QTextCursor::End);
  this->scriptLog->insertPlainText(log);
  this->scriptLog->ensureCursorVisible();
}

void ScriptConsole::appendLine(const QString& log) { this->append(log + "\n"); }
