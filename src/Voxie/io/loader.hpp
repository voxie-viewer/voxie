#pragma once

#include <Voxie/voxiecore_global.hpp>

#include <Voxie/plugin/pluginmember.hpp>

#include <Voxie/scripting/dbustypes.hpp>

#include <QtCore/QObject>

#include <QtDBus/QDBusContext>

namespace voxie
{
// Forward declarations
class Root;
namespace data {
class VoxelData;
}
namespace io {
class Operation;
}

namespace io
{

class VOXIECORESHARED_EXPORT Loader :
        public voxie::plugin::PluginMember, public QDBusContext
{
    Q_OBJECT
public:
    class Filter {
        QString description_;
        QStringList patterns_;
        QList<QRegExp> compiledPatterns_;

        QString filterString_;

    public:
        Filter () {}
        VOXIECORESHARED_EXPORT Filter (const QString& description, const QStringList& patterns);
        VOXIECORESHARED_EXPORT Filter (const QVariantMap& filter);
        VOXIECORESHARED_EXPORT operator QVariantMap () const;

        const QString& description() const { return description_; }
        const QStringList& patterns() const { return patterns_; }
        const QList<QRegExp>& compiledPatterns() const { return compiledPatterns_; }

        const QString& filterString() const { return filterString_; }

        VOXIECORESHARED_EXPORT bool matches (const QString& filename) const;
    };

private:
    Filter filter_;

    QWeakPointer<Loader> self;

public:
    explicit Loader(Filter filter, QObject *parent = 0);
    virtual ~Loader();

    Q_INVOKABLE const Filter& filter() { return filter_; }

    const QWeakPointer<Loader>& getSelf() { return self; }
    void setSelf(const QSharedPointer<Loader>& ptr);

    // throws ScriptingException
    virtual QSharedPointer<voxie::data::VoxelData> load(const QSharedPointer<Operation>& op, const QString &fileName) = 0;
};

// LoaderAdaptor is in Main/io/loaderadaptor.hpp

}
}

Q_DECLARE_METATYPE(voxie::io::Loader::Filter)

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
