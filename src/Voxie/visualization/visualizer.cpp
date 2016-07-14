#include "visualizer.hpp"

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
