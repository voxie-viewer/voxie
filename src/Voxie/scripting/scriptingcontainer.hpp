#pragma once

#include <Voxie/voxiecore_global.hpp>

#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QStringList>
#include <QtCore/QSharedPointer>

#include <QtDBus/QDBusObjectPath>
#include <QtDBus/QDBusArgument>
#include <QtDBus/QDBusMetaType>

template <typename T, typename... U>
static inline QSharedPointer<T> createQSharedPointer(U&&... par) {
    // QSharedPointer<T>::create() is broken: If the constructor throws an
    // exception, it will call the objects destructor.
    // https://bugreports.qt.io/browse/QTBUG-49824
    return QSharedPointer<T>(new T(std::forward<U>(par)...));
}

namespace voxie
{
namespace scripting
{

class VOXIECORESHARED_EXPORT ScriptableObject : public QObject {
    Q_OBJECT

private:
    QString path;
    bool exportScriptable_;

public:
	explicit ScriptableObject(const QString& type, QObject *parent = 0, bool singleton = false, bool exportScriptable = false);
	virtual ~ScriptableObject();

    QDBusObjectPath getPath () const;

    bool exportScriptable() const { return exportScriptable_; }

    static QDBusObjectPath getPath (ScriptableObject* obj) {
        if (!obj)
            return QDBusObjectPath ("/");
        return obj->getPath ();
    }

    static ScriptableObject* lookupWeakObject(const QDBusObjectPath& path);

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

    static void registerObject(const QSharedPointer<ScriptableObject>& obj);
    static QSharedPointer<ScriptableObject> lookupObject(const QDBusObjectPath& path);
};

} }

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
