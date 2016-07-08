#pragma once

#include <Voxie/voxiecore_global.hpp>

#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QStringList>

#include <QtDBus/QDBusObjectPath>
#include <QtDBus/QDBusArgument>
#include <QtDBus/QDBusMetaType>

#include <QtWidgets/QWidget>

namespace voxie
{
namespace scripting
{

class VOXIECORESHARED_EXPORT ScriptingContainerBase
{
private:
	QMap<QObject*, QMetaObject::Connection> connections;
    QString path;
    bool exportScriptable_;

public:
	ScriptingContainerBase();
	ScriptingContainerBase(QObject* obj, const QString& type, bool singleton, bool exportScriptable);
	virtual ~ScriptingContainerBase();

    QDBusObjectPath getPath () const;

    bool exportScriptable() const { return exportScriptable_; }

    static QDBusObjectPath getPath (ScriptingContainerBase* obj) {
        if (!obj)
            return QDBusObjectPath ("/");
        return obj->getPath ();
    }

    static ScriptingContainerBase* lookupWeakObject(const QDBusObjectPath& path);
    static QObject* lookupWeakQObject(const QDBusObjectPath& path);

private:
    static void addToQSet(QSet<QString>& set) {
        Q_UNUSED(set);
    }
    template <typename T, typename... U>
    static void addToQSet(QSet<QString>& set, T head, U... tail) {
        set.insert(head);
        addToQSet(set, tail...);
    }

    static Q_NORETURN void throwMissingOption(const QString& name);
    static Q_NORETURN void throwInvalidOption(const QString& name, const QString& expected, const QString& actual);

public:
    static void checkOptions (const QMap<QString, QVariant>& options, const QSet<QString>& allowed);
    template <typename... T>
    static void checkOptions (const QMap<QString, QVariant>& options, T... allowed) {
        QSet<QString> set;
        addToQSet(set, allowed...);
        checkOptions(options, set);
    }

    static bool hasOption(const QMap<QString, QVariant>& options, const QString& name) {
        return options.find(name) != options.end();
    }
    template <typename T>
    static T getOptionValue(const QMap<QString, QVariant>& options, const QString& name) {
        auto value = options.find(name);
        if (value == options.end())
            throwMissingOption(name);

        auto metaTypeIdT = qMetaTypeId<T>();

        if (value->userType() == metaTypeIdT)
            return value->value<T>();

        if (value->userType() == qMetaTypeId<QDBusArgument>()) {
            auto arg = value->value<QDBusArgument>();
            auto sig = arg.currentSignature();
            auto expected = QDBusMetaType::typeToSignature(metaTypeIdT);
            if (sig != expected)
                throwInvalidOption(name, expected, sig);
            return qdbus_cast<T>(arg);
        }

        throwInvalidOption(name, QMetaType::typeName(metaTypeIdT), value->typeName());
    }

    virtual QObject* scriptingContainerGetQObject() = 0;
};

class VOXIECORESHARED_EXPORT ScriptingContainer : public QObject, public ScriptingContainerBase
{
	Q_OBJECT
private:
	QMap<QObject*, QMetaObject::Connection> connections;
public:
	explicit ScriptingContainer(const QString& type, QObject *parent = 0, bool singleton = false, bool exportScriptable = false);
	explicit ScriptingContainer(QObject *parent = 0);
	~ScriptingContainer();

    static void registerObject(const QSharedPointer<ScriptingContainer>& obj);
    static QSharedPointer<ScriptingContainer> lookupObject(const QDBusObjectPath& path);

    virtual QObject* scriptingContainerGetQObject();
};

class VOXIECORESHARED_EXPORT WidgetScriptingContainer : public QWidget, public ScriptingContainerBase
{
	Q_OBJECT
private:
	QMap<QObject*, QMetaObject::Connection> connections;
public:
	explicit WidgetScriptingContainer(const QString& type, QWidget *parent = 0, bool singleton = false);
	explicit WidgetScriptingContainer(QWidget *parent = 0);
	~WidgetScriptingContainer();

    virtual QObject* scriptingContainerGetQObject();
};

}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
