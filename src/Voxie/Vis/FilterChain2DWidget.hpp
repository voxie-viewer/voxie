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

#include <VoxieBackend/Data/FloatImage.hpp>
#include <VoxieBackend/Data/SliceImage.hpp>

#include <Voxie/OldFilter/FilterChain2D.hpp>

#include <QtWidgets/QDialog>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QWidget>

namespace vx {
namespace visualization {

/**
 * Widget to display and modify a FilterChain2D
 */
class VOXIECORESHARED_EXPORT FilterChain2DWidget : public QWidget {
  Q_OBJECT

 private:
  vx::filter::FilterChain2D* filterchain;
  QThread* filterchainThread;
  QListWidget* list;
  QDialog* addDialog;
  QListWidget* filterToAdd;

 public:
  FilterChain2DWidget(QWidget* parent = 0);
  vx::filter::FilterChain2D* getFilterChain();

 public Q_SLOTS:
  void addFilter();
  void removeFilter();
  void moveFilterUp();
  void moveFilterDown();
  void updateList();
  void checkEnabled(QListWidgetItem* item);
  void openSettingsDialog();
  void openFiltermaskEditor();
  void applyFilter(vx::SliceImage slice);
  void exportFilterChain();
  void importFilterChain();
 Q_SIGNALS:
  void requestFilterMaskEditor(vx::filter::Filter2D* filter);
};
}  // namespace visualization
}  // namespace vx
