#include "client.hpp"

#include <Voxie/scripting/scriptingexception.hpp>

#include <limits>

#include <QtCore/QDebug>
#include <QtCore/QMetaType>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusMetaType>
#include <QtDBus/QDBusServiceWatcher>

using namespace voxie::scripting;

Client::Client(QObject* parent, const QString& uniqueConnectionName) : ScriptingContainer ("Client", parent, false, true), uniqueConnectionName_ (uniqueConnectionName) {
    if (uniqueConnectionName != "") {
        QDBusServiceWatcher* watcher = new QDBusServiceWatcher (uniqueConnectionName, QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForUnregistration, this);
        connect(watcher, &QDBusServiceWatcher::serviceUnregistered, [=]() {
                deleteLater ();
            });
        auto reply = QDBusConnection::sessionBus().interface()->isServiceRegistered(uniqueConnectionName);
        if (!reply.isValid() || !reply.value())
            deleteLater ();
    }
}
Client::~Client() {
}

bool Client::DecRefCount (QDBusObjectPath o) {
    try {
        QSharedPointer<ScriptingContainer> obj = ScriptingContainer::lookupObject(o);
        if (!obj)
            return false;

        QMap<QObject*, Reference>::iterator it = references.find (obj.data());
        if (it == references.end ())
            throw ScriptingException("de.uni_stuttgart.Voxie.ReferenceNotFound", "Client " + getPath().path() + " does not hold a reference to object " + o.path());

        if (!--it->refCount) {
            references.remove(obj.data());
        }
        return true;
    } catch (ScriptingException& e) {
        e.handle(this);
        return false;
    }
}

void Client::IncRefCount (QDBusObjectPath o) {
    try {
        QSharedPointer<ScriptingContainer> obj = ScriptingContainer::lookupObject(o);
        if (!obj)
            throw ScriptingException("de.uni_stuttgart.Voxie.ObjectNotFound", "Object " + o.path() + " not found");
        IncRefCount (obj);
    } catch (ScriptingException& e) {
        e.handle(this);
    }
}

void Client::IncRefCount (const QSharedPointer<ScriptingContainer>& obj) {
    QMap<QObject*, Reference>::iterator it = references.find (obj.data());
    if (it == references.end ()) {
        Reference ref;
        ref.refCount = 1;
        ref.target = obj;
        references.insert (obj.data(), ref);
    } else {
        if (it->refCount == std::numeric_limits<quint64>::max())
            throw ScriptingException("de.uni_stuttgart.Voxie.Overflow", "Reference counter overflow");
        it->refCount++;
    }
}

QMap<QDBusObjectPath, quint64> Client::GetReferencedObjects () {
    QMap<QDBusObjectPath, quint64> map;

    for (const auto& ref : references) {
        map.insert (ref.target->getPath (), ref.refCount);
    }

    return map;
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
