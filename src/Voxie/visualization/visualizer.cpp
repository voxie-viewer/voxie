#include "visualizer.hpp"

#include <Voxie/plugin/metavisualizer.hpp>

#include <QtGui/QIcon>

using namespace voxie::visualization;
using namespace voxie::visualization::internal;

Visualizer::Visualizer(QObject *parent) :
    voxie::data::DataObject("Visualizer", parent),
    sections()
{

}

Visualizer::~Visualizer()
{

}

QIcon Visualizer::icon() const {
    switch (type()->type()) {
    case voxie::plugin::vt2D: return QIcon(":/icons/layers.png");
    case voxie::plugin::vt3D: return QIcon(":/icons/spectacle-3d.png");
    case voxie::plugin::vtAnalytic: return QIcon(":/icons/flask.png");
    case voxie::plugin::vtMiscellaneous: return QIcon(":/icons/equalizer.png");
    default: return QIcon();
    }
}


QVector<QWidget*> &Visualizer::dynamicSections()
{
    return this->sections;
}

VolumeDataVisualizer::VolumeDataVisualizer(QObject *parent)
    : Visualizer(parent)
{
    new VolumeDataVisualizerAdaptor (this);
}

VolumeDataVisualizer::~VolumeDataVisualizer()
{
}

SliceDataVisualizer::SliceDataVisualizer(QObject *parent)
    : VolumeDataVisualizer(parent)
{
    new SliceDataVisualizerAdaptor (this);
}

SliceDataVisualizer::~SliceDataVisualizer()
{
}

QDBusObjectPath VolumeDataVisualizerAdaptor::dataSet()
{
  return voxie::scripting::ScriptableObject::getPath(object->dataSet());
}

QDBusObjectPath SliceDataVisualizerAdaptor::slice()
{
  return voxie::scripting::ScriptableObject::getPath(object->slice());
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
