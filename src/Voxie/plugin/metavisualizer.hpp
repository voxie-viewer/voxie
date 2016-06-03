#pragma once

#include <Voxie/voxiecore_global.hpp>

#include <Voxie/data/range.hpp>
#include <Voxie/data/slice.hpp>
#include <Voxie/data/voxeldata.hpp>

#include <Voxie/plugin/pluginmember.hpp>

#include <Voxie/scripting/dbustypes.hpp>

#include <Voxie/visualization/visualizer.hpp>

#include <QtCore/QObject>
#include <QtCore/QVariant>

namespace voxie
{
namespace plugin
{

enum VisualizerType
{
    vtDefault = 0,
    vt2D = 1,
    vt3D = 2,
    vtAnalytic = 3,
    vtMiscellaneous = 0,
};

class VOXIECORESHARED_EXPORT MetaVisualizer :
        public voxie::plugin::PluginMember, public QDBusContext
{
    Q_OBJECT
public:
    explicit MetaVisualizer(QObject *parent = nullptr);
    ~MetaVisualizer();

    virtual QString name() const
    {
        return this->metaObject()->className();
    }

    virtual VisualizerType type() const = 0;

    virtual data::Range requiredSliceCount() const = 0;
    virtual data::Range requiredDataSetCount() const = 0;

    // throws ScriptingException
    visualization::Visualizer *create(const QVector<data::DataSet*> &dataSets, const QVector<data::Slice*> &slices);

protected:
    // throws ScriptingException
    virtual visualization::Visualizer *createVisualizer(const QVector<data::DataSet*> &dataSets, const QVector<data::Slice*> &slices) = 0;
};


namespace internal {
class MetaVisualizerAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "de.uni_stuttgart.Voxie.VisualizerFactory")

    MetaVisualizer* object;

public:
    MetaVisualizerAdaptor (MetaVisualizer* object) : QDBusAbstractAdaptor (object), object (object) {}
    virtual ~MetaVisualizerAdaptor () {}

public slots:
    QDBusObjectPath Create(const QVector<QDBusObjectPath>& dataSets, const QVector<QDBusObjectPath>& slices, const QMap<QString, QVariant>& options);
};
}

}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
