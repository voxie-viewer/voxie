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

#include <Voxie/Voxie.hpp>

#include <VoxieBackend/IO/FilenameFilter.hpp>

#include <QtWidgets/QFileDialog>

#include <memory>

namespace vx {
namespace io {

// A dialog for selecting the filename which will automatically append the
// correct extension (or will select the correct filetype if an extension is
// provided by the user)
class VOXIECORESHARED_EXPORT SaveFileDialog : public QFileDialog {
  Q_OBJECT

  struct Filter {
    FilenameFilter filter;
    // Use std::shared_ptr instead of QSharedPointer because QSharedPointer
    // doesn't support void
    std::shared_ptr<void> data;
    bool isDefault;
  };

  QList<Filter> filters;

  const Filter* currentFilter();

 public:
  SaveFileDialog(QWidget* parent, const QString& caption,
                 const QString& directory);
  ~SaveFileDialog();

  void addFilter(const FilenameFilter& filter,
                 const std::shared_ptr<void>& data, bool isDefault = false);

  void setup();

  std::shared_ptr<void> selectedFilterData();

 protected:
  void accept() override;
};

}  // namespace io
}  // namespace vx
