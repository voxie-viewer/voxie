#pragma once

#include <Voxie/scripting/dbustypes.hpp>
#include <Voxie/scripting/scriptingcontainer.hpp>

#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>

#include <QtDBus/QDBusContext>
#include <QtDBus/QDBusObjectPath>

namespace voxie
{
namespace scripting
{

class VOXIECORESHARED_EXPORT Client : public ScriptingContainer, public QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "de.uni_stuttgart.Voxie.Client")

    struct Reference {
        quint64 refCount;
        QSharedPointer<ScriptingContainer> target;
    };

	QMap<QObject*, Reference> references;

    QString uniqueConnectionName_;

public:
	Client(QObject* parent, const QString& uniqueConnectionName);
	virtual ~Client();

    Q_SCRIPTABLE bool DecRefCount (QDBusObjectPath o);
    void IncRefCount (const QSharedPointer<ScriptingContainer>& obj);
    Q_SCRIPTABLE void IncRefCount (QDBusObjectPath o);

    // Should only be used for debugging purposes
    Q_PROPERTY (QString UniqueConnectionName READ uniqueConnectionName)
    const QString& uniqueConnectionName() const { return uniqueConnectionName_; }

    // Should only be used for debugging purposes
    //Q_SCRIPTABLE QMap<QDBusObjectPath, quint64> GetReferencedObjects ();
    // To make moc happy / https://bugreports.qt.io/browse/QTBUG-11485
    Q_SCRIPTABLE QMap_QDBusObjectPath_quint64 GetReferencedObjects ();
};

}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
