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

#include "HorizontalScrollArea.hpp"

#include <QtCore/QDebug>
#include <QtCore/QEvent>

#include <QtWidgets/QScrollBar>

vx::HorizontalScrollArea::HorizontalScrollArea(QWidget* parent)
    : QScrollArea(parent) {
  // Note: If a frame is show, this also would have to be considered for the
  // minimum size
  setFrameShape(QFrame::NoFrame);

  setWidgetResizable(true);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  connect(this->horizontalScrollBar(), &QScrollBar::valueChanged, this,
          [this](int value) {
            (void)value;
            // qDebug() << "value changed to" << value;

            // TODO: Why is this needed? Without this the widget is not updated
            // on scrolling.
            this->widget()->update();
          });

  connect(this->horizontalScrollBar(), &QScrollBar::rangeChanged, this,
          [this]() {
            // This is triggered when the scroll bar is shown / hidden (and also
            // in other cases). Update the minimum size in this case (because
            // the additional space for the scroll bar might change).

            // qDebug() << "rangeChanged";

            updateSize();
          });
}

bool vx::HorizontalScrollArea::eventFilter(QObject* o, QEvent* e) {
  // This works because QScrollArea::setWidget installs an eventFilter on the
  // widget
  if (o && o == widget() && e->type() == QEvent::Resize) updateSize();

  return QScrollArea::eventFilter(o, e);
}

void vx::HorizontalScrollArea::updateSize() {
  // setMinimumWidth(widget()->minimumSizeHint().width() +
  // verticalScrollBar()->width());

  if (0)
    qDebug() << "Resize" << widget()->windowTitle()
             << widget()->minimumSizeHint().height()
             << horizontalScrollBar()->height() << this->minimumHeight()
             << this->height() << horizontalScrollBar()->isVisible();
  setMinimumHeight(widget()->minimumSizeHint().height() +
                   (horizontalScrollBar()->isVisible()
                        ? horizontalScrollBar()->height()
                        : 0));
  if (0) qDebug() << "new" << this->minimumHeight() << this->height();
}
