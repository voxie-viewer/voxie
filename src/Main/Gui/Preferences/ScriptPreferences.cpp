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

#include "ScriptPreferences.hpp"

#include <Main/Root.hpp>

#include <QtCore/QList>
#include <QtCore/QSettings>

#include <QtWidgets/QAction>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidgetItem>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QVBoxLayout>

using namespace vx::gui::preferences;

ScriptPreferences::ScriptPreferences(QWidget* parent) : QWidget(parent) {
  Root::instance()->settings()->beginGroup("scripting");
  int size = Root::instance()->settings()->beginReadArray("externals");
  for (int i = 0; i < size; ++i) {
    Root::instance()->settings()->setArrayIndex(i);
    this->scriptingTools.insert(
        Root::instance()->settings()->value("name").toString(),
        {Root::instance()->settings()->value("extension").toString(),
         Root::instance()->settings()->value("executable").toString(),
         Root::instance()->settings()->value("arguments").toString()});
  }
  Root::instance()->settings()->endArray();
  Root::instance()->settings()->endGroup();

  QVBoxLayout* mainlayout = new QVBoxLayout();
  {
    QToolBar* toolbar = new QToolBar();
    {
      QAction* add = toolbar->addAction(QIcon(":/icons/script--plus.png"),
                                        "Add scripting tool.");
      connect(add, &QAction::triggered, this, &ScriptPreferences::addNew);

      QAction* minus = toolbar->addAction(QIcon(":/icons/script--minus.png"),
                                          "Remove scripting tool.");
      connect(minus, &QAction::triggered, this,
              &ScriptPreferences::removeCurrent);

      QAction* dup = toolbar->addAction(QIcon(":/icons/scripts.png"),
                                        "Duplicate scripting tool.");
      connect(dup, &QAction::triggered, this,
              &ScriptPreferences::duplicateCurrent);
    }
    mainlayout->addWidget(toolbar);

    QHBoxLayout* listlayout = new QHBoxLayout();
    {
      this->list = new QListWidget();
      {
        this->updateList();
        connect(list, &QListWidget::itemSelectionChanged, this,
                &ScriptPreferences::updateData);
      }
      listlayout->addWidget(this->list);

      QVBoxLayout* rightlayout = new QVBoxLayout();
      {
        QFormLayout* form = new QFormLayout();
        {
          form->addRow("Name", (this->nameEdit = new QLineEdit()));
          form->addRow("Extension", (this->extensionEdit = new QLineEdit()));
          form->addRow("Executable", (this->executableEdit = new QLineEdit()));
          form->addRow("Arguments", (this->argsEdit = new QLineEdit()));
        }
        rightlayout->addLayout(form);

        QHBoxLayout* spaceLayout = new QHBoxLayout();
        {
          spaceLayout->addSpacerItem(
              new QSpacerItem(0, 0, QSizePolicy::Expanding));
          QPushButton* applyButton = new QPushButton("Apply");
          connect(applyButton, &QPushButton::clicked, this,
                  &ScriptPreferences::storeData);
          spaceLayout->addWidget(applyButton);
        }
        rightlayout->addLayout(spaceLayout);
      }
      listlayout->addLayout(rightlayout);
    }
    mainlayout->addLayout(listlayout);
  }
  this->setLayout(mainlayout);
}

ScriptPreferences::~ScriptPreferences() {}

/**
 * @brief Fills the list with the entries from the settings.
 */
void ScriptPreferences::updateList() {
  this->list->clear();
  for (const QString& key : this->scriptingTools.keys()) {
    this->list->addItem(key);
  }
}

/**
 * @brief Fills the data fields with the data from the selected entry.
 */
void ScriptPreferences::updateData() {
  QList<QListWidgetItem*> selection = this->list->selectedItems();
  if (selection.size() != 1) return;
  QString key = selection.at(0)->text();

  if (this->scriptingTools.contains(key)) {
    this->nameEdit->setText(key);
    this->nameEdit->setEnabled(false);

    SettingsEntry tool = this->scriptingTools[key];

    this->extensionEdit->setText(tool.extension);
    this->extensionEdit->setEnabled(true);

    this->executableEdit->setText(tool.executable);
    this->executableEdit->setEnabled(true);

    this->argsEdit->setText(tool.arguments);
    this->argsEdit->setEnabled(true);
  } else {
    this->nameEdit->setText("");
    this->nameEdit->setEnabled(false);

    this->extensionEdit->setText("");
    this->extensionEdit->setEnabled(false);

    this->executableEdit->setText("");
    this->executableEdit->setEnabled(false);

    this->argsEdit->setText("");
    this->argsEdit->setEnabled(false);
  }
}

/**
 * @brief Stores the data from the data form into the selected entry.
 */
void ScriptPreferences::storeData() {
  QList<QListWidgetItem*> selection = this->list->selectedItems();
  if (selection.size() == 1) {
    this->scriptingTools.remove(selection.at(0)->text());
  }

  QString key = this->nameEdit->text().trimmed();

  if (key.length() == 0) return;

  this->nameEdit->setEnabled(false);

  this->scriptingTools.insert(
      key, {this->extensionEdit->text(), this->executableEdit->text(),
            this->argsEdit->text()});

  this->save();
  this->updateList();
}

void ScriptPreferences::addNew() {
  this->nameEdit->setText("");
  this->extensionEdit->setText("");
  this->executableEdit->setText("");
  this->argsEdit->setText("");

  this->setEnabled(true, true);
  this->list->clearSelection();
}

void ScriptPreferences::removeCurrent() {
  QList<QListWidgetItem*> selection = this->list->selectedItems();
  if (selection.size() != 1) return;

  this->scriptingTools.remove(selection.at(0)->text());

  this->addNew();
  this->setEnabled(false, false);

  this->save();

  this->updateList();
  this->list->clearSelection();
}

void ScriptPreferences::duplicateCurrent() {
  this->updateData();
  this->list->clearSelection();
  this->nameEdit->setText(this->nameEdit->text() + " - Clone");
  this->setEnabled(true, true);
}

void ScriptPreferences::setEnabled(bool all, bool enableName) {
  this->nameEdit->setEnabled(enableName);
  this->extensionEdit->setEnabled(all);
  this->executableEdit->setEnabled(all);
  this->argsEdit->setEnabled(all);
}

void ScriptPreferences::save() {
  Root::instance()->settings()->beginGroup("scripting");
  Root::instance()->settings()->beginWriteArray("externals");
  int index = 0;
  for (const QString& key : this->scriptingTools.keys()) {
    Root::instance()->settings()->setArrayIndex(index++);
    Root::instance()->settings()->setValue("name", key);
    Root::instance()->settings()->setValue("extension",
                                           this->scriptingTools[key].extension);
    Root::instance()->settings()->setValue(
        "executable", this->scriptingTools[key].executable);
    Root::instance()->settings()->setValue("arguments",
                                           this->scriptingTools[key].arguments);
  }
  Root::instance()->settings()->endArray();
  Root::instance()->settings()->endGroup();
}
