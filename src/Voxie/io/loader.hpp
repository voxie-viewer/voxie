#pragma once

#include <Voxie/voxiecore_global.hpp>

#include <Voxie/plugin/pluginmember.hpp>

#include <Voxie/scripting/dbustypes.hpp>

#include <QtCore/QObject>

#include <QtDBus/QDBusContext>

namespace voxie
{
// Forward declarations
namespace data{
class DataSet;
class VoxelData;
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

public:
    explicit Loader(Filter filter, QObject *parent = 0);
    virtual ~Loader();

    Q_INVOKABLE const Filter& filter() { return filter_; }

    // throws ScriptingException
    voxie::data::DataSet* load(const QString &fileName);

protected:
    // throws ScriptingException
    virtual voxie::data::VoxelData* loadImpl(const QString &fileName) = 0;

    voxie::data::DataSet* registerVoxelData(voxie::data::VoxelData* data, const QString &fileName);

signals:
    void dataLoaded(data::DataSet* dataSet);
};

namespace internal {
class LoaderAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "de.uni_stuttgart.Voxie.Loader")

    Loader* object;

public:
    LoaderAdaptor (Loader* object) : QDBusAbstractAdaptor (object), object (object) {}
    virtual ~LoaderAdaptor () {}

    Q_PROPERTY (QVariantMap Filter READ filter)
    QVariantMap filter() { return object->filter(); }

public slots:
    QDBusObjectPath Load(const QString &fileName, const QMap<QString, QVariant>& options);
};
}

}
}

Q_DECLARE_METATYPE(voxie::io::Loader::Filter)

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
