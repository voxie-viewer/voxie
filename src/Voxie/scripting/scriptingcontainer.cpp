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

// TODO: move fields into a new class?
static QMutex mutex;
static QMap<QString, uint64_t> currentId;
static QMap<QDBusObjectPath, ScriptableObject*> weakReferences;
static QMap<QDBusObjectPath, QWeakPointer<ScriptableObject>> references;

ScriptableObject::ScriptableObject(const QString& type, QObject *parent, bool singleton, bool exportScriptable) :
    QObject(parent),
    exportScriptable_(exportScriptable)
{
    if (QThread::currentThread() != QCoreApplication::instance()->thread())
        qCritical() << "Warning: Trying to create object" << this << "on thread " << QThread::currentThread();

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
		this,
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
    QObject::connect(this, &QObject::destroyed, [=]() {
            QMutexLocker lock (&mutex);
            weakReferences.remove (QDBusObjectPath(path));
        });
    weakReferences.insert (QDBusObjectPath(path), this);
  }
}

ScriptableObject::~ScriptableObject()
{
    if (QThread::currentThread() != QCoreApplication::instance()->thread())
        qCritical() << "Warning: Trying to destroy object on thread " << QThread::currentThread();
}

void ScriptableObject::checkOptions (const QMap<QString, QVariant>& options, const QSet<QString>& allowed) {
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

Q_NORETURN void ScriptableObject::throwMissingOption(const QString& name) {
    throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.MissingOptionValue", "No value given for '" + name + "' option");
}
Q_NORETURN void ScriptableObject::throwInvalidOption(const QString& name, const QString& expected, const QString& actual) {
    throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.InvalidOptionValue", "Invalid type for '" + name + "' option: got " + actual + ", expected " + expected);
}

QDBusObjectPath ScriptableObject::getPath() const {
    return QDBusObjectPath (path);
}

ScriptableObject* ScriptableObject::lookupWeakObject(const QDBusObjectPath& path) {
    QMutexLocker lock (&mutex);
    auto it = weakReferences.find (path);
    if (it == weakReferences.end ())
        return nullptr;
    return *it;
}

void ScriptableObject::registerObject(const QSharedPointer<ScriptableObject>& obj) {
    QMutexLocker lock (&mutex);
    QDBusObjectPath path = obj->getPath();
    connect(obj.data(), &QObject::destroyed, [=]() {
            QMutexLocker lock (&mutex);
            references.remove (path);
        });
    references.insert (path, obj.toWeakRef());
}
QSharedPointer<ScriptableObject> ScriptableObject::lookupObject(const QDBusObjectPath& path) {
    QMutexLocker lock (&mutex);
    auto it = references.find (path);
    if (it == references.end ())
        return QSharedPointer<ScriptableObject>();
    return it->toStrongRef();
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
