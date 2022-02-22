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
 * @brief The IsosurfaceMouseOperation class implements the @link MouseOperation
 * for the Isosurface View. The IsosurfaceMouseOperation always allows view
 * moves.
 * @author Robin Höchster
 */
class IsosurfaceMouseOperation : public MouseOperation {
  Q_OBJECT

  Action preferedAction = RotateView;

 public:
  IsosurfaceMouseOperation();

  /**
   * @brief setPreferedAction sets the prefered @link Action to perform for an
   * mouse event.
   * @param action the prefered @link Action to perform.
   */
  void setPreferedAction(Action action);

  /**
   * @brief create Creates a new @link QSharedPointer containing a new @link
   * IsosurfaceMouseOperation instance.
   * @return
   */
  static QSharedPointer<IsosurfaceMouseOperation> create();

  /**
   * @brief getPreferedAction returns the current prefered @Action to perform on
   * a mouse event.
   * @return
   */
  Action getPreferedAction() { return preferedAction; }

  /**
   * @brief decideAction decides what the target action to perform for the given
   * mouse event.
   * @param mouseEvent
   * @return If the middle mouse button is pressed, @link Action::RotateView is
   * returned. If the previous conditions are not met and left button is not
   * pressed, @link Action::None is returned. If the previous conditions are not
   * met and the CTRL button is pressed, @link Action::MovieView is returned. If
   * none of the previous conditions are met, the currently prefered action is
   * returned.
   */
  MouseOperation::Action decideAction(QMouseEvent* mouseEvent) override {
    auto buttons = mouseEvent->buttons();
    // Middle mouse button is always view rotation
    if (buttons.testFlag(Qt::MiddleButton)) return RotateView;
    if (!buttons.testFlag(Qt::LeftButton)) return MouseOperation::Action::None;
    if (mouseEvent->modifiers().testFlag(Qt::ControlModifier)) return MoveView;
    // Otherwise return the prefered action:
    return preferedAction;
  }

 Q_SIGNALS:
  void preferedActionChanged();
};
