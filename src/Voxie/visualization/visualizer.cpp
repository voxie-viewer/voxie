#include "visualizer.hpp"

using namespace voxie::visualization;
using namespace voxie::visualization::internal;

Visualizer::Visualizer(QWidget *parent) :
    WidgetScriptingContainer("Visualizer", parent),
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

VolumeDataVisualizer::VolumeDataVisualizer(QWidget *parent)
    : Visualizer(parent)
{
    new VolumeDataVisualizerAdaptor (this);
}

VolumeDataVisualizer::~VolumeDataVisualizer()
{
}

SliceDataVisualizer::SliceDataVisualizer(QWidget *parent)
    : VolumeDataVisualizer(parent)
{
    new SliceDataVisualizerAdaptor (this);
}

SliceDataVisualizer::~SliceDataVisualizer()
{
}

QDBusObjectPath VolumeDataVisualizerAdaptor::dataSet()
{
  return voxie::scripting::ScriptingContainerBase::getPath(object->dataSet());
}

QDBusObjectPath SliceDataVisualizerAdaptor::slice()
{
  return voxie::scripting::ScriptingContainerBase::getPath(object->slice());
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
