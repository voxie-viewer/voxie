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

#include <Main/Gui/VisualizerContainer.hpp>

#include <Voxie/IVoxie.hpp>
#include <Voxie/Node/Node.hpp>

#include <QtCore/QMap>
#include <QtCore/QProcess>

#include <QtDBus/QDBusAbstractAdaptor>
#include <QtDBus/QDBusObjectPath>
#include <QtDBus/QDBusPendingReply>

#include <QtWidgets/QDockWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMdiArea>
#include <QtWidgets/QVBoxLayout>

#include <QCloseEvent>

namespace vx {
class Root;
class VisualizerNode;
namespace io {
class Importer;
}
namespace gui {

class CoreWindow;
class SidePanel;

class ActiveVisualizerProviderImpl : public ActiveVisualizerProvider {
  Q_OBJECT

  CoreWindow* win;

 public:
  ActiveVisualizerProviderImpl(CoreWindow* win) : win(win) {}
  ~ActiveVisualizerProviderImpl();

  VisualizerNode* activeVisualizer() const override;
  QList<Node*> selectedNodes() const override;
  void setSelectedNodes(const QList<Node*>& nodes) const override;
};

/**
 * @brief The main window of voxie.
 */
class CoreWindow : public QMainWindow {
  Q_OBJECT

  friend class ::vx::Root;

 private:
  QMdiArea* mdiArea;
  QMenuBar* menuBar;
  QMenu* windowMenu;
  QMenu* scriptsMenu;
  int scriptsMenuStaticSize;
  QMenu* toolMenu = nullptr;
  int toolMenuStaticSize = 0;

  QList<VisualizerContainer*> visualizers;

  VisualizerNode* activeVisualizer;

  vx::ExportedObject* guiDBusObject;

  bool isBeginDestroyed;
  bool isTilePattern = false;
  bool sidePanelOnLeft = true;

  ActiveVisualizerProviderImpl activeVisualizerProvider;

 Q_SIGNALS:
  void activeVisualizerChanged(VisualizerNode* visualizer);

 public:
  SidePanel* sidePanel;

  VisualizerNode* getActiveVisualizer();

  vx::ExportedObject* getGuiDBusObject() { return guiDBusObject; }

  bool getTilePattern() { return isTilePattern; }
  void setTilePattern(bool value) {
    isTilePattern = value;
    if (isTilePattern && !toggleTabbedAction->isChecked()) {
      mdiArea->tileSubWindows();
    }
  }

  QMdiArea* getMdiArea() { return mdiArea; }

  QAction* toggleTabbedAction;

  void updateExtensions();

 private:
  /**
   * @brief Initializes the main menu.
   */
  void initMenu();

  /**
   * @brief Initializes the status bar.
   */
  void initStatusBar();

  /**
   * @brief Initializes the central widget.
   */
  void initWidgets(Root* root);

  /**
   * @brief Destroys all currently loaded nodes
   * User gets asked to confirm this action first if any datasets are loaded.
   *
   * @return True if a new session was created or if no datasets were loaded in
   * the first place. False otherwise.
   */
  bool newSession();

  /**
   * @brief Closes all currently loaded nodes
   */
  void clearNodes();

  /**
   * @brief Shows the file dialog
   */
  void loadFile();

  /**
   * @brief Shows the dialog to save current setup as voxie project
   */
  bool saveProject();

  /**
   * @brief Shows the file dialog to select an existing voxie project or node
   * group to load.
   */
  void loadProject();

  /**
   * @brief Populates the script menu with script files.
   */
  void populateScriptsMenu();

 public:
  explicit CoreWindow(Root* root, QWidget* parent = 0);
  ~CoreWindow();

  /**
   * @brief Adds a visualizer to the window.
   * @param widget
   */
  void addVisualizer(VisualizerNode* visualizer);

  /**
   * @brief This overrides the close event, so that the user gets asked to save
   * before Voxie is closed
   * @param event
   */
  void closeEvent(QCloseEvent* event) override;

  bool isSidePanelOnLeft() { return sidePanelOnLeft; }
};

}  // namespace gui
}  // namespace vx
