#include "scriptingcontainer.hpp"

#include <Voxie/scripting/scriptingexception.hpp>

#include <cstdint>

#include <QtCore/QDebug>
#include <QtCore/QMetaMethod>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaProperty>
#include <QtCore/QMutexLocker>
#include <QtCore/QRegExp>
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>

using namespace voxie::scripting;

static QRegExp invalidChars("[^A-Za-z0-9_]+", Qt::CaseSensitive);

ScriptingContainerBase::ScriptingContainerBase() :
	connections()
{
}

// TODO: move fields into a new class?
static QMutex mutex;
static QMap<QString, uint64_t> currentId;
static QMap<QDBusObjectPath, ScriptingContainerBase*> weakReferences;
static QMap<QDBusObjectPath, QWeakPointer<ScriptingContainer>> references;

ScriptingContainerBase::ScriptingContainerBase(QObject* obj, const QString& type, bool singleton, bool exportScriptable) : exportScriptable_(exportScriptable)
{
    if (QThread::currentThread() != QCoreApplication::instance()->thread())
        qCritical() << "Warning: Trying to create object" << obj << "on thread " << QThread::currentThread();

  if (singleton) {
    path = QString ("/de/uni_stuttgart/Voxie%1%2").arg (type == "" ? "" : "/").arg (type);
  } else {
    uint64_t id;
    {
      QMutexLocker lock (&mutex);
      id = ++currentId[type];
    }
    path = QString ("/de/uni_stuttgart/Voxie/%1/%2").arg (type).arg (id);
  }
  QDBusConnection::sessionBus().registerObject(
		path,
		//scriptingContainerGetQObject(),
		obj,
        exportScriptable ?
        (
         QDBusConnection::ExportAdaptors
         | QDBusConnection::ExportScriptableInvokables
         | QDBusConnection::ExportScriptableSlots
         | QDBusConnection::ExportScriptableProperties
         | QDBusConnection::ExportScriptableSignals
         ) :
        QDBusConnection::ExportAdaptors
		);

  {
    QMutexLocker lock (&mutex);
    QObject::connect(obj, &QObject::destroyed, [=]() {
            QMutexLocker lock (&mutex);
            weakReferences.remove (QDBusObjectPath(path));
        });
    weakReferences.insert (QDBusObjectPath(path), this);
  }
}

ScriptingContainerBase::~ScriptingContainerBase()
{
    if (QThread::currentThread() != QCoreApplication::instance()->thread())
        qCritical() << "Warning: Trying to destroy object on thread " << QThread::currentThread();
}

void ScriptingContainerBase::checkOptions (const QMap<QString, QVariant>& options, const QSet<QString>& allowed) {
    QSet<QString> optional;
    if (options.contains("Optional")) {
        auto value = options["Optional"];
        if (value.type() != QVariant::StringList)
            throw ScriptingException("de.uni_stuttgart.Voxie.InvalidOptionValue", "Invalid type for 'Optional' option");
        optional = QSet<QString>::fromList(value.toStringList());
    }
    for (auto key : options.keys()) {
        if (!allowed.contains(key) && !optional.contains(key) && key != "Optional")
            throw ScriptingException("de.uni_stuttgart.Voxie.InvalidOption", "Unknown option '" + key + "'");

    }
}

QDBusObjectPath ScriptingContainerBase::getPath() const {
    return QDBusObjectPath (path);
}

ScriptingContainerBase* ScriptingContainerBase::lookupWeakObject(const QDBusObjectPath& path) {
    QMutexLocker lock (&mutex);
    auto it = weakReferences.find (path);
    if (it == weakReferences.end ())
        return nullptr;
    return *it;
}

void ScriptingContainer::registerObject(const QSharedPointer<ScriptingContainer>& obj) {
    QMutexLocker lock (&mutex);
    QDBusObjectPath path = obj->getPath();
    connect(obj.data(), &QObject::destroyed, [=]() {
            QMutexLocker lock (&mutex);
            references.remove (path);
        });
    references.insert (path, obj.toWeakRef());
}
QSharedPointer<ScriptingContainer> ScriptingContainer::lookupObject(const QDBusObjectPath& path) {
    QMutexLocker lock (&mutex);
    auto it = references.find (path);
    if (it == references.end ())
        return QSharedPointer<ScriptingContainer>();
    return it->toStrongRef();
}

ScriptingContainer::ScriptingContainer(QObject *parent) :
	QObject(parent)
{
}

ScriptingContainer::ScriptingContainer(const QString& type, QObject *parent, bool singleton, bool exportScriptable) :
	QObject(parent), ScriptingContainerBase(this, type, singleton, exportScriptable)
{
}

ScriptingContainer::~ScriptingContainer()
{
}

QObject* ScriptingContainer::scriptingContainerGetQObject()
{
  return this;
}

WidgetScriptingContainer::WidgetScriptingContainer(QWidget *parent) :
	QWidget(parent)
{
}

WidgetScriptingContainer::WidgetScriptingContainer(const QString& type, QWidget *parent, bool singleton) :
	QWidget(parent), ScriptingContainerBase(this, type, singleton, false)
{
}

WidgetScriptingContainer::~WidgetScriptingContainer()
{
}

QObject* WidgetScriptingContainer::scriptingContainerGetQObject()
{
  return this;
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
