#include "root.hpp"

// QDBusConnection should be included as early as possible: https://bugreports.qt.io/browse/QTBUG-48351
#include <QtDBus/QDBusConnection>

#include <Main/dbusproxies.hpp>
#include <Main/directorymanager.hpp>
#include <Main/metatyperegistration.hpp>
#include <Main/scriptwrapper.hpp>

#include <Main/gui/sidepanel.hpp>

#include <Main/gui/preferences/openclpreferences.hpp>

#include <Main/io/load.hpp>
#include <Main/io/loaderadaptor.hpp>

#include <Main/script/externaloperation.hpp>

#include <Voxie/data/image.hpp>

#include <Voxie/io/operation.hpp>

#include <Voxie/opencl/clinstance.hpp>
#include <Voxie/opencl/clutil.hpp>

#include <Voxie/plugin/voxieplugin.hpp>

#include <Voxie/scripting/client.hpp>
#include <Voxie/scripting/dbustypes.hpp>
#include <Voxie/scripting/scriptingcontainer.hpp>
#include <Voxie/scripting/scriptingexception.hpp>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonValue>
#include <QtCore/QPluginLoader>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QSettings>
#include <QtCore/QtCoreVersion>
#include <QtCore/QTextStream>

#include <QtDBus/QDBusAbstractAdaptor>
#include <QtDBus/QDBusArgument>
#include <QtDBus/QDBusInterface>

#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>

using namespace voxie;
using namespace voxie::io;
using namespace voxie::gui;
using namespace voxie::data;
using namespace voxie::plugin;
using namespace voxie::scripting;
using namespace voxie::visualization;

static Root *root = nullptr;

void runTests();

Root *Root::instance()
{
	return ::root;
}

Root::Root(QObject *parent) :
	QObject(parent),
	coreWindow(nullptr),
	jsEngine(),
    scriptWrapper(&jsEngine),
    disableOpenGL_(false),
    disableOpenCL_(false)
{
	voxieInstance = new VoxieInstance(this);
    directoryManager_ = new DirectoryManager(this);
    settings_ = new QSettings(QSettings::IniFormat, QSettings::UserScope, "voxie", "voxie", this);

    connect(this, &Root::logEmitted, [](const QString& msg) {
            QDateTime now = QDateTime::currentDateTime();
            QTextStream(stderr) << "[" << now.toString(Qt::ISODate) << "] " << msg << endl << flush;
        });

	this->coreWindow = new CoreWindow(this);

	this->pluginContainer = new QObject(this);
	this->pluginContainer->setObjectName("plugins");

	this->dataSetContainer = new QObject(this);
	this->dataSetContainer->setObjectName("dataSets");

	QScriptValue globalObject = this->jsEngine.globalObject();

    globalObject.setProperty("voxie", scriptWrapper.getWrapper(voxieInstance), QScriptValue::ReadOnly | QScriptValue::Undeletable);

    // Override normal print() function
    ScriptWrapper::addScriptFunction(globalObject, "print", QScriptValue::ReadOnly | QScriptValue::Undeletable, [this] (QScriptContext* context, QScriptEngine* engine) {
            QString str;
            for (int i = 0; i < context->argumentCount(); i++) {
                if (i > 0)
                    str += " ";
                str += context->argument(i).toString();
            }
            log(str);
            return engine->undefinedValue();
        });

    ScriptWrapper::addScriptFunction(globalObject, "describe", QScriptValue::ReadOnly | QScriptValue::Undeletable, [] (QScriptContext* context, QScriptEngine* engine) {
            Q_UNUSED(engine);
            if (context->argumentCount() != 1)
                return context->throwError(QString("Invalid number of arguments (expected one argument, the object to describe)"));
            QScriptValue obj = context->argument(0);
            QScriptValue description = obj.data().property("description");
            if (!description.isValid())
                description = obj.prototype().data().property("description");
            if (!description.isValid())
                return context->throwError(QString("Object does not have a description"));
            QString ret;
            ret += QString ("Description of object %1:\n").arg(obj.toString());
            ret += description.toString();
            return QScriptValue(ret);
        });

    this->jsEngine.collectGarbage();

    // This is a fake ExternalOperationLoad object to make sure that
    // scripts/getDBusInterfaces.py will pick up the methods in
    // ExternalOperation and ExternalOperationLoad
    auto fakeExternalOperation = createQSharedPointer<ExternalOperationLoad>(createQSharedPointer<Operation>());
    ScriptableObject::registerObject(fakeExternalOperation);
    // Make sure that the object stays until the Root object is destroyed
    connect(this, &QObject::destroyed, [fakeExternalOperation] () { });
}

Root::~Root()
{
	delete this->coreWindow;
}

static const size_t bufferMax = 1000;
static QString bufferedMessages[bufferMax];
static size_t bufferPos = 0;
static size_t bufferCount = 0;
static QMutex bufferMutex;
static QString msgTypeToString(QtMsgType type) {
    switch (type) {
    case QtDebugMsg: return "Debug";
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
    case QtInfoMsg: return "Info";
#endif
    case QtWarningMsg: return "Warning";
    case QtCriticalMsg: return "Critical";
    case QtFatalMsg: return "Fatal";
    default: return QString::number(type);
    }
}
static QtMessageHandler handler = nullptr;
static void myHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    QString fullMsg = "[" + msgTypeToString(type) + "] " + msg;
    {
        QMutexLocker locker(&bufferMutex);
        bufferedMessages[bufferPos] = fullMsg;
        if (bufferPos + 1 > bufferCount)
            bufferCount = bufferPos + 1;
        bufferPos = (bufferPos + 1) % bufferMax;
    }
    if (::root)
        emit ::root->logEmitted(fullMsg);
    else if (handler)
        handler(type, context, msg);
    else
        abort();
}
QVector<QString> Root::getBufferedMessages() {
    QMutexLocker locker(&bufferMutex);
    QVector<QString> list(bufferCount);
    for (std::size_t i = 0; i < bufferCount; i++)
        list[i] = bufferedMessages[(bufferPos + bufferMax - bufferCount + i) % bufferMax];
    return list;
}

int Root::startVoxie(QApplication &app, QCommandLineParser& parser)
{
    handler = qInstallMessageHandler(myHandler);

    MetatypeRegistration::registerMetatypes();
    initDBusTypes();

	app.setOrganizationName("voxie");
	app.setApplicationName("voxie");

	if(::root != nullptr)
	{
		return -1;
	}

    if (!parser.isSet("no-dbus")) {
        if (!QDBusConnection::sessionBus().isConnected()) {
            qDebug() << "Connecting to DBus session bus failed:" << QDBusConnection::sessionBus().lastError();
        } else {
            qDebug() << "Voxie DBus connection:" << QDBusConnection::sessionBus().baseService();
            if(!QDBusConnection::sessionBus().registerService("de.uni_stuttgart.Voxie") && !parser.isSet("new-instance")) {
                de::uni_stuttgart::Voxie::Voxie iface("de.uni_stuttgart.Voxie", "/de/uni_stuttgart/Voxie", QDBusConnection::sessionBus());
                if (iface.isValid()) {
                    iface.setTimeout (INT_MAX);
                    int retval = 0;
                    try {
                        QDBusObjectPath guiPath = iface.gui();
                        if (guiPath == QDBusObjectPath("/")) {
                            qWarning("No remote GUI object found");
                            retval = 1;
                        } else {
                            de::uni_stuttgart::Voxie::Gui gui_iface("de.uni_stuttgart.Voxie", guiPath.path(), QDBusConnection::sessionBus());
                            if (gui_iface.isValid()) {
                                gui_iface.setTimeout (INT_MAX);
                                HANDLEDBUSPENDINGREPLY(gui_iface.RaiseWindow(QMap<QString, QVariant>()));
                            } else {
                                qWarning("Failed to contact other voxie instance GUI over DBus");
                                retval = 1;
                            }
                        }
                        for (const QString& arg : parser.positionalArguments()) {
                            QDBusObjectPath isoVisualizer ("/");
                            if (parser.isSet("iso")) {
                                QDBusObjectPath plugin = HANDLEDBUSPENDINGREPLY(iface.GetPluginByName("Vis3D"));
                                de::uni_stuttgart::Voxie::Plugin pluginIface("de.uni_stuttgart.Voxie", plugin.path(), QDBusConnection::sessionBus());
                                pluginIface.setTimeout (INT_MAX);
                                isoVisualizer = HANDLEDBUSPENDINGREPLY(pluginIface.GetMemberByName("de.uni_stuttgart.Voxie.VisualizerFactory", "IsosurfaceMetaVisualizer"));
                                if (isoVisualizer.path() == "/") {
                                    qCritical("Failed to get isosurface visualizer factory");
                                    retval = 1;
                                }
                            }
                            QDBusObjectPath sliceVisualizer ("/");
                            if (parser.isSet("slice")) {
                                QDBusObjectPath plugin = HANDLEDBUSPENDINGREPLY(iface.GetPluginByName("VisSlice"));
                                de::uni_stuttgart::Voxie::Plugin pluginIface("de.uni_stuttgart.Voxie", plugin.path(), QDBusConnection::sessionBus());
                                pluginIface.setTimeout (INT_MAX);
                                sliceVisualizer = HANDLEDBUSPENDINGREPLY(pluginIface.GetMemberByName("de.uni_stuttgart.Voxie.VisualizerFactory", "SliceMetaVisualizer"));
                                if (sliceVisualizer.path() == "/") {
                                    qCritical("Failed to get slice visualizer factory");
                                    retval = 1;
                                }
                            }

                            QDBusObjectPath dataSet = HANDLEDBUSPENDINGREPLY(iface.OpenFile(QFileInfo(arg).absoluteFilePath(), QMap<QString, QVariant>()));
                            if (dataSet == QDBusObjectPath("/")) // User pressed 'Cancel'
                                continue;
                            de::uni_stuttgart::Voxie::DataSet dataSetIface("de.uni_stuttgart.Voxie", dataSet.path(), QDBusConnection::sessionBus());
                            dataSetIface.setTimeout (INT_MAX);
                            if (parser.isSet("slice")) {
                                QDBusObjectPath slice = HANDLEDBUSPENDINGREPLY(dataSetIface.CreateSlice(QMap<QString, QVariant>()));
                                if (sliceVisualizer.path() != "/") {
                                    QList<QDBusObjectPath> dataSets;
                                    QList<QDBusObjectPath> slices;
                                    slices.push_back(slice);
                                    de::uni_stuttgart::Voxie::VisualizerFactory factoryIface("de.uni_stuttgart.Voxie", sliceVisualizer.path(), QDBusConnection::sessionBus());
                                    factoryIface.setTimeout (INT_MAX);
                                    HANDLEDBUSPENDINGREPLY(factoryIface.Create(dataSets, slices, QMap<QString, QVariant>()));
                                }
                            }
                            if (isoVisualizer.path() != "/") {
                                QList<QDBusObjectPath> dataSets;
                                QList<QDBusObjectPath> slices;
                                dataSets.push_back(dataSet);
                                de::uni_stuttgart::Voxie::VisualizerFactory factoryIface("de.uni_stuttgart.Voxie", isoVisualizer.path(), QDBusConnection::sessionBus());
                                factoryIface.setTimeout (INT_MAX);
                                HANDLEDBUSPENDINGREPLY(factoryIface.Create(dataSets, slices, QMap<QString, QVariant>()));
                            }
                        }
                    } catch (ScriptingException& e) {
                        qCritical("Error during DBus call: %s: %s: %s", e.additional().toUtf8().data(), e.name().toUtf8().data(), e.message().toUtf8().data());
                        retval = 1;
                    }
                    return retval;
                } else {
                    qWarning("Failed to contact other voxie instance over DBus");
                }
            }
        }
    }

	::root = new Root();

    ::root->disableOpenGL_ = parser.isSet("no-opengl");
    ::root->disableOpenCL_ = parser.isSet("no-opencl");

	setVoxieRoot(::root);

	::root->initOpenCL();

#if !defined(Q_OS_WIN)
    QString split = ":";
#else
    QString split = ";";
#endif
    for (QString path : ::root->directoryManager()->pluginPath()) {
        ::root->loadPlugins(path);
    }

    //runTests();

	::root->coreWindow->show();

    for (const QString& arg : parser.positionalArguments()) {
        MetaVisualizer* isoVisualizer = nullptr;
        if (parser.isSet("iso")) {
            try {
                isoVisualizer = qobject_cast<MetaVisualizer*>(::root->getPluginByName("Vis3D")->getMemberByName("de.uni_stuttgart.Voxie.VisualizerFactory", "IsosurfaceMetaVisualizer"));
                if (!isoVisualizer)
                    qCritical("Failed to cast isosurface visualizer factory");
            } catch (ScriptingException& e) {
                qCritical("Error while getting isosurface visualizer factory: %s: %s", e.name().toUtf8().data(), e.message().toUtf8().data());
            }
        }
        MetaVisualizer* sliceVisualizer = nullptr;
        if (parser.isSet("slice")) {
            try {
                sliceVisualizer = qobject_cast<MetaVisualizer*>(::root->getPluginByName("VisSlice")->getMemberByName("de.uni_stuttgart.Voxie.VisualizerFactory", "SliceMetaVisualizer"));
                if (!sliceVisualizer)
                    qCritical("Failed to cast slice visualizer factory");
            } catch (ScriptingException& e) {
                qCritical("Error while getting slice visualizer factory: %s: %s", e.name().toUtf8().data(), e.message().toUtf8().data());
            }
        }
        auto op = voxie::io::Load::openFile(root, arg);
        connect(op, &LoadOperation::loadAborted, root, [arg] (QSharedPointer<voxie::scripting::ScriptingException> error) {
            qCritical("Failed to open file %s: %s: %s", arg.toUtf8().data(), error->name().toUtf8().data(), error->message().toUtf8().data());
            });
        if (parser.isSet("slice")) {
            connect(op, &LoadOperation::loadFinished, root, [sliceVisualizer] (voxie::data::DataSet* dataSet) {
                    auto slice = dataSet->createSlice();
                    if (sliceVisualizer) {
                        QVector<data::DataSet*> dataSets;
                        QVector<data::Slice*> slices;
                        slices.push_back(slice);
                        try {
                            sliceVisualizer->create(dataSets, slices);
                        } catch (voxie::scripting::ScriptingException& e) {
                            qCritical("Error while creating slice visualizer: %s: %s", e.name().toUtf8().data(), e.message().toUtf8().data());
                        }
                    }
                });
        }
        if (isoVisualizer) {
            connect(op, &LoadOperation::loadFinished, root, [isoVisualizer] (voxie::data::DataSet* dataSet) {
                    QVector<data::DataSet*> dataSets;
                    QVector<data::Slice*> slices;
                    dataSets.push_back(dataSet);
                    try {
                        isoVisualizer->create(dataSets, slices);
                    } catch (voxie::scripting::ScriptingException& e) {
                        qCritical("Error while creating isosurface visualizer: %s: %s", e.name().toUtf8().data(), e.message().toUtf8().data());
                    }
                });
        }
    }

	int result = app.exec();
	delete ::root; ::root = nullptr;
	return result;
}

void Root::loadPlugins(QString pluginDirectory)
{
    //qDebug() << "Searching for plugins in" << pluginDirectory;
	QDir pluginDir = QDir(pluginDirectory);

    QStringList entries;
#if defined(Q_OS_WIN)
    QString libExt = "VoxiePlugin*.dll";
#else
    QString libExt = "libVoxiePlugin*.so";
#endif //#ifdefined(Q_OS_WIN)

    entries = pluginDir.entryList(QStringList(libExt));
    for (QString entry : entries) {
      QPluginLoader *loader;
      QString dir;
      QString lib;

      lib = entry;

      QString name = lib;
#if defined(Q_OS_WIN)
      if (name.startsWith("VoxiePlugin"))
          name = name.mid(strlen("VoxiePlugin"));
      if (name.endsWith(".dll"))
          name = name.left(name.length() - strlen(".dll"));
#else
      if (name.startsWith("libVoxiePlugin"))
          name = name.mid(strlen("libVoxiePlugin"));
      if (name.endsWith(".so"))
          name = name.left(name.length() - strlen(".so"));
#endif

      lib = pluginDir.absolutePath() + "/" + lib;
      loader = new QPluginLoader(lib, this);

      if(loader->load()) {
          //qDebug() << loader->instance();

          VoxiePlugin *plugin = new VoxiePlugin(loader->instance(), this->pluginContainer, name);

          for(Importer *importer : plugin->importers()) {
              connect(importer, &Importer::dataLoaded, this, &Root::registerDataSet);
              connect(importer, &Importer::dataLoaded, this, &Root::registerDataObject);
          }

          for (auto loader : plugin->loaders()) {
              //qDebug() << loader << loader->parent();
              loader->setParent(nullptr);
              auto ptr = QSharedPointer<Loader>(loader, [](QObject* obj) { obj->deleteLater(); });
              ptr->setSelf(ptr);
              pluginLoaders.push_back(ptr);
          }

          this->mainWindow()->insertPlugin(plugin);
      } else {
          qDebug() << "Failed to load plugin '"+ lib + "':" << loader->errorString();
          loader->deleteLater();
      }
	}
}

void Root::addProgressBar(voxie::io::Operation* operation) {
    mainWindow()->sidePanel->addProgressBar(operation);
}

QObject* Root::createLoaderAdaptor(voxie::io::Loader* loader) {
    return new LoaderAdaptor(loader);
}


bool
Root::initOpenCL()
{
    if (disableOpenCL()) {
        qWarning() << "OpenCL disabled from the command line";
		opencl::CLInstance::initialize(new opencl::CLInstance());
        return false;
    }

    // load from qsettings
    using namespace gui::preferences;
    QString platformName = settings()->value(OpenclPreferences::defaultPlatformSettingsKey).value<QString>();
    QStringList deviceNames = settings()->value(OpenclPreferences::defaultDevicesSettingsKey).value<QStringList>();

    opencl::CLInstance* instance = nullptr;

    if(!platformName.isEmpty() && !deviceNames.isEmpty()){
        try {
            auto platforms = opencl::getPlatformByName(platformName);
            cl::Platform platform;
            if(!platforms.isEmpty()){
                platform = platforms[0];

                QVector<cl::Device> devices;
                for(QString deviceName : deviceNames){
                    devices += opencl::getDeviceByName(platform, deviceName);
                }
                if(!devices.isEmpty())
                    instance = opencl::CLInstance::createInstance(opencl::createContext(devices));
            }
        } catch(opencl::CLException& ex){
            qWarning() << "failed to load cl settings" << ex;
        }
    }
//	try {
//		auto cpus = opencl::getDevices(CL_DEVICE_TYPE_CPU);
////		if(!cpus.isEmpty())
////			instance = opencl::CLInstance::createInstance(cpus[0]);
//	} catch(opencl::CLException& ex){
//		qWarning() << ex;
//	}

	// initialize/set default instances
	try {
		opencl::CLInstance::initialize(instance);
		//qDebug() << "num devices in default clinstance" << opencl::CLInstance::getDefaultInstance()->getDevices().length();
	} catch(opencl::CLException& ex){
		qWarning() << "failed to properly init default CLInstances. Raised Exception: "<< ex;
		return false;
	}

	return true;
}

void Root::registerVisualizer(Visualizer *visualizer)
{
	this->mainWindow()->addVisualizer(visualizer);
}

void Root::registerSection(QWidget *section, bool closeable)
{
	this->mainWindow()->sidePanel->addSection(section, closeable);
}

void Root::registerDataSet(DataSet *dataSet)
{
	dataSet->setParent(this->dataSetContainer);
	this->mainWindow()->openDataSet(dataSet);
}

void Root::registerSlice(Slice *slice)
{
	this->mainWindow()->openSlice(slice);
}

void Root::registerDataObject(voxie::data::DataObject* obj) {
    connect(obj, &QObject::destroyed, this, [this, obj] () {
            dataObjects_.removeOne(obj);
            emit dataObjectRemoved(obj);
        });
    dataObjects_ << obj;
    emit dataObjectAdded(obj);
}

QVector<DataSet*> Root::dataSets() const
{
	QVector<DataSet*> dataSets;
    for(QObject *obj : this->dataSetContainer->findChildren<DataSet*>(QString(), Qt::FindDirectChildrenOnly))
	{
		dataSets.append(qobject_cast<DataSet*>(obj));
	}
	return dataSets;
}

QVector<VoxiePlugin*> Root::plugins() const
{
	QVector<VoxiePlugin*> plugins;
	for(QObject *obj : this->pluginContainer->findChildren<VoxiePlugin*>())
	{
		plugins.append(qobject_cast<VoxiePlugin*>(obj));
	}
	return plugins;
}

QScriptEngine& Root::scriptEngine()
{
	return this->jsEngine;
}

void Root::log(QScriptValue value)
{
    emit this->logEmitted(value.toString());
}

/*
static QString escapeJSString(const QString& str) {
    QJsonValue json(str);
    QJsonArray array;
    array.push_back(json);
    QJsonDocument doc(array);
    QString str2 = doc.toJson(QJsonDocument::Compact);
    str2.remove(0, 1);
    str2.remove(str2.length() - 1, 1);
    return str2;
}
*/

bool Root::execFile(const QString &fileName)
{
	QFile file(fileName);
	if(file.open(QIODevice::ReadOnly) == false)
	{
		this->log("Can't open file " + fileName);
		return false;
	}
	//this->log("Execute file: " + fileName);
	QTextStream stream(&file);
	QString code = stream.readAll();
    // Hide local variables defined in code
    code = "(function () { " + code + " }) ()";

	bool success = this->exec(code, "", [this](const QString& text) { log(text); });
	//bool success = this->exec(code, "voxie.ExecuteQScriptFile(" + escapeJSString(fileName) + ")");

	file.close();

	return success;
}

bool Root::exec(const QString& code, const QString& codeToPrint, const std::function<void(const QString&)>& print)
{
    if (codeToPrint != "")
        print("> " + codeToPrint);
	QScriptValue result = this->jsEngine.evaluate(code);
	//if(result.isError()) { // isError() only returns whether the object is an instance of the Error class, not whether there was an exception
	if(root->scriptEngine().hasUncaughtException()) {
		print("--- " + result.toString());
		return false;
	}
	if(!result.isUndefined()) {
        if(result.isArray()) {
            int length = result.property("length").toInteger();
            print("Array[" + QString::number(length) + "]");
            for(int i = 0; i < length; i++) {
                print(result.property(i).toString());
            }
        } else {
            print(result.toString());
        }
	}
	return true;
}

void Root::quit()
{
	this->mainWindow()->close();
}

VoxiePlugin* Root::getPluginByName (const QString& name) {
    auto plugins = root->plugins();

    VoxiePlugin* result = nullptr;
    for (VoxiePlugin* plugin : root->plugins ()) {
        if (name == plugin->name()) {
            if (result) {
                throw ScriptingException("de.uni_stuttgart.Voxie.AmbiguousPluginName", "Plugin name '" + name + "' is ambiguous");
            }
            result = plugin;
        }
    }

    if (!result)
        throw ScriptingException("de.uni_stuttgart.Voxie.PluginNotFound", "Could not find plugin '" + name + "'");
    return result;
}

void runTests()
{
    qDebug() << "---- running tests ----";

    QSharedPointer<VoxelData> data = VoxelData::create(8,8,8);
    qDebug() << data->getDimensions().toQVector3D();
    qDebug() << data->getSize();
    qDebug() << data->calcMinMaxValue();
    // does this crash for you too?

    qDebug() << "---- tests done ----";
}

VoxieInstance::VoxieInstance(Root* root) : ScriptableObject("", root, true, true), root (root) {
}
VoxieInstance::~VoxieInstance() {
}

QDBusObjectPath VoxieInstance::gui () {
    return voxie::scripting::ScriptableObject::getPath(root->mainWindow()->getGuiDBusObject());
}

QList<QDBusObjectPath> VoxieInstance::ListPlugins () {
  QList<QDBusObjectPath> paths;

  for (VoxiePlugin* plugin : root->plugins ()) {
    paths.append (voxie::scripting::ScriptableObject::getPath (plugin));
  }

  return paths;
}

QDBusObjectPath VoxieInstance::GetPluginByName (const QString& name) {
    try {
        return voxie::scripting::ScriptableObject::getPath (root->getPluginByName(name));
    } catch (ScriptingException& e) {
        e.handle(this);
        return voxie::scripting::ScriptableObject::getPath(nullptr);
    }
}

QStringList VoxieInstance::ListPluginMemberTypes () {
    VoxiePlugin plugin (nullptr, nullptr, "");
    QStringList types;
    for (QString type : plugin.getAllObjects().keys ())
        types.push_back (type);
    return types;
}

QList<QDBusObjectPath> VoxieInstance::ListDataSets () {
  QList<QDBusObjectPath> paths;

  for (DataSet* dataSet : root->dataSets ()) {
    paths.append (voxie::scripting::ScriptableObject::getPath (dataSet));
  }

  return paths;
}

QDBusObjectPath VoxieInstance::CreateClient (const QMap<QString, QVariant>& options) {
    try {
        voxie::scripting::ScriptableObject::checkOptions(options);
        QString name = "";
        if (calledFromDBus()) {
            name = message().service();
        }
        Client* client = new Client (root, name);
        return voxie::scripting::ScriptableObject::getPath (client);
    } catch (voxie::scripting::ScriptingException& e) {
        e.handle(this);
        return voxie::scripting::ScriptableObject::getPath(nullptr);
    }
}

QDBusObjectPath VoxieInstance::CreateIndependentClient (const QMap<QString, QVariant>& options) {
    try {
        voxie::scripting::ScriptableObject::checkOptions(options);
        Client* client = new Client (root, "");
        return voxie::scripting::ScriptableObject::getPath (client);
    } catch (voxie::scripting::ScriptingException& e) {
        e.handle(this);
        return voxie::scripting::ScriptableObject::getPath(nullptr);
    }
}


bool VoxieInstance::DestroyClient (const QDBusObjectPath& client, const QMap<QString, QVariant>& options) {
    try {
        voxie::scripting::ScriptableObject::checkOptions(options);
        Client* clientPtr = qobject_cast<Client*> (voxie::scripting::ScriptableObject::lookupWeakObject(client));
        if (!clientPtr) {
            return false;
        }
        delete clientPtr;
        return true;
    } catch (voxie::scripting::ScriptingException& e) {
        e.handle(this);
        return false;
    }
}

QDBusObjectPath VoxieInstance::CreateImage (const QDBusObjectPath& client, const voxie::scripting::IntVector2& size, const QMap<QString, QVariant>& options) {
    try {
        voxie::scripting::ScriptableObject::checkOptions(options);
        Client* clientPtr = qobject_cast<Client*> (voxie::scripting::ScriptableObject::lookupWeakObject(client));
        if (!clientPtr) {
            throw ScriptingException("de.uni_stuttgart.Voxie.ObjectNotFound", "Cannot find client object");
        }

        QSharedPointer<Image> image(new Image(size.x, size.y), [](QObject* obj) { obj->deleteLater(); });
        ScriptableObject::registerObject(image);
        clientPtr->IncRefCount(image);
        return voxie::scripting::ScriptableObject::getPath(image.data());
    } catch (voxie::scripting::ScriptingException& e) {
        e.handle(this);
        return voxie::scripting::ScriptableObject::getPath(nullptr);
    }
}

QDBusObjectPath VoxieInstance::CreateVoxelData (const QDBusObjectPath& client, const voxie::scripting::IntVector3& size, const QMap<QString, QVariant>& options) {
    try {
        voxie::scripting::ScriptableObject::checkOptions(options, "Origin", "Spacing");
        Client* clientPtr = qobject_cast<Client*> (voxie::scripting::ScriptableObject::lookupWeakObject(client));
        if (!clientPtr) {
            throw ScriptingException("de.uni_stuttgart.Voxie.ObjectNotFound", "Cannot find client object");
        }

        QVector3D origin(0, 0, 0);
        if (hasOption(options, "Origin"))
            origin = getOptionValue<QVector3D>(options, "Origin");

        QVector3D spacing(1, 1, 1);
        if (hasOption(options, "Spacing"))
            spacing = getOptionValue<QVector3D>(options, "Spacing");

        auto data = VoxelData::create(size.x, size.y, size.z);
        data->setFirstVoxelPos(origin);
        data->setSpacing(spacing);
        clientPtr->IncRefCount(data);
        return voxie::scripting::ScriptableObject::getPath(data.data());
    } catch (voxie::scripting::ScriptingException& e) {
        e.handle(this);
        return voxie::scripting::ScriptableObject::getPath(nullptr);
    }
}

QDBusObjectPath VoxieInstance::CreateDataSet (const QString& name, const QDBusObjectPath& data, const QMap<QString, QVariant>& options) {
    try {
        voxie::scripting::ScriptableObject::checkOptions(options);

        auto obj = voxie::scripting::ScriptableObject::lookupObject(data);
        if (!obj)
            throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.ObjectNotFound", "Object " + data.path() + " not found");
        auto voxelData = qSharedPointerCast<VoxelData> (obj);
        if (!voxelData)
            throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.InvalidObjectType", "Object " + data.path() + " is not a voxel data object");

        auto dataSet = new DataSet(voxelData);
        dataSet->setDisplayName(name);
        root->registerDataSet(dataSet);
        root->registerDataObject(dataSet);
        return voxie::scripting::ScriptableObject::getPath(dataSet);
    } catch (voxie::scripting::ScriptingException& e) {
        e.handle(this);
        return voxie::scripting::ScriptableObject::getPath(nullptr);
    }
}

void VoxieInstance::Quit (const QMap<QString, QVariant>& options) {
    try {
        voxie::scripting::ScriptableObject::checkOptions(options);
        root->quit ();
    } catch (voxie::scripting::ScriptingException& e) {
        e.handle(this);
    }
}

QDBusVariant VoxieInstance::ExecuteQScriptCode (const QString& code, const QMap<QString, QVariant>& options) {
    try {
        voxie::scripting::ScriptableObject::checkOptions(options);
        QScriptValue result = root->scriptEngine().evaluate(code);
        //if(result.isError()) { // isError() only returns whether the object is an instance of the Error class, not whether there was an exception
        if(root->scriptEngine().hasUncaughtException()) {
            throw ScriptingException("de.uni_stuttgart.Voxie.QScriptExecutionFailed", result.toString());
            return QDBusVariant ();
        }
        if (result.isUndefined()) {
            return QDBusVariant ("<undefined>");
        }
        QVariant resultVariant = ScriptWrapper::fromScriptValue(result);
        // Test whether result value can be marshalled
        QDBusArgument arg;
        arg.beginStructure();
        arg << QDBusVariant(resultVariant);
        arg.endStructure();
        //qDebug() << "QQ" << arg.currentSignature();
        if (arg.currentSignature() == "") {
            throw ScriptingException("de.uni_stuttgart.Voxie.QScriptMarshallingFailed", "Failed to marshal return value");
            return QDBusVariant ();
        }
        return QDBusVariant(resultVariant);
    } catch (voxie::scripting::ScriptingException& e) {
        e.handle(this);
        return QDBusVariant ();
    }
}

QDBusObjectPath VoxieInstance::OpenFileImpl (Root* root, QDBusContext* context, const QString& interface_, const QString& member, const QString& file, const QSharedPointer<voxie::io::Loader>& loader, const QMap<QString, QVariant>& options) {
    try {
        voxie::scripting::ScriptableObject::checkOptions(options);

        //qDebug() << loader.data();
        auto op = loader ? voxie::io::Load::openFile(root, loader, file) : voxie::io::Load::openFile(root, file);

        //if (context->calledFromDBus()) qDebug() << context->message().interface() << interface << context->message().member() << member;
        if (context->calledFromDBus() && (context->message().interface() == "" || context->message().interface() == interface_) && context->message().member() == member) {
            auto main = QCoreApplication::instance();
            auto conn = context->connection();
            auto msg = context->message();
            context->setDelayedReply(true);

            connect(op, &LoadOperation::loadAborted, main, [conn, msg] (QSharedPointer<voxie::scripting::ScriptingException> errorPar) {
                    conn.send(msg.createErrorReply(errorPar->name(), errorPar->message()));
                });
            connect(op, &LoadOperation::loadFinished, main, [conn, msg] (voxie::data::DataSet* dataSet) {
                    conn.send(msg.createReply(QVariant::fromValue(voxie::scripting::ScriptableObject::getPath(dataSet))));
                });

            // Reply is delayed
            return voxie::scripting::ScriptableObject::getPath(nullptr);
        } else {
            qDebug() << "Waiting for load on main thread";

            auto loop = createQSharedPointer<QEventLoop>();
            auto result = createQSharedPointer<DataSet*>();
            auto error = createQSharedPointer<QSharedPointer<ScriptingException>>();
            connect(op, &LoadOperation::loadAborted, loop.data(), [loop, error] (QSharedPointer<voxie::scripting::ScriptingException> errorPar) {
                    *error = errorPar;
                    loop->exit();
                });
            connect(op, &LoadOperation::loadFinished, loop.data(), [loop, result] (voxie::data::DataSet* dataSet) {
                    *result = dataSet;
                    loop->exit();
                });

            loop->exec();

            qDebug() << "Waiting for load on main thread finished";

            if (*error)
                throw **error;

            if (*result)
                return voxie::scripting::ScriptableObject::getPath(*result);

            throw ScriptingException("de.uni_stuttgart.Voxie.Error", "Should not be reached");
        }
    } catch (ScriptingException& e) {
        e.handle(context);
        return voxie::scripting::ScriptableObject::getPath(nullptr);
    }
}

QDBusObjectPath VoxieInstance::OpenFile (const QString& file, const QMap<QString, QVariant>& options) {
    return OpenFileImpl (root, this, "de.uni_stuttgart.Voxie.Voxie", "OpenFile", file, QSharedPointer<voxie::io::Loader>(), options);
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
