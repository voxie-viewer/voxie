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

#include <QSharedPointer>
#include <Voxie/Vis/MouseOperation.hpp>

/**
 * @brief The SimpleMouseOperation class is a simple implementation of the @see
 * MouseOperation class.
 * @author Robin Höchster
 */
class VOXIECORESHARED_EXPORT SimpleMouseOperation : public MouseOperation {
 public:
  SimpleMouseOperation(bool allowViewMove);

  /**
   * @brief decideAction returns @link RotateView for all mouse events. If the
   * left mouse button is not pressed, @link Action::None is returned. If the
   * CTRL button is pressed and view moves are allowed, @link Action::MoveView
   * is returned.
   * @param mouseEvent
   * @return
   */
  MouseOperation::Action decideAction(QMouseEvent* mouseEvent) override {
    if (!(mouseEvent->buttons() & Qt::LeftButton)) return None;
    if (mouseEvent->modifiers().testFlag(Qt::ControlModifier)) {
      if (this->allowViewMove) return MoveView;
      return None;
    }
    return RotateView;
  }

  /**
   * @brief create Creates a new @link QSharedPointer containing a new @link
   * SimpleMouseOperation instance.
   * @param allowViewMove True if view moves are allowed, false if not.
   * @return
   */
  static QSharedPointer<SimpleMouseOperation> create(bool allowViewMove);
};
