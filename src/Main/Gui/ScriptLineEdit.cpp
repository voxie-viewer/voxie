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

#include "ScriptLineEdit.hpp"

#include <QtCore/QDebug>

#include <QtGui/QKeyEvent>

using namespace vx::gui;

ScriptLineEdit::ScriptLineEdit(QWidget* parent)
    : QLineEdit(parent), log(), logOffset(-1) {}

ScriptLineEdit::~ScriptLineEdit() {}

void ScriptLineEdit::keyPressEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key_Up) {
    if (this->logOffset < (this->log.length() - 1)) {
      if (this->logOffset <= this->log.length()) this->logOffset++;
      this->setText(this->log.at(this->logOffset));
    }
  } else if (event->key() == Qt::Key_Down) {
    if (this->logOffset > 0) {
      this->logOffset--;
      this->setText(this->log.at(this->logOffset));
    }
  } else if (event->key() == Qt::Key_Return) {
    if (this->text().length() > 0) {
      if ((this->log.size() == 0) || (this->log.at(0) != this->text()))
        this->log.insert(0, this->text());
      this->logOffset = -1;
    }
    QLineEdit::keyPressEvent(event);
  } else {
    QLineEdit::keyPressEvent(event);
    this->logOffset = -1;
  }
}
