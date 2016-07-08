#pragma once

#include <Voxie/ivoxie.hpp>
#include <Voxie/voxiecore_global.hpp>

#include <Voxie/plugin/pluginmember.hpp>

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
class VOXIECORESHARED_EXPORT Importer :
        public voxie::plugin::PluginMember, public QDBusContext
{
    Q_OBJECT
public:
    explicit Importer(QObject *parent = 0);
    virtual ~Importer();

    virtual QString name() const
    {
        return this->metaObject()->className();
    }

    // throws ScriptingException
    Q_INVOKABLE voxie::data::DataSet* import();

protected:
    // throws ScriptingException
    virtual QSharedPointer<voxie::data::VoxelData> importImpl() = 0;

    voxie::data::DataSet* registerVoxelData(const QSharedPointer<voxie::data::VoxelData>& data);

signals:
    void dataLoaded(data::DataSet* dataSet);
};

namespace internal {
class ImporterAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "de.uni_stuttgart.Voxie.Importer")

    Importer* object;

public:
    ImporterAdaptor (Importer* object) : QDBusAbstractAdaptor (object), object (object) {}
    virtual ~ImporterAdaptor () {}

public slots:
    QDBusObjectPath Import(const QMap<QString, QVariant>& options);
};
}

}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
