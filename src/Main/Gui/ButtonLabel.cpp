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

#include "ButtonLabel.hpp"

#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEngine>

#include <QtWidgets/QLabel>

using namespace vx::gui;

ButtonLabel::ButtonLabel(QWidget* parent) : QLabel(parent) {
  int size = 24;
  this->setMinimumSize(size, size);
  this->setMaximumSize(size, size);
  this->setAlignment(Qt::AlignCenter);
}

void ButtonLabel::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
    Q_EMIT this->clicked();
  }
}

void ButtonLabel::enterEvent(QEvent*) {
  this->setStyleSheet("QLabel { background-color: silver; }");
}

void ButtonLabel::leaveEvent(QEvent*) { this->setStyleSheet("QLabel { }"); }
