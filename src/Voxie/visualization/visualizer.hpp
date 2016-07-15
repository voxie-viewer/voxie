#pragma once

#include <Voxie/voxiecore_global.hpp>

#include <Voxie/data/dataobject.hpp>
#include <Voxie/data/slice.hpp>

#include <QtCore/QVector>

#include <QtDBus/QDBusAbstractAdaptor>

#include <QtWidgets/QWidget>

namespace voxie
{
// Forward declarations
namespace data {
class DataSet;
}
namespace plugin {
class MetaVisualizer;
}

namespace visualization
{

/**
 * @brief A visualizer that can show any kind of voxel data.
 */
class VOXIECORESHARED_EXPORT Visualizer : public voxie::data::DataObject {
    Q_OBJECT
private:
    QVector<QWidget*> sections;
public:
    explicit Visualizer(QObject *parent = 0);
    virtual ~Visualizer();

    virtual voxie::plugin::MetaVisualizer* type() const = 0;

    QIcon icon() const override;

    /**
     * @brief Returns a set of dynamic sections that will be shown/hidden depending on the visualizer state.
     * @return Vector with the side panel secions.
     */
    QVector<QWidget*> &dynamicSections();

    virtual QWidget* mainView() = 0;
};

class VOXIECORESHARED_EXPORT VolumeDataVisualizer : public Visualizer
{
    Q_OBJECT
public:
    explicit VolumeDataVisualizer(QObject *parent = 0);
    virtual ~VolumeDataVisualizer();
    Q_PROPERTY (voxie::data::DataSet* dataSet READ dataSet)
    virtual voxie::data::DataSet* dataSet() = 0;
};

class VOXIECORESHARED_EXPORT SliceDataVisualizer : public VolumeDataVisualizer
{
    Q_OBJECT
public:
    explicit SliceDataVisualizer(QObject *parent = 0);
    virtual ~SliceDataVisualizer();
    Q_PROPERTY (voxie::data::Slice* slice READ slice)
    virtual voxie::data::Slice* slice() = 0;
    virtual voxie::data::DataSet* dataSet() final
    {
        return slice()->getDataset();
    }
};

namespace internal {
class VolumeDataVisualizerAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "de.uni_stuttgart.Voxie.VolumeDataVisualizer")

    VolumeDataVisualizer* object;

public:
    VolumeDataVisualizerAdaptor (VolumeDataVisualizer* object) : QDBusAbstractAdaptor (object), object (object) {}
    virtual ~VolumeDataVisualizerAdaptor () {}

    Q_PROPERTY (QDBusObjectPath/* voxie::data::DataSet* */ DataSet READ dataSet)
    QDBusObjectPath/* voxie::data::DataSet* */ dataSet();
};

class SliceDataVisualizerAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "de.uni_stuttgart.Voxie.SliceDataVisualizer")

    SliceDataVisualizer* object;

public:
    SliceDataVisualizerAdaptor (SliceDataVisualizer* object) : QDBusAbstractAdaptor (object), object (object) {}
    virtual ~SliceDataVisualizerAdaptor () {}

    Q_PROPERTY (QDBusObjectPath/* voxie::data::Slice* */ Slice READ slice)
    QDBusObjectPath/* voxie::data::Slice* */ slice();
};
}

}
}

Q_DECLARE_METATYPE(voxie::visualization::Visualizer*)

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
