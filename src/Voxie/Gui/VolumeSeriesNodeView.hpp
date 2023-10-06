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

#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QWidget>

namespace vx {
class VolumeSeriesNode;

namespace gui {

class VolumeSeriesNodeView : public QWidget {
  Q_OBJECT
 private:
  QVBoxLayout* splitLayout;
  vx::VolumeSeriesNode* node;

  QLineEdit* filenameLabel;

  QLabel* tagLabel;

  QFormLayout* form;
  QList<std::tuple<QString, QLabel*>> labels;
  void setValues(const QList<std::tuple<QString, QString>>& data);

  /**
   * @brief update updates the labels which may change due to the work of a
   * filter node to the most recent values
   */
  void update();

 public:
  explicit VolumeSeriesNodeView(vx::VolumeSeriesNode* node,
                                QWidget* parent = 0);
  ~VolumeSeriesNodeView();
};

}  // namespace gui
}  // namespace vx
