#pragma once

#include <Voxie/voxiecore_global.hpp>

#include <Voxie/plugin/pluginmember.hpp>

#include <Voxie/scripting/scriptingcontainer.hpp>

#include <QtCore/QObject>

namespace voxie
{
namespace filter
{
    class Filter3D;
}
namespace plugin
{

/**
 * Factory class for a Filter. Every Filter needs to have a MetaFilter of its own.
 * A Plugin featuring Filters needs to provide a list of metafilters to make
 * the filter known to Voxie.
 */
class VOXIECORESHARED_EXPORT MetaFilter3D :
        public voxie::plugin::PluginMember
{
    Q_OBJECT
public:
    explicit MetaFilter3D(QObject *parent = 0);
    virtual ~MetaFilter3D();

    /** factory method for the filter */
    virtual filter::Filter3D *createFilter() const = 0;
};

}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
