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

#include <QtCore/QList>

#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QWidget>

namespace vx {
namespace gui {
namespace preferences {

class ScriptPreferences : public QWidget {
  Q_OBJECT
 private:
  struct SettingsEntry {
    QString extension;
    QString executable;
    QString arguments;
  };

  QListWidget* list;
  QLineEdit* nameEdit;
  QLineEdit* extensionEdit;
  QLineEdit* executableEdit;
  QLineEdit* argsEdit;

  QMap<QString, SettingsEntry> scriptingTools;

 public:
  explicit ScriptPreferences(QWidget* parent = 0);
  ~ScriptPreferences();

 private:
  /**
   * @brief Fills the list with the entries from the settings.
   */
  void updateList();

  /**
   * @brief Fills the data fields with the data from the selected entry.
   */
  void updateData();

  /**
   * @brief Stores the data from the data form into the selected entry.
   */
  void storeData();

  /**
   * @brief Adds a new entry.
   */
  void addNew();

  /**
   * @brief Removes the currently selected entry.
   * @todo Implement message box.
   */
  void removeCurrent();

  /**
   * @brief Duplicates the current entry.
   */
  void duplicateCurrent();

  /**
   * @brief Sets the state of the edit fields.
   * @param all Enable state of all fields except the name edit.
   * @param enableName Enable state of the name edit.
   */
  void setEnabled(bool all, bool enableName);

  /**
   * @brief Saves the preferences.
   */
  void save();

 Q_SIGNALS:

 public Q_SLOTS:
};

}  // namespace preferences
}  // namespace gui
}  // namespace vx
