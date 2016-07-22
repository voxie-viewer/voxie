#include "filterplugin.hpp"

#include <PluginFilter/boxblur3d.hpp>
#include <PluginFilter/gaussfilter2d.hpp>
#include <PluginFilter/normalizefilter2d.hpp>
#include <PluginFilter/valuelimiter3d.hpp>

FilterPlugin::FilterPlugin(QObject* parent) :
    QGenericPlugin(parent)
{

}

QObject *FilterPlugin::create(const QString & key, const QString & specification)
{
    (void)key;
    (void)specification;
    return nullptr;
}

QVector<voxie::plugin::MetaFilter2D*> FilterPlugin::filters2D()
{
    QVector<voxie::plugin::MetaFilter2D*> list;
    list.append(new GaussMetaFilter2D());
    list.append(new NormalizeMetaFilter2D());
    return list;
}

QVector<voxie::plugin::MetaFilter3D*> FilterPlugin::filters3D()
{
    QVector<voxie::plugin::MetaFilter3D*> list;
    list.append(new MetaValueLimiter3D());
    list.append(new BoxBlur3DMeta());
    return list;
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
