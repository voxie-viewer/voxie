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

#include <QObject>

#include <Voxie/Node/Object3DNode.hpp>

/**
 * @brief The CuttingDirection enum describes the three possible clipping
 * directions: No clipping, clipping the part behind the cutting plane or
 * clipping the part in front of the plane.
 * @author Robin Höchster
 */
enum CuttingDirection { Negative = -1, None = 0, Positive = 1 };

/**
 * @brief The CuttingMode enum describes the possible clipping modes.
 * @author Robin Höchster
 */
enum CuttingMode {
  // Hide the parts that are cut out by at least n cutting planes.
  // This is the default and recommended behaviour.
  AtLeast,
  // Cut away the parts that are cut out by all cutting planes but n.
  // Use this mode to cut holes into objects.
  AllBut
};

/**
 * @brief The Cutting class holds the @link CuttingMode and a corresponding
 * cutting limit. The combination of both determines if parts of an object are
 * hidden depending on how often that part is cut out by cutting planes.
 * Example: If the center part of an object shall be cut but the left side and
 * the right side should remain visible, in normal mode @link
 * CuttingMode::AtLeast this wouldnt work because the right and left side are
 * both cut out by the plane on the opposite side. To prevent this, the cutting
 * mode must set via @link Cutting::setMode() to @link CuttingMode::AllBut and
 * the limit must set to 0 (default value for @line CuttingMode::AllBut). This
 * means that a part is hidden, if it is cut out by all but 0 cutting planes, so
 * for the two limiting planes, it must be cut out by 2 planes to be hidden. The
 * left and right margin are only cut out by the one plane, so these parts
 * remain visible.
 * @author Robin Höchster
 */
class Cutting : public QObject {
  Q_OBJECT

  CuttingMode _mode = AtLeast;
  int _cuttingLimit = 1;

 public:
  /**
   * @brief isValidDirection checks if the given direction is valid.
   * @param direction
   * @return true if the direction is valid, false if not.
   */
  static bool isValidDirection(CuttingDirection direction) {
    switch (direction)
    case Positive:
    case Negative:
    case None:
      return true;
    return false;
  }

  /**
   * @brief setMode sets the cutting mode and an optional cutting limit. @see
   * Cutting for a more detailed description.
   * @param mode
   * @param cuttingLimit is the cutting limit. If the limit is less than zero,
   * it will be automatically reset to a default value fitting to the given
   * mode.
   */
  void setMode(CuttingMode mode, int cuttingLimit = -1) {
    if (cuttingLimit < 0) {
      if (mode == AtLeast)
        cuttingLimit = 1;
      else if (mode == AllBut)
        cuttingLimit = 0;
      else
        throw "Invalid cutting mode";
    }
    bool hasChanged = false;
    if (mode != _mode) {
      _mode = mode;
      hasChanged = true;
    }
    if (_cuttingLimit != cuttingLimit) {
      _cuttingLimit = cuttingLimit;
      hasChanged = true;
    }
    if (hasChanged) Q_EMIT changed();
  }

  /**
   * @brief setLimit sets the cutting limit.
   * @param cuttingLimit is the cutting limit. If the limit is less than zero,
   * it will be automatically reset to a default value fitting to the given
   * mode.
   */
  void setLimit(int cuttingLimit) {
    if (cuttingLimit < 0) {
      if (_mode == AtLeast)
        cuttingLimit = 0;
      else if (_mode == AllBut)
        cuttingLimit = 0;
      else
        throw "Invalid cutting mode";
    }
    if (_cuttingLimit == cuttingLimit) return;
    _cuttingLimit = cuttingLimit;
    Q_EMIT changed();
  }

  /**
   * @brief limit returns the cutting limit of this cutting setting.
   * @return
   */
  int limit() const { return _cuttingLimit; }

  /**
   * @brief mode returns the cutting mode of this cutting setting.
   * @return
   */
  int mode() const { return _mode; }

 Q_SIGNALS:
  /**
   * @brief changed is emitted if the cutting mode or the cutting limit has
   * changed.
   */
  void changed();
};
