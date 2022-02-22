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

#include <VoxieBackend/lib/CL/cl-patched.hpp>

#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

namespace vx {
namespace gui {
namespace preferences {

class OpenclPreferences : public QWidget {
  Q_OBJECT
 public:
  explicit OpenclPreferences(QWidget* parent = 0);
  ~OpenclPreferences();

  static const QString defaultPlatformSettingsKey;
  static const QString defaultDevicesSettingsKey;

 private:
  QGridLayout* setupLayout();
  void setupElements(QGridLayout* layout);
  void updateDeviceToUseList(cl::Platform platform);

  // elements
  QLabel* lbl_currentPlatform;
  QComboBox* cmb_platformToUse;
  QListWidget* list_currentDevices;
  QListWidget* list_devicesToUse;
  QLabel* lbl_currentDeviceInfo;
  QLabel* lbl_deviceToUseInfo;
  QPushButton* btn_saveConfig;
  QLabel* lbl_needToRestart;

 Q_SIGNALS:

 private Q_SLOTS:
  void currentDeviceSelected(QListWidgetItem* item);
  void deviceToUseSelected(QListWidgetItem* item);
  void platformToUseSelected(int index);
  void saveConfig();
};

}  // namespace preferences
}  // namespace gui
}  // namespace vx

Q_DECLARE_METATYPE(cl::Device)
Q_DECLARE_METATYPE(cl::Platform)
