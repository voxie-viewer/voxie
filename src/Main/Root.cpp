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
// https://bugreports.qt.io/browse/QTBUG-48351
#include <QtDBus/QDBusConnection>

#include "Root.hpp"

#include <Main/DebugOptions.hpp>
#include <Main/DirectoryManager.hpp>
#include <Main/MetatypeRegistration.hpp>

#include <VoxieClient/DBusProxies.hpp>

#include <Main/Gui/DataNodeUI.hpp>
#include <Main/Gui/GraphNode.hpp>
#include <Main/Gui/GraphWidget.hpp>
#include <Main/Gui/SelectObjectConnection.hpp>
#include <Main/Gui/SidePanel.hpp>
#include <Main/Gui/SliceView.hpp>

#include <Main/IO/Load.hpp>

#include <Main/Vis/MarkdownVisualizer.hpp>

#include <VoxieBackend/Component/ComponentContainerList.hpp>
#include <VoxieBackend/Component/ComponentType.hpp>
#include <VoxieBackend/Component/ExtensionExporter.hpp>
#include <VoxieBackend/Component/ExternalOperation.hpp>

#include <Main/Component/ComponentTypes.hpp>
#include <Main/Component/ScriptLauncher.hpp>

#include <Voxie/Util.hpp>

#include <Voxie/Data/ContainerNode.hpp>
#include <Voxie/Data/EventListNode.hpp>
#include <Voxie/Data/FileNode.hpp>
#include <Voxie/Data/GeometricPrimitiveObject.hpp>
#include <Voxie/Data/SurfaceNode.hpp>
#include <Voxie/Data/TomographyRawDataNode.hpp>
#include <Voxie/Data/VolumeNode.hpp>
#include <Voxie/Data/VolumeSeriesNode.hpp>

#include <Voxie/Gui/ErrorMessage.hpp>

#include <Voxie/Vis/VisualizerNode.hpp>

#include <Voxie/IO/SaveFileDialog.hpp>
#include <VoxieBackend/IO/Operation.hpp>
#include <VoxieBackend/IO/OperationRegistry.hpp>
#include <VoxieBackend/IO/OperationResult.hpp>

#include <VoxieBackend/Data/BufferType.hpp>
#include <VoxieBackend/Data/GeometricPrimitive.hpp>
#include <VoxieBackend/Data/GeometricPrimitiveType.hpp>
#include <VoxieBackend/Data/VolumeDataBlock.hpp>
#include <VoxieBackend/Data/VolumeDataBlockJpeg.hpp>
#include <VoxieBackend/Data/VolumeDataVoxelInst.hpp>

#include <VoxieBackend/OpenCL/CLInstance.hpp>
#include <VoxieBackend/OpenCL/CLUtil.hpp>

#include <Voxie/Component/Plugin.hpp>

#include <Main/Instance.hpp>
#include <VoxieBackend/Component/Extension.hpp>

#include <VoxieClient/ObjectExport/BusManager.hpp>
#include <VoxieClient/ObjectExport/Client.hpp>
#include <VoxieClient/ObjectExport/ClientManager.hpp>
#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <VoxieClient/DBusTypeList.hpp>
#include <VoxieClient/Exception.hpp>
#include <VoxieClient/Format.hpp>

#include <Voxie/Data/TableNode.hpp>

#include <Voxie/PropertyObjects/PlaneNode.hpp>
#include <Voxie/PropertyObjects/PreviewBoxNode.hpp>

#include <Voxie/Node/NodeGroup.hpp>
#include <Voxie/Node/NodeTag.hpp>
#include <Voxie/Node/Types.hpp>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonValue>
#include <QtCore/QPluginLoader>
#include <QtCore/QProcess>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTextStream>
#include <QtCore/QtCoreVersion>

#include <QtDBus/QDBusAbstractAdaptor>
#include <QtDBus/QDBusArgument>
#include <QtDBus/QDBusInterface>

#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMessageBox>

#include <Main/Gui/HelpWindow.hpp>

#include <Main/Help/CMark.hpp>
#include <Main/Help/ExportHelpPages.hpp>
#include <Main/Help/HelpBrowserBackendQTextBrowser.hpp>

#include <Voxie/Component/HelpCommon.hpp>

#if !defined(Q_OS_WIN)
#include <sys/resource.h>
#include <sys/time.h>
#endif

using namespace vx;
using namespace vx::io;
using namespace vx::gui;
using namespace vx::plugin;
using namespace vx::visualization;

static Root* root = nullptr;

void runTests();

Root* Root::instance() { return ::root; }

void Root::createNodeConnection(Node* parent, Node* child, int slot) const {
  SelectNodeConnection::createNodeConnection(parent, child, slot);
}

class CorePluginInstance : public PluginInstance {
 protected:
  QList<QSharedPointer<vx::Component>> createComponents() override {
    QList<QSharedPointer<vx::Component>> components;

    // Types
    components << vx::allTypesAsComponents();

    // BufferTypes
    components.append(vx::ByteBuffer::bufferType());
    components.append(vx::VolumeDataBlock::BlockID::bufferType());
    components.append(vx::VolumeDataBlock::BlockOffsetEntry::bufferType());
    components.append(vx::VolumeDataBlockJpeg::BlockSize::bufferType());
    components.append(
        vx::VolumeDataBlockJpeg::HuffmanSymbolCounter::bufferType());

    // TODO: Also return node prototypes etc. here

    return components;
  }
};
class CorePlugin : public Plugin {
  VX_REFCOUNTEDOBJECT

 public:
  CorePlugin()
      : Plugin(new CorePluginInstance(), "Voxie Core", getComponentTypes()) {
    this->isCorePlugin_ = true;
  }
  ~CorePlugin() override {}

  // Make method public
  using ComponentContainer::setContainer;

  QList<QSharedPointer<vx::Component>> listComponents(
      const QSharedPointer<ComponentType>& componentType) override {
    // TODO: Treat these more generically?
    // TODO: Probably use CorePluginInstance::createComponents() instead

    if (componentType->name() ==
        "de.uni_stuttgart.Voxie.ComponentType.GeometricPrimitiveType") {
      QList<QSharedPointer<vx::plugin::Component>> result;
      for (const auto& entry : GeometricPrimitive::allTypes()->values())
        result << entry;
      return result;
    }

    return Plugin::listComponents(componentType);
  }

  QSharedPointer<vx::Component> getComponent(
      const QSharedPointer<ComponentType>& componentType, const QString& name,
      bool allowCompatibilityNames, bool allowMissing) override {
    // TODO: Treat these more generically?

    if (componentType->name() ==
        "de.uni_stuttgart.Voxie.ComponentType.GeometricPrimitiveType") {
      // TODO: Support allowCompatibilityNames?
      auto types = GeometricPrimitive::allTypes();
      if (!types->contains(name)) {
        if (allowMissing) return QSharedPointer<vx::Component>();
        throw Exception("de.uni_stuttgart.Voxie.ComponentNotFound",
                        "Could not find component '" + name + "' with type '" +
                            componentType->name() + "'");
      }
      return (*types)[name];
    }

    return Plugin::getComponent(componentType, name, allowCompatibilityNames,
                                allowMissing);
  }
};

class DebugEventFilter : public QObject {
 protected:
  bool eventFilter(QObject* object, QEvent* event) override {
    qDebug() << "Got event:" << object << event;
    return QObject::eventFilter(object, event);
  }
};

Root::Root(bool headless)
    : QObject(),
      coreWindow(nullptr),
      isHeadless_(headless),
      disableOpenGL_(false),
      disableOpenCL_(false) {
  voxieInstance = new Instance(this);
  new ClientManager();  // TODO: this should also be freed somewhere
  directoryManager_ = new DirectoryManager(this);
  scriptLauncher_ = makeSharedQObject<ScriptLauncher>(this);
  helpRegistry_ = createQSharedPointer<vx::help::HelpPageRegistry>();
  helpLinkHandler_ = createQSharedPointer<vx::help::HelpLinkHandler>(nullptr);

  connect(this, &Root::logEmitted, [](const QString& msg) {
    QDateTime now = QDateTime::currentDateTime();
    QTextStream(stderr) << "[" << now.toString(Qt::ISODate) << "] " << msg
                        << endl
                        << flush;
  });

  auto corePlugin = CorePlugin::create();
  this->corePlugin_ = corePlugin;

  factories_.append(VolumeNode::getPrototypeSingleton());
  factories_.append(VolumeSeriesNode::getPrototypeSingleton());
  factories_.append(ContainerNode::getPrototypeSingleton());
  factories_.append(SurfaceNode::getPrototypeSingleton());
  factories_.append(TableNode::getPrototypeSingleton());
  factories_.append(TomographyRawDataNode::getPrototypeSingleton());
  factories_.append(GeometricPrimitiveNode::getPrototypeSingleton());
  factories_.append(FileNode::getPrototypeSingleton());
  factories_.append(EventListNode::getPrototypeSingleton());

  factories_.append(PlaneNode::getPrototypeSingleton());
  factories_.append(PreviewBoxNode::getPrototypeSingleton());

  factories_.append(NodeGroup::getPrototypeSingleton());

  factories_.append(MarkdownVisualizer::getPrototypeSingleton());

  NodeTag::tagsFromJson(":/tags/tags.json");

  {
    QList<QSharedPointer<NodePrototype>> pr;
    for (const auto& ptr : factories_) pr << ptr;
    corePlugin->allObjectPrototypes = pr;
    corePlugin->reAddPrototypes();
  }

  // This means that PropertyTypes in built-in node prototypes cannot be
  // in plugins.
  auto coreContainer = ComponentContainerList::create(
      getComponentTypes(),
      QList<QSharedPointer<ComponentContainer>>{corePlugin});
  for (const auto& prototype : factories_) prototype->resolve1(coreContainer);
  for (const auto& prototype : factories_) prototype->resolve2(coreContainer);

  vx::debug_option::Log_QtEvents()->registerAndCallChangeHandler(
      this, [this](bool value) {
        if (value) {
          if (!this->debugEventFilter) {
            this->debugEventFilter = new DebugEventFilter();
            qApp->installEventFilter(this->debugEventFilter);
          }
        } else {
          if (this->debugEventFilter) {
            this->debugEventFilter->deleteLater();
            this->debugEventFilter = nullptr;
          }
        }
      });

  vx::debug_option::Log_FocusChanges()->registerAndCallChangeHandler(
      this, [this](bool value) {
        if (value) {
          if (!this->focusChangedDebugHandler) {
            this->focusChangedDebugHandler =
                connect(qApp, &QApplication::focusChanged, this,
                        [](QWidget* old, QWidget* newW) {
                          qDebug() << "Focus changed:" << old << newW;
                        });
          }
        } else {
          if (this->focusChangedDebugHandler) {
            QObject::disconnect(this->focusChangedDebugHandler);
            this->focusChangedDebugHandler = QMetaObject::Connection();
          }
        }
      });

  if (!headless) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    qApp->setDesktopFileName("de.uni_stuttgart.Voxie");
#endif
    this->coreWindow = new CoreWindow(this);
  }
}

Root::~Root() { delete this->coreWindow; }

static const size_t bufferMax = 1000;
static QString bufferedMessages[bufferMax];
static size_t bufferPos = 0;
static size_t bufferCount = 0;
static QMutex bufferMutex;
static QString msgTypeToString(QtMsgType type) {
  switch (type) {
    case QtDebugMsg:
      return "Debug";
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
    case QtInfoMsg:
      return "Info";
#endif
    case QtWarningMsg:
      return "Warning";
    case QtCriticalMsg:
      return "Critical";
    case QtFatalMsg:
      return "Fatal";
    default:
      return QString::number(type);
  }
}
static QtMessageHandler handler = nullptr;
static void myHandler(QtMsgType type, const QMessageLogContext& context,
                      const QString& msg) {
  (void)context;
  QString fullMsg = "[" + msgTypeToString(type) + "] " + msg;
  {
    QMutexLocker locker(&bufferMutex);
    bufferedMessages[bufferPos] = fullMsg;
    if (bufferPos + 1 > bufferCount) bufferCount = bufferPos + 1;
    bufferPos = (bufferPos + 1) % bufferMax;
  }
  if (::root) {
    Q_EMIT ::root->logEmitted(fullMsg);
  } else if (handler) {
    // handler(type, context, msg);
    QDateTime now = QDateTime::currentDateTime();
    QTextStream(stderr) << "[" << now.toString(Qt::ISODate) << "] " << fullMsg
                        << endl
                        << flush;
  } else {
    abort();
  }
}
QVector<QString> Root::getBufferedMessages() {
  QMutexLocker locker(&bufferMutex);
  QVector<QString> list(bufferCount);
  for (std::size_t i = 0; i < bufferCount; i++)
    list[i] =
        bufferedMessages[(bufferPos + bufferMax - bufferCount + i) % bufferMax];
  return list;
}

class FakeFilter : public FilterNode {
  // Q_OBJECT
  VX_REFCOUNTEDOBJECT

 public:
  FakeFilter(const QSharedPointer<NodePrototype>& prototype)
      : FilterNode(prototype) {
    // qDebug() << "FakeFilter";
  }
  ~FakeFilter() {
    // qDebug() << "~FakeFilter";
  }

 private:
  QSharedPointer<vx::io::RunFilterOperation> calculate(
      bool isAutomaticFilterRun) override {
    Q_UNUSED(isAutomaticFilterRun);
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Cannot call FakeFilter::calculate()");
  }
};
static QList<QSharedPointer<PropertyBase>> parsePropertiesImpl(
    const QJsonObject& json) {
  // TODO: Clean this up
  QJsonObject obj{
      {"Name", "de.uni_stuttgart.Voxie.FakeFilter"},
      {"NodeKind", "de.uni_stuttgart.Voxie.NodeKind.Filter"},
      {"Properties", json},
  };
  auto prototypePtr = createQSharedPointer<QSharedPointer<NodePrototype>>();
  auto prototype = NodePrototype::fromJson(
      obj,
      [prototypePtr](const QMap<QString, QVariant>&, const QList<Node*>&,
                     const QMap<QString, QDBusVariant>&,
                     vx::NodePrototype* prototype2,
                     bool) -> QSharedPointer<vx::Node> {
        Q_UNUSED(prototype2);  // Use *prototypePtr which is a shared ptr
        if (!*prototypePtr)
          throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                              "prototypePtr not set");

        // qDebug() << "New" << (*prototypePtr)->name();

        return FakeFilter::create(*prototypePtr);
      });
  *prototypePtr = prototype;
  // TODO: Clean this up, this probably also should call resolve2()
  auto coreContainer = ComponentContainerList::create(
      getComponentTypes(),
      QList<QSharedPointer<ComponentContainer>>{::root->corePlugin()});
  prototype->resolve1(coreContainer);

  QList<QSharedPointer<PropertyBase>> properties;
  for (const auto& property : prototype->nodeProperties()) {
    ::root->propertyFakeNodes[property] = prototype;
    properties << property;
  }
  return properties;
}

int Root::startVoxie(QCoreApplication& app, QCommandLineParser& parser,
                     bool headless) {
#if !defined(Q_OS_WIN)
  rlimit rlim;
  if (getrlimit(RLIMIT_NOFILE, &rlim)) {
    qWarning() << "getrlimit(RLIMIT_NOFILE, ...) failed:"
               << qt_error_string(errno);
  } else {
    rlim_t newLimit = std::max<rlim_t>(
        rlim.rlim_cur, std::min<rlim_t>(rlim.rlim_max, 1048576));
    if (newLimit != rlim.rlim_cur) {
      // qDebug() << "Raising RLIMIT_NOFILE from" << rlim.rlim_cur << "to" <<
      // newLimit;
      rlim.rlim_cur = newLimit;
      if (setrlimit(RLIMIT_NOFILE, &rlim)) {
        qWarning() << "setrlimit(RLIMIT_NOFILE, ...) failed:"
                   << qt_error_string(errno);
      }
    }
  }
#endif

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
  bool useBus = false;
#else
  bool useBus = true;
#endif
  if (parser.isSet("batch")) useBus = false;
  if (parser.isSet("dbus-p2p") && parser.isSet("dbus-bus")) {
    qCritical("Got both --dbus-p2p and --dbus-bus options");
    return 1;
  }
  if (parser.isSet("dbus-p2p")) useBus = false;
  if (parser.isSet("dbus-bus")) useBus = true;
  if (parser.isSet("opencl") && parser.isSet("no-opencl")) {
    qCritical("Got both --opencl and --no-opencl options");
    return 1;
  }

  handler = qInstallMessageHandler(myHandler);

  MetatypeRegistration::registerMetatypes();
  initDBusTypes();

  app.setOrganizationName("voxie");
  app.setApplicationName("voxie");

  if (::root != nullptr) {
    return -1;
  }

  QSharedPointer<DBusService> dbusService;

  if (!useBus) {
    dbusService = DBusServicePeer::create();
  } else {
    if (!QDBusConnection::sessionBus().isConnected()) {
      qDebug() << "Connecting to DBus session bus failed:"
               << QDBusConnection::sessionBus().lastError();

      dbusService = DBusServicePeer::create();
    } else {
      // qDebug() << "name" << QDBusConnection::sessionBus().name();
      getBusManager()->addConnection(makeSharedQObject<BusConnection>(
          QDBusConnection::sessionBus(), true));
      dbusService =
          makeSharedQObject<DBusServiceBus>(QDBusConnection::sessionBus());

      if (!QDBusConnection::sessionBus().registerService(
              "de.uni_stuttgart.Voxie") &&
          !parser.isSet("new-instance")) {
        qDebug() << "Voxie already running";  // TODO: print bus name?
        de::uni_stuttgart::Voxie::Instance iface("de.uni_stuttgart.Voxie",
                                                 "/de/uni_stuttgart/Voxie",
                                                 QDBusConnection::sessionBus());
        if (iface.isValid()) {
          iface.setTimeout(INT_MAX);
          int retval = 0;
          try {
            QDBusObjectPath guiPath = iface.gui();
            if (guiPath == QDBusObjectPath("/")) {
              qWarning("No remote GUI object found");
              retval = 1;
            } else {
              de::uni_stuttgart::Voxie::Gui gui_iface(
                  "de.uni_stuttgart.Voxie", guiPath.path(),
                  QDBusConnection::sessionBus());
              if (gui_iface.isValid()) {
                gui_iface.setTimeout(INT_MAX);
                HANDLEDBUSPENDINGREPLY(
                    gui_iface.RaiseWindow(QMap<QString, QDBusVariant>()));
              } else {
                qWarning(
                    "Failed to contact other voxie instance GUI over DBus");
                retval = 1;
              }
            }

            if (parser.isSet("run-script") &&
                parser.positionalArguments().size() != 0) {
              // TODO: Clean up

              auto directoryManager = new DirectoryManager();

              QStringList args = parser.positionalArguments();
              QString cmd = args[0];
              args.removeAt(0);

              auto process = new QProcess();

              // args.append("--voxie-bus-address=" + ...);
              QString busName =
                  "de.uni_stuttgart.Voxie";  // TODO: use unique name?
              args.append("--voxie-bus-name=" + busName);
              ScriptLauncher::setupEnvironment(directoryManager,
                                               process);  // Set PYTHONPATH

              process->setProcessChannelMode(QProcess::ForwardedChannels);
              process->setProgram(cmd);
              process->setArguments(args);

              QObject::connect<void (QProcess::*)(int, QProcess::ExitStatus)>(
                  process, &QProcess::finished, process, &QObject::deleteLater);

              process->start();
              process->waitForFinished(-1);
              // TODO: Display error message when process fails?
              if (process->exitCode() != 0) retval = process->exitCode();
            } else if (parser.positionalArguments().size() != 0) {
              auto directoryManager = new DirectoryManager();

              QList<QString> options;
              QMap<QString, QList<QString>> argOptions;
              QList<QString> allOptions = {"iso", "slice", "raw"};
              QList<QString> allArgOptions = {"import-property"};
              for (const auto& str : allOptions)
                if (parser.isSet(str)) options << str;
              for (const auto& str : allArgOptions)
                if (parser.isSet(str)) argOptions[str] = parser.values(str);
              QString escapedPythonLibDirs =
                  escapePythonStringArray(directoryManager->allPythonLibDirs());
              QString code =
                  "import sys; sys.path = " + escapedPythonLibDirs +
                  " + sys.path; " +
                  "import voxie, voxie.process_command_line_args; args = "
                  "voxie.parser.parse_args(); context = "
                  "voxie.VoxieContext(args); "
                  "instance = context.createInstance(); "
                  "voxie.process_command_line_args.process(instance, " +
                  vx::escapePythonString(QDir::currentPath()) + "," +
                  vx::escapePythonStringArray(parser.positionalArguments()) +
                  ", " + vx::escapePythonStringArray(options) + ", " +
                  vx::escapePython(argOptions) + "); context.client.destroy()";

              QStringList args;
              args << "-c";
              args << code;

              auto process = new QProcess();

              // args.append("--voxie-bus-address=" + ...);
              QString busName =
                  "de.uni_stuttgart.Voxie";  // TODO: use unique name?
              args.append("--voxie-bus-name=" + busName);
              // TODO: This is needed for when process_command_line_args starts
              // a .vxprj.py script but it doesn't help under Windows,
              // process_command_line_args should probably start python with
              // "-c 'import sys; sys.path.insert(0, ...'" under windows
              //
              // Same also applies when calling process_command_line_args for
              // the new voxie instance (there the ScriptLauncher::startScript()
              // default of true for setPythonLibDir is used)
              ScriptLauncher::setupEnvironment(directoryManager,
                                               process);  // Set PYTHONPATH

              process->setProcessChannelMode(QProcess::ForwardedChannels);
              process->setProgram(directoryManager->pythonExecutable());
              process->setArguments(args);

              QObject::connect<void (QProcess::*)(int, QProcess::ExitStatus)>(
                  process, &QProcess::finished, process, &QObject::deleteLater);

              process->start();
              process->waitForFinished(-1);
              // TODO: Display error message when process fails?
              if (process->exitCode() != 0) retval = process->exitCode();
            }
          } catch (Exception& e) {
            qCritical("Error during DBus call: %s: %s: %s",
                      e.additional().toUtf8().data(), e.name().toUtf8().data(),
                      e.message().toUtf8().data());
            retval = 1;
          }
          return retval;
        } else {
          qWarning("Failed to contact other voxie instance over DBus");
        }
      } else {
        qDebug() << "Voxie DBus connection:"
                 << QDBusConnection::sessionBus().baseService();
      }
    }
  }
  if (auto server = qSharedPointerDynamicCast<DBusServicePeer>(dbusService)) {
    qDebug() << "Started DBus server at" << server->server()->address();
  }

  ::root = new Root(headless);

  ::root->mainDBusService_ = dbusService;
  ::root->disableOpenGL_ = parser.isSet("no-opengl");
  // ::root->disableOpenCL_ = parser.isSet("no-opencl");
  ::root->disableOpenCL_ = !parser.isSet("opencl");

  setVoxieRoot(::root);

  ::root->initOpenCL();

  ::root->helpRegistry()->registerHelpPageDirectory(
      ::root->directoryManager()->docPrototypePath());

#if !defined(Q_OS_WIN)
  QString split = ":";
#else
  QString split = ";";
#endif
  for (QString path : ::root->directoryManager()->pluginPath()) {
    ::root->loadPlugins(path);
  }

  {
    QMap<QString, QSharedPointer<vx::NodePrototype>> map;
    for (const auto& prototype : ::root->factories_)
      map[prototype->name()] = prototype;

    for (const auto& plugin : ::root->plugins()) {
      if (plugin == ::root->corePlugin()) continue;
      // prototypes in plugins see only prototypes in the core plugin and in
      // itself
      auto container = ComponentContainerList::create(
          getComponentTypes(), QList<QSharedPointer<ComponentContainer>>{
                                   ::root->corePlugin(), plugin});
      for (const auto& prototype :
           plugin->listComponentsTyped<NodePrototype>()) {
        ::root->factories_.append(prototype);
      }
      for (const auto& prototype : plugin->listComponentsTyped<NodePrototype>())
        prototype->resolve1(container);
      for (const auto& prototype : plugin->listComponentsTyped<NodePrototype>())
        prototype->resolve2(container);
    }
  }

  QList<QSharedPointer<vx::NodePrototype>> scriptPrototypes;
  for (const auto& dirList :
       QList<QList<QString>>{// Don't get extensions from scriptPath
                             //::root->directoryManager()->scriptPath(), //
                             // TODO: Ignore .json files in scripts path?
                             ::root->directoryManager()->extensionPath()}) {
    for (const auto& dir : dirList) {
      auto extensions = vx::Extension::loadFromDir(
          dir, getComponentTypes(), ::root->scriptLauncher(),
          ::root->mainDBusService(), &parsePropertiesImpl);
      // qDebug() << "Loading extensions from" << dir;
      for (const auto& extension : extensions) {
        ::root->extensions_.append(extension);
        // qDebug() << "Got extension" << extension->scriptFilename();
        // TODO: Should this NodePrototype-specific code be here?
        for (const auto& prototype :
             extension->listComponentsTyped<NodePrototype>()) {
          // qDebug() << "Got prototype" << prototype->name();
          scriptPrototypes.append(prototype);
          ::root->factories_.append(prototype);

          // Register help page for prototype
          ::root->helpRegistry()->registerHelpPageFile(
              prototype->name(), extension->scriptFilename() + ".md");
        }
      }
    }
  }
  QList<QSharedPointer<ComponentContainer>> containers;
  containers << ::root->corePlugin();
  for (const auto& plugin : ::root->plugins_) containers << plugin;
  {
    auto container =
        ComponentContainerList::create(getComponentTypes(), containers);
    for (const auto& prototype : scriptPrototypes)
      prototype->resolve1(container);
    for (const auto& prototype : scriptPrototypes)
      prototype->resolve2(container);
  }

  {
    QSet<QString> foundNames;
    for (const auto& prototype : ::root->factories_) {
      QString name = prototype->name();
      if (foundNames.contains(name)) {
        ::root->prototypeMap_[name] = QSharedPointer<vx::NodePrototype>();
      } else {
        foundNames.insert(name);
        ::root->prototypeMap_[name] = prototype;
      }
    }
  }

  for (const auto& extension : ::root->extensions_) containers << extension;
  ::root->components_ =
      ComponentContainerList::create(getComponentTypes(), containers);

  // Run validateComponent() for all components
  for (const auto& type : *::root->components_->componentTypes())
    for (const auto& component : ::root->components_->listComponents(type))
      component->validateComponent(::root->components_);

  // Check whether all default exporters exist
  for (const auto& prototype : ::root->factories_) {
    if (prototype->nodeKind() != NodeKind::Data) continue;
    auto defaultExporterName = prototype->defaultExporterName();
    if (defaultExporterName == "") continue;
    try {
      ::root->components_->getComponentTyped<Exporter>(defaultExporterName,
                                                       false);
    } catch (vx::Exception& e) {
      qWarning() << "Error getting default exporter" << defaultExporterName
                 << "for prototype" << prototype->name() << ":" << e.message();
    }
  }

  if (!::root->helpBrowserBackend_) {
    // Install fallback help browser backend
    ::root->registerHelpBrowserBackend(
        makeSharedQObject<HelpBrowserBackendQTextBrowser>());
  }

  if (parser.isSet("output-help-directory")) {
    auto dir = parser.value("output-help-directory");

    vx::help::HelpPageGenerator pageGenerator(::root->helpRegistry());

    bool success = vx::help::exportHelpPages(::root, &pageGenerator, dir);

    return success ? 0 : 1;
  }

  for (const auto& dir : parser.values("debug-output-prototypes")) {
    for (const auto& prototype : ::root->factories()) {
      QJsonArray prototypes;
      prototypes << prototype->toJson();
      QJsonObject info;
      info["NodePrototype"] = prototypes;
      // qDebug() << qPrintable(QJsonDocument(info).toJson());
      QString name = prototype->name();
      if (name.startsWith("de.uni_stuttgart.Voxie."))
        name.remove(0, strlen("de.uni_stuttgart.Voxie."));
      QFile file(dir + "/" + name + ".json");
      qDebug() << "Writing" << file.fileName();
      if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
        qCritical() << "Error opening file" << file.fileName();
      } else {
        file.write(QJsonDocument(info).toJson());
      }
    }
  }

  if (::root->coreWindow) {
    QString mainWindowOption = "normal";
    if (parser.isSet("batch")) mainWindowOption = "hidden";
    if (parser.isSet("main-window"))
      mainWindowOption = parser.value("main-window");
    if (mainWindowOption == "normal") {
      ::root->coreWindow->show();
    } else if (mainWindowOption == "background") {
      ::root->coreWindow->setAttribute(Qt::WA_ShowWithoutActivating);
      ::root->coreWindow->show();
    } else if (mainWindowOption == "hidden") {
      // Do nothing
    } else if (mainWindowOption == "headless") {
      qCritical() << "--main-window option set to headless, but Root object "
                     "created with headless=false";
    } else {
      qWarning() << "Unkown value for --main-window option, known ones are: "
                    "normal, background, hidden";
      ::root->coreWindow->show();
    }

    ::root->coreWindow->updateExtensions();
  }

  OperationRegistry::instance()->registerNewRunningOperationHandler(
      ::root, [](const QSharedPointer<vx::io::Operation>& operation) {
        if (!::root->isHeadless())
          ::root->mainWindow()->sidePanel->addProgressBar(operation.data());
      });

  Importer::registerCreateNodeHandler(
      [](const QSharedPointer<OperationResultImport>& result,
         const QString& fileName, const QSharedPointer<Importer>& importer,
         const QMap<QString, QVariant>& properties) {
        Load::registerCreateNode(::root, result, fileName, importer,
                                 properties);
      });

  bool batchMode = parser.isSet("batch");
  if (parser.positionalArguments().size() != 0) {
    QString executable;
    QStringList args;
    if (!parser.isSet("run-script")) {
      QList<QString> options;
      QMap<QString, QList<QString>> argOptions;
      QList<QString> allOptions = {"iso", "slice", "raw"};
      QList<QString> allArgOptions = {"import-property"};
      for (const auto& str : allOptions)
        if (parser.isSet(str)) options << str;
      for (const auto& str : allArgOptions)
        if (parser.isSet(str)) argOptions[str] = parser.values(str);
      QString escapedPythonLibDirs = escapePythonStringArray(
          vx::Root::instance()->directoryManager()->allPythonLibDirs());
      QString code =
          "import sys; sys.path = " + escapedPythonLibDirs + " + sys.path; " +
          "import voxie, voxie.process_command_line_args; args = "
          "voxie.parser.parse_args(); context = voxie.VoxieContext(args); "
          "instance = context.createInstance(); "
          "voxie.process_command_line_args.process(instance, " +
          vx::escapePythonString(QDir::currentPath()) + "," +
          vx::escapePythonStringArray(parser.positionalArguments()) + ", " +
          vx::escapePythonStringArray(options) + ", " +
          vx::escapePython(argOptions) + "); context.client.destroy()";

      args << "-c";
      args << code;
      executable = Root::instance()->directoryManager()->pythonExecutable();
    } else {
      args = parser.positionalArguments();
      executable = args[0];
      args.removeAt(0);
    }

    auto scriptOutput = createQSharedPointer<QString>();
    auto process = Root::instance()->scriptLauncher()->startScript(
        executable, nullptr, args, new QProcess(), scriptOutput);

    // TODO: Move parts to ScriptLauncher?
    // TODO: This should be done before the process is started
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    QObject::connect(
        process, &QProcess::errorOccurred, Root::instance(),
        [](QProcess::ProcessError error) {
          if (!voxieRoot().isHeadless()) {
            QMessageBox(QMessageBox::Critical,
                        "Error while processing command line arguments",
                        QString() +
                            "Error while processing command line arguments: " +
                            QVariant::fromValue(error).toString(),
                        QMessageBox::Ok, Root::instance()->mainWindow())
                .exec();
          }
        });
#endif
    QObject::connect(
        process,
        static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(
            &QProcess::finished),
        Root::instance(),
        [scriptOutput, batchMode](int exitCode,
                                  QProcess::ExitStatus exitStatus) {
          if (exitStatus != QProcess::NormalExit || exitCode != 0) {
            if (batchMode) {
              qWarning() << "Failure while proccesing command line arguments";
              QCoreApplication::instance()->exit(1);
            }

            QString scriptOutputString = *scriptOutput;
            if (scriptOutputString != "")
              scriptOutputString = "\n\nScript output:\n" + scriptOutputString;
            if (!voxieRoot().isHeadless()) {
              QMessageBox(
                  QMessageBox::Critical,
                  "Error while processing command line arguments",
                  QString() +
                      "Error while processing command line arguments: " +
                      QVariant::fromValue(exitStatus).toString() + ", code = " +
                      QString::number(exitCode) + scriptOutputString,
                  QMessageBox::Ok, Root::instance()->mainWindow())
                  .exec();
            }
          } else {
            if (batchMode) {
              qDebug() << "Quitting after processing command line arguments";
              QCoreApplication::instance()->exit(0);
            }

            // TODO: Display script output here?
            /*
            QString scriptOutputString = *scriptOutput;
            if (scriptOutputString != "") {
            if (!voxieRoot().isHeadless()) {
              QMessageBox(
                  QMessageBox::Warning,
                  "Warnings while processing command line arguments",
                  QString() +
                      "Warnings while processing command line arguments:\n" +
                      scriptOutputString,
                  QMessageBox::Ok, Root::instance()->mainWindow())
                  .exec();
            }
            }
            */
          }
        });

    // TODO: Is there a race condition when the process finishes before this
    // line?
    // TODO: Is this needed or will the process delete itself anyway?
    QObject::connect<void (QProcess::*)(int, QProcess::ExitStatus)>(
        process, &QProcess::finished, process, &QObject::deleteLater);
  } else {
    if (batchMode) {
      qWarning() << "Batch mode is set but there are no command line arguments";
      vx::enqueueOnMainThread([]() { QCoreApplication::instance()->exit(0); });
    }
  }

  // TODO: Remote? Multiple pages?
  for (const auto& uri : parser.values("open-help-page")) {
    Root::instance()->helpWindow()->openHelpForUri(uri);
  }

  int result = app.exec();
  delete ::root;
  ::root = nullptr;
  return result;
}

void Root::loadPlugins(QString pluginDirectory) {
  QDir pluginDir = QDir(pluginDirectory);

  QStringList entries;
#if defined(Q_OS_WIN)
  QString libExt = "VoxiePlugin*.dll";
#elif defined(Q_OS_MACOS)
  QString libExt = "libVoxiePlugin*.dylib";
#else
  QString libExt = "libVoxiePlugin*.so";
#endif  //#ifdefined(Q_OS_WIN)

  entries = pluginDir.entryList(QStringList(libExt));
  for (QString entry : entries) {
    QPluginLoader* pluginLoader;
    QString dir;
    QString lib;

    lib = entry;

    QString name = lib;
#if defined(Q_OS_WIN)
    if (name.startsWith("VoxiePlugin")) name = name.mid(strlen("VoxiePlugin"));
    if (name.endsWith(".dll")) name = name.left(name.length() - strlen(".dll"));
#elif defined(Q_OS_MACOS)
    if (name.startsWith("libVoxiePlugin"))
      name = name.mid(strlen("libVoxiePlugin"));
    if (name.endsWith(".dylib"))
      name = name.left(name.length() - strlen(".dylib"));
#else
    if (name.startsWith("libVoxiePlugin"))
      name = name.mid(strlen("libVoxiePlugin"));
    if (name.endsWith(".so")) name = name.left(name.length() - strlen(".so"));
#endif

    lib = pluginDir.absolutePath() + "/" + lib;
    pluginLoader = new QPluginLoader(lib, this);

    if (pluginLoader->load()) {
      auto plugin =
          Plugin::create(pluginLoader->instance(), name, getComponentTypes());
      // TODO: This probably should not be done here
      for (auto importer : plugin->listComponentsTyped<Importer>()) {
        importer->setSelf(importer);
      }
      for (auto exporter : plugin->listComponentsTyped<Exporter>()) {
        exporter->setSelf(exporter);
      }

      plugins_ << plugin;
    } else {
      qDebug() << "Failed to load plugin '" + lib + "':"
               << pluginLoader->errorString();
      pluginLoader->deleteLater();
    }
  }
}

bool Root::initOpenCL() {
  if (disableOpenCL()) {
    // qWarning() << "OpenCL disabled from the command line";
    opencl::CLInstance::initialize(new opencl::CLInstance());
    return false;
  }
  qDebug() << "OpenCL support enabled";

  // TODO: Clean up
  QString platformName = "";
  QStringList deviceNames{};

  opencl::CLInstance* instance = nullptr;

  if (!platformName.isEmpty() && !deviceNames.isEmpty()) {
    try {
      auto platforms = opencl::getPlatformByName(platformName);
      cl::Platform platform;
      if (!platforms.isEmpty()) {
        platform = platforms[0];

        QVector<cl::Device> devices;
        for (QString deviceName : deviceNames) {
          devices += opencl::getDeviceByName(platform, deviceName);
        }
        if (!devices.isEmpty())
          instance = opencl::CLInstance::createInstance(
              opencl::createContext(devices));
      }
    } catch (opencl::CLException& ex) {
      qWarning() << "failed to load cl settings" << ex;
    }
  }

  // initialize/set default instances
  try {
    opencl::CLInstance::initialize(instance);
  } catch (opencl::CLException& ex) {
    qWarning()
        << "failed to properly init default CLInstances. Raised Exception: "
        << ex;
    return false;
  }
  return true;
}

void Root::registerVisualizer(VisualizerNode* visualizer) {
  if (!this->isHeadless()) this->mainWindow()->addVisualizer(visualizer);
}

// TODO: Remove this? Seems to be unused.
void Root::registerSection(QWidget* section) {
  if (!this->isHeadless()) this->mainWindow()->sidePanel->addSection(section);
}

void Root::registerNode(const QSharedPointer<vx::Node>& obj) {
  auto connection = createQSharedPointer<QMetaObject::Connection>();
  // TODO: Avoid putting obj into lambda?
  *connection = connect(obj.data(), &Node::stateChanged, this,
                        [this, obj, connection](Node::State newState) {
                          if (newState != Node::State::Destroyed) return;
                          nodes_.removeOne(obj);
                          // qDebug() << "Remaining node count" <<
                          // nodes_.count();
                          Q_EMIT nodeRemoved(obj.data());
                          // Needs to be disconnected to avoid keeping a
                          // reference to obj
                          if (connection) {
                            QObject::disconnect(*connection);
                          } else {
                            qCritical() << "connection is null";
                          }
                        });
  nodes_ << obj;
  Q_EMIT nodeAdded(obj.data());

  if (auto node = dynamic_cast<vx::PlaneNode*>(obj.data())) {  // TODO: clean up
    node->addPropertySection(new SliceView(node));
  }
}

void Root::handleLink(const QString& url) {
  helpLinkHandler()->handleLink(url);
}

QList<QSharedPointer<Plugin>> Root::plugins() const { return plugins_; }

const QList<QSharedPointer<NodePrototype>>& Root::factories() {
  return factories_;
}

void Root::log(const QString& str) { Q_EMIT this->logEmitted(str); }

void Root::quit(bool askForConfirmation) {
  if (askForConfirmation && !this->isHeadless())
    this->mainWindow()->close();
  else
    QCoreApplication::instance()->quit();
}

QSharedPointer<Plugin> Root::getPluginByName(const QString& name) {
  QSharedPointer<Plugin> result;
  for (const auto& plugin : root->plugins()) {
    if (name == plugin->name()) {
      if (result) {
        throw Exception("de.uni_stuttgart.Voxie.AmbiguousPluginName",
                        "Plugin name '" + name + "' is ambiguous");
      }
      result = plugin;
    }
  }

  if (!result)
    throw Exception("de.uni_stuttgart.Voxie.PluginNotFound",
                    "Could not find plugin '" + name + "'");
  return result;
}

void runTests() {
  qDebug() << "---- running tests ----";
  QSharedPointer<VolumeDataVoxel> data = VolumeDataVoxel::createVolume(
      {8, 8, 8}, DataType::Float32, {0, 0, 0}, {1, 1, 1});
  qDebug() << data->getDimensions().toQVector3D();
  qDebug() << data->getSize();
  data->performInGenericContext(
      [](auto& genericData) { qDebug() << genericData.calcMinMaxValue(); });
  // does this crash for you too?
  qDebug() << "---- tests done ----";
}

vx::Vector<double, 2> Root::getGraphPosition(vx::Node* obj) {
  if (isHeadless())
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Calling Root::getGraphPosition() in headless mode");
  if (mainWindow()->sidePanel->dataflowWidget->map.contains(obj)) {
    GraphNode* graphNode =
        mainWindow()->sidePanel->dataflowWidget->map.value(obj);
    if (graphNode) return pointToVector(graphNode->pos());
  }
  throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                      "Root::getGraphPosition(): Unable to find node");
}

void Root::setGraphPosition(vx::Node* obj, const vx::Vector<double, 2>& pos) {
  if (isHeadless())
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Calling Root::setGraphPosition() in headless mode");
  if (mainWindow()->sidePanel->dataflowWidget->map.contains(obj)) {
    GraphNode* graphNode =
        mainWindow()->sidePanel->dataflowWidget->map.value(obj);
    if (graphNode) graphNode->setPos(toQPoint(vectorCastNarrow<int>(pos)));
  }
}

vx::Vector<double, 2> Root::getVisualizerPosition(vx::VisualizerNode* obj) {
  if (isHeadless())
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.Error",
        "Calling Root::getVisualizerPosition() in headless mode");
  auto visContainer = coreWindow->getContainerForVisualizer(obj);
  if (!visContainer)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Unable to find visualizer container");
  return visContainer->getVisualizerPosition();
}

void Root::setVisualizerPosition(vx::VisualizerNode* obj,
                                 const vx::Vector<double, 2>& pos) {
  if (isHeadless())
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.Error",
        "Calling Root::setVisualizerPosition() in headless mode");
  auto visContainer = coreWindow->getContainerForVisualizer(obj);
  if (!visContainer)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Unable to find visualizer container");
  visContainer->setVisualizerPosition(pos);
}

WindowMode Root::getVisualizerWindowMode(vx::VisualizerNode* obj) {
  if (isHeadless())
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.Error",
        "Calling Root::getVisualizerWindowMode() in headless mode");
  auto visContainer = coreWindow->getContainerForVisualizer(obj);
  if (!visContainer)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Unable to find visualizer container");
  return visContainer->getWindowMode();
}

void Root::setVisualizerWindowMode(vx::VisualizerNode* obj, WindowMode mode) {
  if (isHeadless())
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.Error",
        "Calling Root::setVisualizerWindowMode() in headless mode");
  auto visContainer = coreWindow->getContainerForVisualizer(obj);
  if (!visContainer)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Unable to find visualizer container");
  visContainer->setWindowMode(mode);
}

vx::Vector<double, 2> Root::getVisualizerSize(vx::VisualizerNode* obj) {
  if (isHeadless())
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Calling Root::getVisualizerSize() in headless mode");
  auto visContainer = coreWindow->getContainerForVisualizer(obj);
  if (!visContainer)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Unable to find visualizer container");
  return visContainer->getVisualizerSize();
}

void Root::setVisualizerSize(vx::VisualizerNode* obj,
                             const vx::Vector<double, 2>& size) {
  if (isHeadless())
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Calling Root::setVisualizerSize() in headless mode");
  auto visContainer = coreWindow->getContainerForVisualizer(obj);
  if (!visContainer)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Unable to find visualizer container");
  visContainer->setVisualizerSize(size);
}

bool Root::isAttached(vx::VisualizerNode* obj) {
  if (isHeadless())
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Calling Root::isAttached() in headless mode");
  auto visContainer = coreWindow->getContainerForVisualizer(obj);
  if (!visContainer)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Unable to find visualizer container");
  return visContainer->isAttached;
}

void Root::setIsAttached(vx::VisualizerNode* obj, bool value) {
  if (isHeadless())
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Calling Root::setIsAttached() in headless mode");
  auto visContainer = coreWindow->getContainerForVisualizer(obj);
  if (!visContainer)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Unable to find visualizer container");
  if (value != visContainer->isAttached) {
    visContainer->switchPopState();
  }
}

void Root::registerHelpBrowserBackend(
    const QSharedPointer<HelpBrowserBackend>& backend) {
  vx::checkOnMainThread("Root::registerHelpBrowserBackend");
  if (helpBrowserBackend_) {
    qWarning() << "Calling Root::registerHelpBrowserBackend multiple times";
    return;
  }
  helpBrowserBackend_ = backend;
}

void Root::openHelpForUri(const QString& uri) {
  Root::instance()->helpWindow()->openHelpForUri(uri);
}

void Root::openMarkdownString(const QString& uri, const QString& title,
                              const QString& markdown, const QUrl& baseUrl) {
  // TODO: Clean up the help page opening code?
  try {
    QFile templateFile(":/Help/template.html");
    QString pageTemplate = "{0}";
    if (templateFile.open(QFile::ReadOnly | QFile::Text)) {
      pageTemplate = QString::fromUtf8(templateFile.readAll());
    } else {
      qWarning() << "Missing HTML help template";
    }

    auto html = vx::cmark::parseDocumentWithExtensions(markdown)->renderHtml();

    // TODO: Math formatting
    QString header = "";

    // TODO: Should probably use vformat()
    QString fullHtml = vx::format(pageTemplate.toUtf8().data(), html, header);

    openHtmlString(uri, title, html, baseUrl);
  } catch (vx::Exception& e) {
    vx::showErrorMessage("Got exception while rendering markdown data", e);
  }
}

void Root::openHtmlString(const QString& uri, const QString& title,
                          const QString& html, const QUrl& baseUrl) {
  Root::instance()->helpWindow()->openHtmlString(uri, title, html, baseUrl);
}

void Root::connectLinkHandler(QLabel* label) {
  label->setOpenExternalLinks(false);
  label->setTextInteractionFlags(Qt::TextBrowserInteraction);
  connect(label, &QLabel::linkActivated, this, &Root::handleLink);
}

gui::HelpWindow* Root::helpWindow() {
  if (!helpWindow_) helpWindow_ = new gui::HelpWindow;
  return helpWindow_;
}

QSharedPointer<vx::ExportedObject> Root::toExportedObject(
    const QSharedPointer<vx::Extension>& extension) {
  return extension;
}
QSharedPointer<vx::ExportedObject> Root::toExportedObject(
    const QSharedPointer<Plugin>& plugin) {
  return plugin;
}

void Root::createDataNodeUI(vx::DataNode* obj) {
  return vx::createDataNodeUI(obj);
}
