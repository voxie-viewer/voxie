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

// QDBusConnection should be included as early as possible:
// https://bugreports.qt.io/browse/QTBUG-48351 /
// https://bugreports.qt.io/browse/QTBUG-48377
#include <QtDBus/QDBusConnection>

#include "CoreWindow.hpp"

#include <Voxie/Util.hpp>

#include <VoxieClient/DBusAdaptors.hpp>
#include <VoxieClient/DBusProxies.hpp>

#include <VoxieBackend/IO/OperationImport.hpp>
#include <VoxieBackend/IO/OperationResult.hpp>

#include <Main/Gui/AboutDialogWindow.hpp>
#include <Main/Gui/ButtonLabel.hpp>
#include <Main/Gui/OpenFileDialog.hpp>
#include <Main/Gui/PluginManagerWindow.hpp>
#include <Main/Gui/PreferencesWindow.hpp>
#include <Main/Gui/ScriptConsole.hpp>
#include <Main/Gui/SidePanel.hpp>
#include <Main/Gui/SliceView.hpp>
#include <Main/Gui/VScrollArea.hpp>

#include <Main/DirectoryManager.hpp>
#include <Main/Root.hpp>

#include <Main/Component/SessionManager.hpp>

#include <Main/Help/HelpLinkHandler.hpp>

#include <Main/Component/ScriptLauncher.hpp>

#include <Voxie/Component/HelpCommon.hpp>
#include <Voxie/Component/Plugin.hpp>

#include <Voxie/Node/NodePrototype.hpp>

#include <VoxieClient/Exception.hpp>

#include <Voxie/Gui/ErrorMessage.hpp>

#include <Voxie/IO/SaveFileDialog.hpp>

#include <Voxie/Vis/VisualizerNode.hpp>

#include <VoxieClient/ObjectExport/BusManager.hpp>

#include <functional>

#include <assert.h>

#include <QCloseEvent>

#include <QtCore/QDebug>
#include <QtCore/QProcess>
#include <QtCore/QSettings>
#include <QtCore/QUrl>

#include <QtDBus/QDBusMessage>

#include <QtGui/QDesktopServices>
#include <QtGui/QScreen>
#include <QtGui/QWindow>

#include <QtWidgets/QDockWidget>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMdiSubWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>

using namespace vx::io;
using namespace vx::gui;
using namespace vx;
using namespace vx::plugin;
using namespace vx::visualization;
using namespace vx;

ActiveVisualizerProviderImpl::~ActiveVisualizerProviderImpl() {}

VisualizerNode* ActiveVisualizerProviderImpl::activeVisualizer() const {
  return win->getActiveVisualizer();
}

QList<Node*> ActiveVisualizerProviderImpl::selectedNodes() const {
  return win->sidePanel->selectedNodes();
}

void ActiveVisualizerProviderImpl::setSelectedNodes(
    const QList<Node*>& nodes) const {
  win->sidePanel->dataflowWidget->setSelectedNodes(nodes);
}

namespace vx {
namespace gui {

class Gui : public ExportedObject {
  friend class GuiAdaptorImpl;

  CoreWindow* window;

 public:
  Gui(CoreWindow* window);
  virtual ~Gui();
};

class GuiAdaptorImpl : public GuiAdaptor {
  Gui* object;

 public:
  GuiAdaptorImpl(Gui* object) : GuiAdaptor(object), object(object) {}
  virtual ~GuiAdaptorImpl() {}

  QList<QDBusObjectPath> selectedNodes() const override;
  void setSelectedNodes(const QList<QDBusObjectPath>& list) override;
  QList<QDBusObjectPath> selectedObjects() const override;

  QDBusObjectPath activeVisualizer() const override;

  void RaiseWindow(const QMap<QString, QDBusVariant>& options) override;

  qulonglong GetMainWindowID(
      const QMap<QString, QDBusVariant>& options) override;

  void setMdiViewMode(const QString& mdiViewMode) override {
    if (mdiViewMode == "de.uni_stuttgart.Voxie.MdiViewMode.SubWindow") {
      object->window->setTilePattern(false);
    } else if (mdiViewMode ==
               "de.uni_stuttgart.Voxie.MdiViewMode.SubWindowTiled") {
      object->window->setTilePattern(true);
    }
    bool isToggleChecked = object->window->toggleTabbedAction->isChecked();
    bool isTabbed =
        (mdiViewMode == "de.uni_stuttgart.Voxie.MdiViewMode.Tabbed");
    if ((isToggleChecked && !isTabbed) || (!isToggleChecked && isTabbed)) {
      object->window->toggleTabbedAction->trigger();
    }
  }

  QString mdiViewMode() const override {
    QString mode;
    if (object->window->getMdiArea()->viewMode() == QMdiArea::TabbedView) {
      mode = "de.uni_stuttgart.Voxie.MdiViewMode.Tabbed";
    } else if (object->window->getTilePattern()) {
      mode = "de.uni_stuttgart.Voxie.MdiViewMode.SubWindowTiled";
    } else {
      mode = "de.uni_stuttgart.Voxie.MdiViewMode.SubWindow";
    }
    return mode;
  }
};

}  // namespace gui
}  // namespace vx

Gui::Gui(CoreWindow* window)
    : ExportedObject("Gui", window, true), window(window) {
  new GuiAdaptorImpl(this);
  // https://bugreports.qt.io/browse/QTBUG-48008
  // https://randomguy3.wordpress.com/2010/09/07/the-magic-of-qtdbus-and-the-propertychanged-signal/
  connect(window, &CoreWindow::activeVisualizerChanged,
          [=](VisualizerNode* visualizer) {
            // http://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces-properties
            // org.freedesktop.DBus.Properties.PropertiesChanged (STRING
            // interface_name,
            //                                                    DICT<STRING,VARIANT>
            //                                                    changed_properties,
            //                                                    ARRAY<STRING>
            //                                                    invalidated_properties);
            QString propertyName = "ActiveVisualizer";
            auto value = ExportedObject::getPath(visualizer);
            QDBusMessage signal = QDBusMessage::createSignal(
                window->getGuiDBusObject()->getPath().path(),
                "org.freedesktop.DBus.Properties", "PropertiesChanged");
            signal << dbusMakeVariant<QString>("de.uni_stuttgart.Voxie.Gui")
                          .variant();
            QMap<QString, QDBusVariant> changedProps;
            changedProps.insert(propertyName,
                                dbusMakeVariant<QDBusObjectPath>(value));
            signal << dbusMakeVariant<QMap<QString, QDBusVariant>>(changedProps)
                          .variant();
            signal << dbusMakeVariant<QList<QString>>(QStringList()).variant();
            getBusManager()->sendEverywhere(signal);
          });
  connect(window->sidePanel, &SidePanel::nodeSelectionChanged, [=]() {
    // http://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces-properties
    // org.freedesktop.DBus.Properties.PropertiesChanged (STRING
    // interface_name,
    //                                                    DICT<STRING,VARIANT>
    //                                                    changed_properties,
    //                                                    ARRAY<STRING>
    //                                                    invalidated_properties);
    QString propertyName = "SelectedNodes";
    QString propertyNameOld = "SelectedObjects";
    QList<QDBusObjectPath> value;
    for (const auto& obj : window->sidePanel->selectedNodes())
      value << ExportedObject::getPath(obj);
    // qDebug() << "Selected nodes" << value;
    QDBusMessage signal = QDBusMessage::createSignal(
        window->getGuiDBusObject()->getPath().path(),
        "org.freedesktop.DBus.Properties", "PropertiesChanged");
    signal << dbusMakeVariant<QString>("de.uni_stuttgart.Voxie.Gui").variant();
    QMap<QString, QDBusVariant> changedProps;
    changedProps.insert(propertyName,
                        dbusMakeVariant<QList<QDBusObjectPath>>(value));
    changedProps.insert(propertyNameOld,
                        dbusMakeVariant<QList<QDBusObjectPath>>(value));
    signal
        << dbusMakeVariant<QMap<QString, QDBusVariant>>(changedProps).variant();
    signal << dbusMakeVariant<QStringList>(QStringList()).variant();
    getBusManager()->sendEverywhere(signal);
  });
}
Gui::~Gui() {}

QList<QDBusObjectPath> GuiAdaptorImpl::selectedNodes() const {
  try {
    QList<QDBusObjectPath> res;
    for (const auto& obj : object->window->sidePanel->selectedNodes())
      res << ExportedObject::getPath(obj);
    return res;
  } catch (Exception& e) {
    e.handle(object);
    return QList<QDBusObjectPath>();
  }
}
void GuiAdaptorImpl::setSelectedNodes(const QList<QDBusObjectPath>& list) {
  try {
    QList<Node*> nodes;
    for (const auto& path : list) {
      if (path.path() == "/") {
        throw Exception("de.uni_stuttgart.Voxie.Error",
                        "Node list contains nullptr");
      }
      auto ptr = ExportedObject::lookupWeakObject(path);
      if (!ptr) {
        throw vx::Exception("de.uni_stuttgart.Voxie.ObjectNotFound",
                            "Object " + path.path() + " not found");
      }
      auto obj = dynamic_cast<Node*>(ptr);
      if (!obj) {
        throw vx::Exception("de.uni_stuttgart.Voxie.InvalidObjectType",
                            "Object " + path.path() + " is not a Node");
      }
      nodes.append(obj);
    }
    object->window->sidePanel->dataflowWidget->setSelectedNodes(nodes);
  } catch (Exception& e) {
    e.handle(object);
  }
}
QList<QDBusObjectPath> GuiAdaptorImpl::selectedObjects() const {
  try {
    QList<QDBusObjectPath> res;
    for (const auto& obj : object->window->sidePanel->selectedNodes())
      res << ExportedObject::getPath(obj);
    return res;
  } catch (Exception& e) {
    e.handle(object);
    return QList<QDBusObjectPath>();
  }
}

QDBusObjectPath GuiAdaptorImpl::activeVisualizer() const {
  try {
    return ExportedObject::getPath(object->window->getActiveVisualizer());
  } catch (Exception& e) {
    e.handle(object);
    return ExportedObject::getPath(nullptr);
  }
}

void GuiAdaptorImpl::RaiseWindow(const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);
    object->window->activateWindow();
    object->window->raise();
  } catch (Exception& e) {
    e.handle(object);
  }
}

qulonglong GuiAdaptorImpl::GetMainWindowID(
    const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);

    return object->window->winId();
  } catch (Exception& e) {
    e.handle(object);
    return 0;
  }
}

CoreWindow::CoreWindow(vx::Root* root, QWidget* parent)
    : QMainWindow(parent),
      visualizers(),
      activeVisualizer(nullptr),
      isBeginDestroyed(false),
      activeVisualizerProvider(this) {
  connect(this, &CoreWindow::activeVisualizerChanged, this,
          [this](VisualizerNode* current) {
            Q_EMIT activeVisualizerProvider.activeVisualizerChanged(current);
          });
  this->setWindowTitle("Voxie");
  this->initMenu();
  this->initStatusBar();
  this->initWidgets(root);
  this->guiDBusObject = new Gui(this);

  this->setWindowIcon(QIcon(":/icons-voxie/voxel-data-32.png"));
}
CoreWindow::~CoreWindow() { isBeginDestroyed = true; }

void CoreWindow::insertPlugin(const QSharedPointer<Plugin>& plugin) {
  if (plugin->uiCommands().size() > 0) {
    QMenu* pluginEntry = this->pluginsMenu->addMenu(plugin->name());
    // Insert UI Actions
    QList<QAction*> pluginActions;
    for (QAction* action : plugin->uiCommands()) {
      pluginActions.append(action);
    }
    pluginEntry->addActions(pluginActions);
  }
}

void CoreWindow::addVisualizer(VisualizerNode* visualizer) {
  if (visualizer == nullptr) {
    return;
  }

  VisualizerContainer* container =
      new VisualizerContainer(this->mdiArea, visualizer);

  this->visualizers.append(container);
  connect(container, &QObject::destroyed, this, [=]() {
    if (isBeginDestroyed) return;
    this->visualizers.removeAll(container);
  });

  QAction* windowEntry =
      this->windowMenu->addAction(visualizer->icon(), container->windowTitle());
  connect(windowEntry, &QAction::triggered, container,
          &VisualizerContainer::activate);
  connect(container, &QWidget::destroyed, windowEntry, &QAction::deleteLater);
  connect(visualizer, &QWidget::destroyed, container,
          &VisualizerContainer::closeWindow);

  QMetaObject::Connection connection =
      connect(container, &VisualizerContainer::sidePanelVisiblityChanged,
              [=](bool visible) {
                // qDebug() << "VisualizerContainer::sidePanelVisiblityChanged"
                // << container << visualizer << visible;
                if (visible == false) return;
                if (this->activeVisualizer != visualizer) {
                  this->activeVisualizer = visualizer;
                  Q_EMIT activeVisualizerChanged(visualizer);
                }
                this->mdiArea->setActiveSubWindow(container->window);

                // This is disabled because it is also e.g. triggered when the
                // main window regains focus and causes another node to be
                // selected in this case
                //// update sidepanel to view data of selected visualizer
                // this->sidePanel->nodeTree->select(visualizer);
              });
  connect(container, &QObject::destroyed, this, [=]() {
    if (isBeginDestroyed) return;
    if (this->activeVisualizer == visualizer) {
      this->activeVisualizer = nullptr;
      Q_EMIT activeVisualizerChanged(nullptr);
    }
    disconnect(connection);
  });

  QVector<QWidget*> sections = visualizer->dynamicSections();
  for (QWidget* section : sections) visualizer->addPropertySection(section);
  visualizer->mainView()->setFocus();

  this->activeVisualizer = visualizer;
  Q_EMIT activeVisualizerChanged(visualizer);
}

VisualizerNode* CoreWindow::getActiveVisualizer() { return activeVisualizer; }

void CoreWindow::initMenu() {
  QMenuBar* menu = new QMenuBar(this);

  QMenu* fileMenu = menu->addMenu("&File");
  this->pluginsMenu = menu->addMenu("Plu&gins");
  this->scriptsMenu = menu->addMenu("&Scripts");
  {
    connect(scriptsMenu, &QMenu::aboutToShow, this,
            &CoreWindow::populateScriptsMenu);
  }
  this->windowMenu = menu->addMenu("&Window");
  QMenu* helpMenu = menu->addMenu("&Help");

  // Lets the user begin a new session by destroying all data nodes
  QAction* newAction =
      fileMenu->addAction(QIcon(":/icons/blue-document.png"), "&New...");
  newAction->setShortcut(QKeySequence("Ctrl+N"));
  connect(newAction, &QAction::triggered, this, &CoreWindow::newSession);

  QAction* openAction = fileMenu->addAction(
      QIcon(":/icons/blue-folder-horizontal-open.png"), "&Open…");
  openAction->setShortcut(QKeySequence("Ctrl+O"));
  connect(openAction, &QAction::triggered, this, &CoreWindow::loadFile);

  QAction* loadAction = fileMenu->addAction(
      QIcon(":/icons/folder-horizontal.png"), "&Load Voxie Project");
  loadAction->setShortcut(QKeySequence("Ctrl+L"));
  connect(loadAction, &QAction::triggered, this, [this] {
    if (newSession()) loadProject();
  });

  QAction* importAction = fileMenu->addAction(QIcon(":/icons/folder--plus.png"),
                                              "&Import Project/Node Group");
  importAction->setShortcut(QKeySequence("Ctrl+I"));
  connect(importAction, &QAction::triggered, this, &CoreWindow::loadProject);

  QAction* saveAction =
      fileMenu->addAction(QIcon(":/icons/disk.png"), "&Save Voxie Project");
  saveAction->setShortcut(QKeySequence("Ctrl+S"));
  connect(saveAction, &QAction::triggered, this, &CoreWindow::saveProject);

  fileMenu->addSeparator();

  QAction* addNodeAction =
      fileMenu->addAction(QIcon(":/icons/disk.png"), "Add n&ew node");
  addNodeAction->setShortcut(QKeySequence("Ctrl+Shift+E"));
  connect(addNodeAction, &QAction::triggered, this, [this]() {
    auto globalPos = QCursor::pos();
    sidePanel->dataflowWidget->setSelectedNodes({});
    sidePanel->showContextMenu(globalPos);
  });

  QAction* addNodeActionAsChild =
      fileMenu->addAction(QIcon(":/icons/disk.png"), "Add n&ew node as child");
  addNodeActionAsChild->setShortcut(QKeySequence("Ctrl+E"));
  connect(addNodeActionAsChild, &QAction::triggered, this, [this]() {
    auto globalPos = QCursor::pos();
    sidePanel->showContextMenu(globalPos);
  });

  fileMenu->addSeparator();
  QAction* preferencesAction =
      fileMenu->addAction(QIcon(":/icons/gear.png"), "&Preferences…");
  connect(preferencesAction, &QAction::triggered, this, [this]() -> void {
    PreferencesWindow* preferences = new PreferencesWindow(this);
    preferences->setAttribute(Qt::WA_DeleteOnClose);
    preferences->exec();
  });

  QAction* quitAction =
      fileMenu->addAction(QIcon(":/icons/cross.png"), "E&xit");
  quitAction->setShortcut(QKeySequence("Ctrl+Q"));
  connect(quitAction, &QAction::triggered, this, &QMainWindow::close);

  QAction* pluginManagerAction = this->pluginsMenu->addAction(
      QIcon(":/icons/plug.png"), "&Plugin Manager…");
  connect(pluginManagerAction, &QAction::triggered, this, [this]() -> void {
    PluginManagerWindow* pluginmanager = new PluginManagerWindow(this);
    pluginmanager->setAttribute(Qt::WA_DeleteOnClose);
    pluginmanager->exec();
  });
  this->pluginsMenu->addSeparator();

  scriptsMenuStaticSize = 0;

  QAction* scriptConsoleAction = this->scriptsMenu->addAction(
      QIcon(":/icons/application-terminal.png"), "&JS Console…");
  scriptsMenuStaticSize++;
  connect(scriptConsoleAction, &QAction::triggered, this, [this]() -> void {
    ScriptConsole* console = new ScriptConsole(this, "Voxie - JS Console");
    console->setAttribute(Qt::WA_DeleteOnClose);
    connect(console, &ScriptConsole::executeCode, this,
            [console](const QString& code) -> void {
              Root::instance()->exec(code, code,
                                     [console](const QString& text) {
                                       console->appendLine(text);
                                     });
            });
    console->show();
    console->activateWindow();
  });

  QAction* pythonConsoleAction = this->scriptsMenu->addAction(
      QIcon(":/icons/application-terminal.png"), "&Python Console…");
  scriptsMenuStaticSize++;
  pythonConsoleAction->setShortcut(QKeySequence("Ctrl+Shift+S"));
  connect(pythonConsoleAction, &QAction::triggered, this, [this]() -> void {
    auto service = Root::instance()->mainDBusService();
    if (!service) {
      QMessageBox(
          QMessageBox::Critical, Root::instance()->mainWindow()->windowTitle(),
          QString("Cannot launch python console because DBus is not available"),
          QMessageBox::Ok, Root::instance()->mainWindow())
          .exec();
      return;
    }

    ScriptConsole* console = new ScriptConsole(this, "Voxie - Python Console");
    console->setAttribute(Qt::WA_DeleteOnClose);
    // TODO: This should probably also use ScriptLauncher
    auto process = new QProcess();
    connect(console, &QObject::destroyed, process,
            &QProcess::closeWriteChannel);
    process->setProcessChannelMode(QProcess::MergedChannels);
    process->setWorkingDirectory(
        Root::instance()->directoryManager()->baseDir());
    process->setProgram(
        Root::instance()->directoryManager()->pythonExecutable());
    QStringList args;
    args << "-i";
    args << "-u";
    args << "-c";
    QString escapedPythonLibDirs = escapePythonStringArray(
        vx::Root::instance()->directoryManager()->allPythonLibDirs());
    args << "import sys; sys.path = " + escapedPythonLibDirs + " + sys.path; " +
                "import numpy as np; import voxie; args = "
                "voxie.parser.parse_args(); context = "
                "voxie.VoxieContext(args); instance = "
                "context.createInstance();";
    service->addArgumentsTo(args);
    process->setArguments(args);
    connect<void (QProcess::*)(int, QProcess::ExitStatus),
            std::function<void(int, QProcess::ExitStatus)>>(
        process, &QProcess::finished, console,
        std::function<void(int, QProcess::ExitStatus)>(
            [process, console](int exitCode,
                               QProcess::ExitStatus exitStatus) -> void {
              console->appendLine(
                  QString(
                      "Script host finished with exit status %1 / exit code %2")
                      .arg(exitStatus)
                      .arg(exitCode));
              process->deleteLater();
            }));
    auto isStarted = createQSharedPointer<bool>();
    connect(process, &QProcess::started, this,
            [isStarted]() { *isStarted = true; });
    connect(process, &QProcess::stateChanged, this,
            [process, isStarted](QProcess::ProcessState newState) {
              if (newState == QProcess::NotRunning && !*isStarted) {
                Root::instance()->log(
                    QString("Error while starting script host: %1")
                        .arg(process->error()));
                process->deleteLater();
              }
            });
    connect(process, &QProcess::readyRead, console,
            [process, console]() -> void {
              QString data = QString(process->readAll());
              console->append(data);
            });
    connect(console, &ScriptConsole::executeCode, process,
            [process, console](const QString& code) -> void {
              QString code2 = code + "\n";
              console->append(code2);
              // TODO: handle case when python process does not read input
              process->write(code2.toUtf8());
            });
    process->start();
    console->show();
    console->activateWindow();
  });
  // TODO: Clean up
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
  QAction* pythonConsoleTerminalAction = this->scriptsMenu->addAction(
      QIcon(":/icons/application-terminal.png"), "Python Console (&terminal)…");
  scriptsMenuStaticSize++;
  pythonConsoleTerminalAction->setShortcut(QKeySequence("Ctrl+Shift+T"));
  connect(
      pythonConsoleTerminalAction, &QAction::triggered, this, [this]() -> void {
        auto service = Root::instance()->mainDBusService();
        if (!service) {
          QMessageBox(
              QMessageBox::Critical,
              Root::instance()->mainWindow()->windowTitle(),
              QString(
                  "Cannot launch python console because DBus is not available"),
              QMessageBox::Ok, Root::instance()->mainWindow())
              .exec();
          return;
        }

        QStringList args;
#if !defined(Q_OS_MACOS)
        // TODO: clean up
        args << "--";
#endif
        args << Root::instance()->directoryManager()->pythonExecutable();
        args << "-i";
        args << "-u";
        args << "-c";
        QString escapedPythonLibDirs = escapePythonStringArray(
            vx::Root::instance()->directoryManager()->allPythonLibDirs());
        args << "import sys; sys.path = " + escapedPythonLibDirs +
                    " + sys.path; " +
                    "import numpy as np; import voxie; args = "
                    "voxie.parser.parse_args(); context = "
                    "voxie.VoxieContext(args); instance = "
                    "context.createInstance();";

        auto output = createQSharedPointer<QString>();
#if defined(Q_OS_MACOS)
        service->addArgumentsTo(args);

        QString command = escapeArguments(args);
        QString script = "tell application \"Terminal\" to do script " +
                         appleScriptEscape(command);
        qDebug() << script.toUtf8().data();
        QStringList osascriptArgs;
        osascriptArgs << "-e";
        osascriptArgs << script;

        auto process = new QProcess();
        Root::instance()->scriptLauncher()->setupEnvironment(
            process, false);  // Set PYTHONPATH etc.
        process->setProgram("osascript");
        process->setArguments(osascriptArgs);
        process->start();
#else
        auto process = Root::instance()->scriptLauncher()->startScript(
            "gnome-terminal", nullptr, args, new QProcess(), output, false);
#endif

    // TODO: Move parts to ScriptLauncher?
    // TODO: This should be done before the process is started
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
        QObject::connect(
            process, &QProcess::errorOccurred, Root::instance(),
            [](QProcess::ProcessError error) {
              QMessageBox(QMessageBox::Critical,
                          "Error while starting python console",
                          QString() + "Error while starting python console: " +
                              QVariant::fromValue(error).toString(),
                          QMessageBox::Ok)
                  .exec();
            });
#endif
        QObject::connect(
            process,
            static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(
                &QProcess::finished),
            Root::instance(),
            [output](int exitCode, QProcess::ExitStatus exitStatus) {
              if (exitStatus != QProcess::NormalExit || exitCode != 0) {
                QString scriptOutputString = *output;
                if (scriptOutputString != "")
                  scriptOutputString =
                      "\n\nScript output:\n" + scriptOutputString;
                QMessageBox(QMessageBox::Critical,
                            "Error while starting python console",
                            QString() +
                                "Error while starting python console: " +
                                QVariant::fromValue(exitStatus).toString() +
                                ", code = " + QString::number(exitCode) +
                                scriptOutputString,
                            QMessageBox::Ok)
                    .exec();
              } else {
                /*
                QString scriptOutputString = *output;
                if (scriptOutputString != "") {
                  QMessageBox(QMessageBox::Warning,
                              "Warnings while starting python console",
                              QString() +
                                  "Warnings while starting python console:\n" +
                                  scriptOutputString,
                              QMessageBox::Ok)
                      .exec();
                }
                */
              }
            });
      });
#endif
  this->scriptsMenu->addSeparator();
  scriptsMenuStaticSize++;

  QAction* cascadeAction = this->windowMenu->addAction(
      QIcon(":/icons/applications-blue.png"), "&Cascade");
  connect(cascadeAction, &QAction::triggered, [=]() -> void {
    this->mdiArea->cascadeSubWindows();
    isTilePattern = false;
  });
  QAction* tileAction = this->windowMenu->addAction(
      QIcon(":/icons/application-tile.png"), "&Tile");
  connect(tileAction, &QAction::triggered, [=]() -> void {
    this->mdiArea->tileSubWindows();
    isTilePattern = true;
  });
  QAction* fillAction = this->windowMenu->addAction(
      QIcon(":/icons/application-blue.png"), "&Fill");
  connect(fillAction, &QAction::triggered, [=]() -> void {
    QMdiSubWindow* subwindow = this->mdiArea->currentSubWindow();
    if (subwindow == nullptr) {
      return;
    }
    subwindow->setWindowState(subwindow->windowState() | Qt::WindowMaximized);
  });
  toggleTabbedAction = this->windowMenu->addAction(
      QIcon(":/icons/application-dock-tab.png"), "Ta&bbed");
  toggleTabbedAction->setCheckable(true);
  connect(toggleTabbedAction, &QAction::toggled, [=]() -> void {
    bool checked = toggleTabbedAction->isChecked();
    cascadeAction->setEnabled(!checked);
    tileAction->setEnabled(!checked);
    fillAction->setEnabled(!checked);
    if (checked) {
      this->mdiArea->setViewMode(QMdiArea::TabbedView);
    } else {
      this->mdiArea->setViewMode(QMdiArea::SubWindowView);
      this->setTilePattern(isTilePattern);
    }
  });
  this->windowMenu->addSeparator();

  QAction* helpShowAction =
      helpMenu->addAction(QIcon(":/icons/book-question.png"), "&Show help…");
  connect(helpShowAction, &QAction::triggered, []() -> void {
    Root::instance()->helpLinkHandler()->handleLink(
        vx::help::uriForHelpTopic("main"));
  });
  QAction* helpIndexAction =
      helpMenu->addAction(QIcon(":/icons/book-open-list.png"), "&Index…");
  connect(helpIndexAction, &QAction::triggered, []() -> void {
    Root::instance()->helpLinkHandler()->handleLink(
        vx::help::uriForHelp("index"));
  });
  QAction* oldManualAction =
      helpMenu->addAction(QIcon(":/icons/book-question.png"), "&Old manual…");
  connect(oldManualAction, &QAction::triggered, this, [this]() -> void {
    auto oldManualFile = Root::instance()->directoryManager()->oldManualFile();
    if (oldManualFile == "")
      QMessageBox(QMessageBox::Warning, this->windowTitle(),
                  "Old manual file not found.", QMessageBox::Ok, this)
          .exec();
    else
      QDesktopServices::openUrl(QUrl::fromLocalFile(oldManualFile));
  });
  QAction* homepageAction = helpMenu->addAction("&Homepage…");
  connect(homepageAction, &QAction::triggered, []() -> void {
    QDesktopServices::openUrl(
        QUrl("https://github.com/voxie-viewer/voxie", QUrl::TolerantMode));
  });
  QAction* aboutAction =
      helpMenu->addAction(QIcon(":/icons/information.png"), "&About…");
  connect(aboutAction, &QAction::triggered, this, [this]() -> void {
    AboutDialogWindow* aboutwindow = new AboutDialogWindow(this);
    aboutwindow->setAttribute(Qt::WA_DeleteOnClose);
    aboutwindow->exec();
  });

  this->setMenuBar(menu);
}

void CoreWindow::initStatusBar() {
  QStatusBar* statusBar = new QStatusBar(this);
  this->setStatusBar(statusBar);
}

void CoreWindow::initWidgets(vx::Root* root) {
  auto container = new QSplitter(Qt::Horizontal);
  {
    {
      this->mdiArea = new QMdiArea(this);
      this->mdiArea->setTabsClosable(true);

      sidePanel = new SidePanel(root, this);
      connect(sidePanel, &SidePanel::openFile, this, &CoreWindow::loadFile);

      connect(sidePanel, &SidePanel::nodeSelectionChanged, this, [this]() {
        Q_EMIT activeVisualizerProvider.nodeSelectionChanged(
            sidePanel->selectedNodes());
      });

      if (sidePanelOnLeft) {
        container->addWidget(sidePanel);
        container->addWidget(this->mdiArea);
        container->setCollapsible(1, false);
      } else {
        container->addWidget(this->mdiArea);
        container->setCollapsible(0, false);
        container->addWidget(sidePanel);
      }
    }
  }
  this->setCentralWidget(container);
}

void CoreWindow::populateScriptsMenu() {
  QList<QAction*> actions = this->scriptsMenu->actions();
  for (int i = this->scriptsMenuStaticSize; i < actions.size(); i++) {
    this->scriptsMenu->removeAction(actions.at(i));
  }

  for (auto scriptDirectory :
       Root::instance()->directoryManager()->scriptPath()) {
    QDir scriptDir = QDir(scriptDirectory);
    QStringList scripts =
        scriptDir.entryList(QStringList("*.js"), QDir::Files | QDir::Readable);
    for (QString script : scripts) {
      if (script.endsWith("~")) continue;
      QString scriptFile = scriptDirectory + "/" + script;

      if (QFileInfo(scriptFile).isExecutable()) continue;

      QString baseName = scriptFile;
      if (baseName.endsWith(".exe"))
        baseName = scriptFile.left(scriptFile.length() - 4);
      if (QFile::exists(baseName + ".json")) continue;

      QAction* action = this->scriptsMenu->addAction(
          QIcon(":/icons/script-attribute-j.png"), script);
      connect(action, &QAction::triggered, this, [scriptFile]() -> void {
        Root::instance()->execFile(scriptFile);
      });
    }
  }
  this->scriptsMenu->addSeparator();

  for (auto scriptDirectory :
       Root::instance()->directoryManager()->scriptPath()) {
    QDir scriptDir = QDir(scriptDirectory);
    QStringList scripts =
        scriptDir.entryList(QStringList("*"), QDir::Files | QDir::Readable);
    for (QString script : scripts) {
      if (script.endsWith("~")) continue;
      QString scriptFile = scriptDirectory + "/" + script;

      if (!QFileInfo(scriptFile).isExecutable()) continue;

      QString baseName = scriptFile;
      if (baseName.endsWith(".exe"))
        baseName = scriptFile.left(scriptFile.length() - 4);
      if (QFile::exists(baseName + ".json")) continue;

      QAction* action = this->scriptsMenu->addAction(
          QIcon(":/icons/script-attribute-e.png"), script);
      connect(action, &QAction::triggered, this, [scriptFile]() -> void {
        Root::instance()->scriptLauncher()->startScript(scriptFile);
      });
    }
  }
  this->scriptsMenu->addSeparator();

  {
    QStringList fileTypes;

    Root::instance()->settings()->beginGroup("scripting");
    int sizeExternals =
        Root::instance()->settings()->beginReadArray("externals");
    for (int i = 0; i < sizeExternals; ++i) {
      Root::instance()->settings()->setArrayIndex(i);
      fileTypes.append(Root::instance()
                           ->settings()
                           ->value("extension")
                           .toString()
                           .split(';'));
    }
    Root::instance()->settings()->endArray();
    Root::instance()->settings()->endGroup();
    if (!fileTypes.contains("*.py")) fileTypes << "*.py";

    for (auto scriptDirectory :
         Root::instance()->directoryManager()->scriptPath()) {
      QDir scriptDir = QDir(scriptDirectory);
      QStringList externalScripts =
          scriptDir.entryList(fileTypes, QDir::Files | QDir::Readable);
      for (QString script : externalScripts) {
        if (script.endsWith("~")) continue;
        QString scriptFile = scriptDirectory + "/" + script;

        if (QFileInfo(scriptFile).isExecutable()) continue;

        QString baseName = scriptFile;
        if (baseName.endsWith(".exe"))
          baseName = scriptFile.left(scriptFile.length() - 4);
        if (QFile::exists(baseName + ".json")) continue;

        QAction* action = this->scriptsMenu->addAction(
            QIcon(":/icons/script-attribute-p.png"), script);
        connect(
            action, &QAction::triggered, this, [this, scriptFile]() -> void {
              Root::instance()->settings()->beginGroup("scripting");
              int size =
                  Root::instance()->settings()->beginReadArray("externals");
              bool found = false;
              for (int i = 0; i < size; ++i) {
                Root::instance()->settings()->setArrayIndex(i);
                QString executable = Root::instance()
                                         ->settings()
                                         ->value("executable")
                                         .toString();
                for (const QString& ext : Root::instance()
                                              ->settings()
                                              ->value("extension")
                                              .toString()
                                              .split(';')) {
                  QRegExp regexp(ext, Qt::CaseInsensitive,
                                 QRegExp::WildcardUnix);
                  if (regexp.exactMatch(scriptFile) == false) continue;

                  Root::instance()->scriptLauncher()->startScript(scriptFile,
                                                                  &executable);
                  found = true;
                  break;
                }
                if (found) break;
              }
              if (!found && scriptFile.endsWith(".py")) {
                // startScript will handle python files
                Root::instance()->scriptLauncher()->startScript(scriptFile);
                found = true;
              }
              if (!found)
                QMessageBox(QMessageBox::Critical, this->windowTitle(),
                            QString("Failed to find interpreter for script " +
                                    scriptFile),
                            QMessageBox::Ok, this)
                    .exec();
              Root::instance()->settings()->endArray();
              Root::instance()->settings()->endGroup();
            });
      }
    }
  }
}

bool CoreWindow::newSession() {
  // Only show the confirmation dialog if there are actually any datasets loaded
  // right now
  if (Root::instance()->nodes().size() > 0) {
    // Ask the user for confirmation before removing everything that's currently
    // loaded
    QMessageBox::StandardButton confirmation;
    confirmation =
        QMessageBox::warning(this, "Voxie",
                             "Warning: This will close all currently loaded "
                             "nodes.\nAre you sure?",
                             QMessageBox::Yes | QMessageBox::No);

    // User selected Yes -> Destroy all datasets (and visualizers, slices)
    if (confirmation == QMessageBox::Yes) {
      clearNodes();
      return true;
    }
    return false;
  }

  return true;
}

void CoreWindow::clearNodes() {
  for (const auto& node : Root::instance()->nodes()) {
    node->destroy();
  }
}

void CoreWindow::loadFile() { new vx::OpenFileDialog(Root::instance(), this); }

/**
 * @brief This method opens a file dialog to let the user save all currently
 * loaded files and settigns as a voxie project The project is then saved in a
 * script file, which can be used to load it again
 */

bool CoreWindow::saveProject() {
  SessionManager* sessionManager = new SessionManager();

  vx::io::SaveFileDialog dialog(this, "Save as...", QString());
  dialog.addFilter(FilenameFilter("Voxie Project", {"*.vxprj.py"}), nullptr);
  dialog.setup();

  // TODO: This should be asynchronous
  if (dialog.exec() != QDialog::Accepted) return false;

  // Pass the selected filename and list of loaded datasets to the
  // SessionManager class, which handles saving and loading projects
  sessionManager->saveSession(dialog.selectedFiles().first());
  return true;
}

void CoreWindow::loadProject() {
  SessionManager* sessionManager = new SessionManager();

  QFileDialog dialog(this, "Select a Voxie Project or Node Group File...");
  dialog.setDefaultSuffix("vxprj.py");
  dialog.setAcceptMode(QFileDialog::AcceptOpen);
  dialog.setNameFilter("Voxie Project Files (*.vxprj.py *.ngrp.py)");
  dialog.setOption(QFileDialog::DontUseNativeDialog, true);
  if (dialog.exec() != QDialog::Accepted) return;
  QString filename = dialog.selectedFiles().first();

  sessionManager->loadSession(filename);
}

void CoreWindow::closeEvent(QCloseEvent* event) {
  // Skip the close dialog if there are no nodes
  // TODO: Also skip the close dialog if there was no change since the last save
  if (vx::Root::instance()->nodes().size() > 0) {
    QMessageBox closeDialog;
    closeDialog.setText("Do you want to save your current project?");
    closeDialog.setStandardButtons(QMessageBox::Save | QMessageBox::Discard |
                                   QMessageBox::Cancel);
    closeDialog.setDefaultButton(QMessageBox::Save);
    closeDialog.setWindowIcon(QIcon(":/icons-voxie/voxel-data-32.png"));
    int ret = closeDialog.exec();

    switch (ret) {
      case QMessageBox::Save:
        if (saveProject()) {
          closeDialog.close();
          clearNodes();
          event->accept();
          // Make sure the application quits if the main window is closed
          QCoreApplication::instance()->quit();
        } else {
          event->ignore();
        }
        break;
      case QMessageBox::Discard:
        closeDialog.close();
        clearNodes();
        event->accept();
        // Make sure the application quits if the main window is closed
        QCoreApplication::instance()->quit();
        break;
      case QMessageBox::Cancel:
        event->ignore();
        break;
      default:
        qWarning() << "Unknown return value for closeEvent";
        event->ignore();
        break;
    }

  } else {
    event->accept();
    // Make sure the application quits if the main window is closed
    QCoreApplication::instance()->quit();
  }
}
