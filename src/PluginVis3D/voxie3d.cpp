#include "voxie3d.hpp"

#include <PluginVis3D/isosurfacemetavisualizer.hpp>
#include <PluginVis3D/xraymetavisualizer.hpp>

#include <Voxie/opencl/clinstance.hpp>

using namespace voxie::plugin;
using namespace voxie::opencl;

Voxie3D::Voxie3D(QObject *parent) :
	QGenericPlugin(parent)
{
    Q_INIT_RESOURCE(Voxie3D);
    try {
        if (CLInstance::getDefaultInstance()->isValid())
            CLInstance::getDefaultInstance()->createProgramFromFile(":/XRay.cl", "", "voxie3d::x-ray-3d");
    } catch (CLException &ex){
        qWarning() << ex;
    }
}

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(Voxie3D, Voxie3D)
#endif // QT_VERSION < 0x050000

QObject* Voxie3D::create(const QString& name, const QString &spec)
{
	(void)name;
	(void)spec;
	return nullptr;
}

QVector<MetaVisualizer*> Voxie3D::visualizers()
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
