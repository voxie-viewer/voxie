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

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QWidget>

namespace vx {
namespace gui {
namespace about {
/**
 * @brief The InformationTab class provides the "information" tab for the about
 * dialog
 * @author Tim Borner
 */
class InformationTab : public QWidget {
  Q_OBJECT
 public:
  explicit InformationTab(QWidget* parent = 0);
  ~InformationTab();

 private:
  QGridLayout* setupLayout();
  void setupElements(QGridLayout* layout);

  QLabel* lbl_name;
  QLabel* lbl_version;
  QLabel* lbl_author;
  QLabel* lbl_homepage;
  QLabel* lbl_documentation;

  QLabel* lbl_name_val;
  QLabel* lbl_version_val;
  QLabel* lbl_author_val;
  QLabel* lbl_homepage_val;
  QLabel* lbl_documentation_val;
};

}  // namespace about
}  // namespace gui
}  // namespace vx
