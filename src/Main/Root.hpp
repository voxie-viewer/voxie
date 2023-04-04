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

#include <Main/Gui/CoreWindow.hpp>

#include <Main/DirectoryManager.hpp>

#include <Voxie/IVoxie.hpp>

#include <VoxieBackend/Component/ComponentContainerList.hpp>

#include <VoxieBackend/Property/PropertyBase.hpp>

#include <Main/Component/ScriptLauncher.hpp>

#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <QtCore/QCommandLineParser>

#include <QtWidgets/QWidget>

int main(int argc, char* argv[]);

namespace vx {
class Instance;
class DBusService;
class ComponentContainer;
class VisualizerNode;
class ScriptLauncher;
class Extension;

class DirectoryManager;

namespace gui {
class HelpWindow;
}

namespace help {
class HelpPageRegistry;
class HelpLinkHandler;
}  // namespace help

/**
 * @brief The voxie root object.
 */
class Root : public QObject, public IVoxie {
  Q_OBJECT
  friend int ::main(int argc, char* argv[]);

 private:
  gui::CoreWindow* coreWindow;
  bool isHeadless_;
  gui::HelpWindow* helpWindow_ = nullptr;

  // TODO: remove, use components() instead?
  QList<QSharedPointer<vx::Plugin>> plugins_;

  /**
   * @brief The plugin containing everything which is in the shared library.
   */
  QSharedPointer<vx::Plugin> corePlugin_;

  Instance* voxieInstance;

  vx::DirectoryManager* directoryManager_;
  QSharedPointer<vx::ScriptLauncher> scriptLauncher_;

  QSharedPointer<DBusService> mainDBusService_;

  bool disableOpenGL_;
  bool disableOpenCL_;

  QList<QSharedPointer<vx::Node>> nodes_;

  // TODO: remove, use components() instead
  QList<QSharedPointer<vx::NodePrototype>> factories_;
  QMap<QString, QSharedPointer<vx::NodePrototype>> prototypeMap_;

  // TODO: remove, use components() instead?
  QList<QSharedPointer<vx::Extension>> extensions_;

  QSharedPointer<HelpBrowserBackend> helpBrowserBackend_;

  QSharedPointer<vx::help::HelpPageRegistry> helpRegistry_;
  QSharedPointer<vx::help::HelpLinkHandler> helpLinkHandler_;

  QSharedPointer<ComponentContainerList> components_;

  QObject* debugEventFilter = nullptr;
  QMetaObject::Connection focusChangedDebugHandler;

  explicit Root(bool headless);

  ~Root();

  /**
   * @brief Starts voxie within the given QApplication
   * @param app
   * @return App exit code.
   */
  static int startVoxie(QCoreApplication& app, QCommandLineParser& parser,
                        bool headless);

  void loadPlugins(QString pluginDirectory);

  bool initOpenCL();

  /**
   * @brief Perform the action when clicking on a link (e.g. select the
   * corresponding node in the node tree).
   */
  void handleLink(const QString& url) override;

 public:
  /**
   * @brief Gets the current voxie instance.
   * @return
   */
  static Root* instance();

  /**
   * @brief Return the DirectoryManager instance.
   */
  vx::DirectoryManager* directoryManager() const override {
    return directoryManager_;
  }

  void createNodeConnection(Node* parent, Node* child, int slot) const override;

  /**
   * @brief Return the ScriptLauncher instance.
   */
  QSharedPointer<vx::ExtensionLauncher> scriptLauncher() const override {
    return scriptLauncher_;
  }

  /**
   * @brief Return the QSettings instance.
   */
  // TODO: Create a new settings system which
  // - Can store values for extensions (probably based on Property mechanism,
  // store as JSON)
  // - Can store additional interpreters (especially for windows)
  // - Can store information about which OpenCL device to use
  // QSettings* settings() const { return settings_; }

  /**
   * @brief Registers a visualizer.
   * @param visualizer The visualizer to be registered.
   *
   * Registers and shows a visualizer. Voxie takes ownership of the visualizer.
   */
  virtual void registerVisualizer(VisualizerNode* visualizer) override;

  /**
   * @brief Registers a side panel section.
   * @param section The section to be registered.
   * @param closeable If true, the section can be closed by the user.
   *
   * Registers and shows a section. Voxie takes ownership of the section.
   */
  virtual void registerSection(QWidget* section,
                               bool closeable = false) override;

  void registerNode(const QSharedPointer<vx::Node>& obj) override;

  /**
   * @brief Gets the main window.
   * @return
   */
  gui::CoreWindow* mainWindow() const override { return this->coreWindow; }

  bool isHeadless() const override { return isHeadless_; }

  /**
   * @brief Returns the help window. If the help window does not yet exist, it
   * will be created.
   * @return
   */
  gui::HelpWindow* helpWindow();

  /**
   * @brief Gets a vector with all loaded plugins.
   * @return
   */
  virtual QList<QSharedPointer<vx::Plugin>> plugins() const override;

  QSharedPointer<vx::Plugin> corePlugin() { return corePlugin_; }

  /**
   * @brief Gets a list with all loaded node prototypes.
   * @return
   */
  // TODO: Remove, use components()->listComponentsTyped<NodePrototype>()
  // instead
  const QList<QSharedPointer<vx::NodePrototype>>& factories();

  // Note prototype names currently are not necessarily unique
  const QMap<QString, QSharedPointer<vx::NodePrototype>>& prototypeMap() const {
    return prototypeMap_;
  }

  const QList<QSharedPointer<vx::Extension>>& extensions() {
    return extensions_;
  }

  virtual bool disableOpenGL() const override { return disableOpenGL_; }
  virtual bool disableOpenCL() const override { return disableOpenCL_; }

  static QVector<QString> getBufferedMessages();

  ActiveVisualizerProvider* activeVisualizerProvider() const override {
    return &mainWindow()->activeVisualizerProvider;
  }

  const QList<QSharedPointer<vx::Node>>& nodes() const override {
    return nodes_;
  }

  QVector2D getGraphPosition(vx::Node* obj) override;

  void setGraphPosition(vx::Node* obj, const QVector2D& pos) override;

  QVector2D getVisualizerPosition(vx::VisualizerNode* obj) override;

  void setVisualizerPosition(VisualizerNode* obj,
                             const QVector2D& pos) override;

  QVector2D getVisualizerSize(VisualizerNode* obj) override;

  void setVisualizerSize(VisualizerNode* obj, const QVector2D& size) override;

  void setVisualizerWindowMode(vx::VisualizerNode* obj,
                               VisualizerWindowMode mode) override;

  bool isAttached(VisualizerNode* obj) override;

  void setIsAttached(VisualizerNode* obj, bool value) override;

  void registerHelpBrowserBackend(
      const QSharedPointer<HelpBrowserBackend>& backend) override;

  const QSharedPointer<HelpBrowserBackend>& helpBrowserBackend() const {
    return helpBrowserBackend_;
  }

 public:
  /**
   * @brief Quits voxie.
   */
  void quit(bool askForConfirmation);

  /**
   * @brief Logs a value.
   * @param str The string to be logged.
   */
  void log(const QString& str);

  // throws Exception
  QSharedPointer<vx::Plugin> getPluginByName(const QString& name);

  const QSharedPointer<help::HelpPageRegistry>& helpRegistry() {
    return helpRegistry_;
  }
  const QSharedPointer<help::HelpLinkHandler>& helpLinkHandler() {
    return helpLinkHandler_;
  }

  const QSharedPointer<ComponentContainerList>& allComponents() {
    return components_;
  }
  QSharedPointer<ComponentContainer> components() override {
    return components_;
  }

  void connectLinkHandler(QLabel* label) override;

  const QSharedPointer<DBusService>& mainDBusService() {
    return mainDBusService_;
  }

  QSharedPointer<vx::ExportedObject> toExportedObject(
      const QSharedPointer<vx::Extension>& extension) override;
  QSharedPointer<vx::ExportedObject> toExportedObject(
      const QSharedPointer<vx::Plugin>& plugin) override;

  void createDataNodeUI(vx::DataNode* obj) override;

  // TODO: clean up
  QMap<QSharedPointer<PropertyBase>, QSharedPointer<NodePrototype>>
      propertyFakeNodes;

 Q_SIGNALS:
  void logEmitted(const QString& text);
  void nodeAdded(vx::Node* obj);
  void nodeRemoved(vx::Node* obj);
};

}  // namespace vx
