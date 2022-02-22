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

#include <QMouseEvent>
#include <Voxie/Voxie.hpp>

/**
 * @brief The MouseOperation class is the prototype class for a mouse operation.
 * A mouse operation represents an action performed on a specific mouse event,
 * for example a click. Ineriting classes must implement @link decideAction().
 * @author Robin Höchster
 */
class VOXIECORESHARED_EXPORT MouseOperation : public QObject {
  Q_OBJECT

 protected:
  /**
   * @brief allowViewMove Determines if moving of the view center point is
   * allowed.
   */
  bool allowViewMove;

 public:
  /**
   * @brief The Action enum contains the possible view and object control
   * actions that determine how mouse movements are interpreted.
   */
  enum Action {
    None,
    MoveObject,
    MoveView,
    RotateObject,
    RotateView,
    SelectObject,
    SetPoint
  };

  /**
   * @brief MouseOperation creates a new instance.
   * @param allowViewMove If true view moves are allowed, if false they are not
   * allowed.
   */
  MouseOperation(bool allowViewMove) : allowViewMove(allowViewMove) {}

  bool canMoveView() { return allowViewMove; }
  /**
   * @brief decideAction returns the target action to perform for the given
   * mouse event.
   * @param mouseEvent
   * @return
   */
  virtual Action decideAction(QMouseEvent* mouseEvent) = 0;
};
