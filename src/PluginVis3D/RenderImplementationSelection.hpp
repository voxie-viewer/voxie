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

#ifndef RENDERIMPLEMENTATIONSELECTION_HPP
#define RENDERIMPLEMENTATIONSELECTION_HPP

#include <QWidget>

namespace Ui {
class RenderImplementationSelection;
}

class RenderImplementationSelection : public QWidget {
  Q_OBJECT

 public:
  explicit RenderImplementationSelection(QWidget* parent = 0);
  ~RenderImplementationSelection();

 public Q_SLOTS:
  void noOpenClAvailable();

 private Q_SLOTS:
  void on_cpuRadioButton_clicked(bool checked);
  void on_gpuRadioButton_clicked(bool checked);
 Q_SIGNALS:
  void renderImplChanged(bool useGPU);

 private:
  Ui::RenderImplementationSelection* ui;
  bool useGPU;
};

#endif  // RENDERIMPLEMENTATIONSELECTION_HPP
