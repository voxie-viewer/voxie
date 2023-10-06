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

#include <Voxie/PropertyObjects/PlaneNode.hpp>

#include <Voxie/Vis/QVecLineEdit.hpp>

#include <Main/Gui/PosRotValid.hpp>

#include <QtWidgets/QLineEdit>
#include <QtWidgets/QWidget>

namespace vx {
namespace gui {

class SliceView : public QWidget {
  Q_OBJECT
  // true= quaternion, false= Euler Angles
  bool rotationType;
  vx::PlaneNode* slice;
  // vx::visualization::QVecLineEdit* rotationEdit;
  QLineEdit* rotationEditX0;
  QLineEdit* rotationEditX1;
  QLineEdit* rotationEditX2;
  QLineEdit* rotationEditX3;
  // vx::visualization::QVecLineEdit* positionEdit;
  QLineEdit* positionEditX;
  QLineEdit* positionEditY;
  QLineEdit* positionEditZ;

 public:
  explicit SliceView(vx::PlaneNode* slice, QWidget* parent = 0);
  ~SliceView();

 Q_SIGNALS:

 public Q_SLOTS:

 private Q_SLOTS:
  void positionEdited();
  void rotationEdited();
  void sliceChanged();
  void changedRotation();
  void copyPosAndRotToClipboard();
  PosRotValid* analyseClipboardContent();
  void pastePos(PosRotValid* posRotValid);
  void pasteRot(PosRotValid* posRotValid);
};

}  // namespace gui
}  // namespace vx
