#include "pluginmember.hpp"

#include <Voxie/plugin/voxieplugin.hpp>

using namespace voxie::plugin;
using namespace voxie::plugin::internal;

PluginMember::PluginMember (const QString& typeShort, QObject* parent) : ScriptableObject("PluginMember/" + typeShort, parent), plugin_ (nullptr) {
    new PluginMemberAdaptor (this);
}
PluginMember::~PluginMember () {
}

QDBusObjectPath PluginMemberAdaptor::plugin() {
    return voxie::scripting::ScriptableObject::getPath(object->plugin());
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
