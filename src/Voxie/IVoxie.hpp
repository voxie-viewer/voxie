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

#include <VoxieClient/Vector.hpp>

#include <QtCore/QProcess>

#include <QtWidgets/QLabel>
#include <QtWidgets/QWidget>

namespace vx {
class ExportedObject;
class HelpBrowserBackend;
class Extension;
class DataNode;
class ComponentContainer;
class ExtensionLauncher;
class Plugin;
class VisualizerNode;
enum class WindowMode;
class VolumeNode;
class RawNode;
class Slice;
class Node;
namespace io {
class Operation;
class Importer;
}  // namespace io
class VOXIECORESHARED_EXPORT ActiveVisualizerProvider : public QObject {
  Q_OBJECT

 public:
  virtual vx::VisualizerNode* activeVisualizer() const = 0;
  virtual QList<Node*> selectedNodes() const = 0;
  virtual void setSelectedNodes(const QList<Node*>& nodes) const = 0;
  ~ActiveVisualizerProvider();

 Q_SIGNALS:
  void activeVisualizerChanged(vx::VisualizerNode* current);

  void nodeSelectionChanged(const QList<Node*>& selectedNodes);
  void customUiOpened(Node* widget);
  void customUiClosed(Node* widget);
};

class VOXIECORESHARED_EXPORT IDirectoryManager {
 public:
  virtual ~IDirectoryManager();

  virtual const QString& baseDir() const = 0;
  virtual const QList<QString>& pluginPath() const = 0;
  virtual const QList<QString>& scriptPath() const = 0;
  virtual const QList<QString>& extensionPath() const = 0;
  virtual const QString& pythonLibDir() const = 0;
  virtual const QList<QString>& additionalPythonLibDirs() const = 0;
  virtual const QList<QString>& allPythonLibDirs() const = 0;
  virtual const QString& pythonExecutable() const = 0;
  virtual const QString& docPrototypePath() const = 0;
  virtual const QString& docTopicPath() const = 0;
  virtual const QString& licensesPath() const = 0;
  virtual const QString& katexPath() const = 0;
  virtual const QString& simpleCssPath() const = 0;
};

class VOXIECORESHARED_EXPORT IVoxie {
 public:
  virtual ~IVoxie();

  virtual void registerVisualizer(vx::VisualizerNode* visualizer) = 0;

  /**
   * @brief Registers a side panel section.
   * @param section The section to be registered.
   * @param closeable If true, the section can be closed by the user.
   *
   * Registers and shows a section. Voxie takes ownership of the section.
   */
  // TODO: Remove this? Seems to be unused.
  virtual void registerSection(QWidget* section) = 0;

  virtual void registerNode(const QSharedPointer<vx::Node>& obj) = 0;

  virtual const QList<QSharedPointer<vx::Node>>& nodes() const = 0;

  /**
   * @brief Gets a vector with all loaded plugins.
   * @return
   */
  virtual QList<QSharedPointer<vx::Plugin>> plugins() const = 0;

  virtual bool disableOpenGL() const = 0;
  virtual bool disableOpenCL() const = 0;

  virtual ActiveVisualizerProvider* activeVisualizerProvider() const = 0;

  virtual QWidget* mainWindow() const = 0;

  virtual bool isHeadless() const = 0;

  /**
   * @brief Return the ScriptLauncher instance.
   */
  virtual QSharedPointer<vx::ExtensionLauncher> scriptLauncher() const = 0;

  /**
   * @brief Return the DirectoryManager instance.
   */
  virtual vx::IDirectoryManager* directoryManager() const = 0;

  virtual void createNodeConnection(Node* parent, Node* child,
                                    int slot) const = 0;

  virtual void handleLink(const QString& url) = 0;

  virtual vx::Vector<double, 2> getGraphPosition(vx::Node* obj) = 0;

  virtual void setGraphPosition(vx::Node* obj,
                                const vx::Vector<double, 2>& pos) = 0;

  virtual vx::Vector<double, 2> getVisualizerPosition(
      vx::VisualizerNode* obj) = 0;

  virtual void setVisualizerPosition(vx::VisualizerNode* obj,
                                     const vx::Vector<double, 2>& pos) = 0;

  virtual vx::Vector<double, 2> getVisualizerSize(vx::VisualizerNode* obj) = 0;

  virtual void setVisualizerSize(vx::VisualizerNode* obj,
                                 const vx::Vector<double, 2>& size) = 0;

  virtual WindowMode getVisualizerWindowMode(vx::VisualizerNode* obj) = 0;
  virtual void setVisualizerWindowMode(vx::VisualizerNode* obj,
                                       WindowMode mode) = 0;

  virtual bool isAttached(vx::VisualizerNode* obj) = 0;

  virtual void setIsAttached(vx::VisualizerNode* obj, bool value) = 0;

  virtual void registerHelpBrowserBackend(
      const QSharedPointer<HelpBrowserBackend>& backend) = 0;

  virtual void connectLinkHandler(QLabel* label) = 0;

  virtual QSharedPointer<vx::ExportedObject> toExportedObject(
      const QSharedPointer<vx::Extension>& extension) = 0;
  virtual QSharedPointer<vx::ExportedObject> toExportedObject(
      const QSharedPointer<vx::Plugin>& plugin) = 0;

  virtual void createDataNodeUI(vx::DataNode* obj) = 0;

  virtual QSharedPointer<ComponentContainer> components() = 0;

  virtual void openHelpForUri(const QString& uri) = 0;
  // TODO: Remove?
  virtual void openMarkdownString(const QString& uri, const QString& title,
                                  const QString& markdown,
                                  const QUrl& baseUrl) = 0;
  // TODO: Remove?
  virtual void openHtmlString(const QString& uri, const QString& title,
                              const QString& html, const QUrl& baseUrl) = 0;
};

void VOXIECORESHARED_EXPORT setVoxieRoot(IVoxie* voxie);

VOXIECORESHARED_EXPORT IVoxie& voxieRoot();

}  // namespace vx
