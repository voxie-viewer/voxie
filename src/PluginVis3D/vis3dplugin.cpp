#include "vis3dplugin.hpp"

#include <PluginVis3D/isosurfacemetavisualizer.hpp>
#include <PluginVis3D/xraymetavisualizer.hpp>

#include <Voxie/opencl/clinstance.hpp>

using namespace voxie::plugin;
using namespace voxie::opencl;

Vis3DPlugin::Vis3DPlugin(QObject *parent) :
	QGenericPlugin(parent)
{
    Q_INIT_RESOURCE(PluginVis3D);
    try {
        if (CLInstance::getDefaultInstance()->isValid())
            CLInstance::getDefaultInstance()->createProgramFromFile(":/XRay.cl", "", "voxie3d::x-ray-3d");
    } catch (CLException &ex){
        qWarning() << ex;
    }
}

QObject* Vis3DPlugin::create(const QString& name, const QString &spec)
{
	(void)name;
	(void)spec;
	return nullptr;
}

QVector<MetaVisualizer*> Vis3DPlugin::visualizers()
{
	QVector<MetaVisualizer*> visualizers;
	visualizers.append(IsosurfaceMetaVisualizer::instance());
    visualizers.append(XRayMetaVisualizer::instance());
	return visualizers;
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
