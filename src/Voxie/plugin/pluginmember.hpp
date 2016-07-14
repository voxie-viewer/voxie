#pragma once

#include <Voxie/scripting/scriptingcontainer.hpp>

#include <QtDBus/QDBusAbstractAdaptor>
#include <QtDBus/QDBusObjectPath>

namespace voxie
{
namespace plugin
{

class VoxiePlugin;

class VOXIECORESHARED_EXPORT PluginMember : public voxie::scripting::ScriptableObject {
    Q_OBJECT

    friend class VoxiePlugin;

    // These values will be set by VoxiePlugin::addObjects() in voxieplugin.cpp
    VoxiePlugin* plugin_;
    QString type_;
    QString name_;

public:
    PluginMember (const QString& typeShort, QObject* parent = nullptr);
    virtual ~PluginMember ();

    Q_PROPERTY (VoxiePlugin* Plugin READ plugin)
    VoxiePlugin* plugin() const { return plugin_; }

    Q_PROPERTY (QString Type READ type)
    const QString& type() const { return type_; }

    Q_PROPERTY (QString Name READ name)
    const QString& name() const { return name_; }
};

namespace internal {
class PluginMemberAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "de.uni_stuttgart.Voxie.PluginMember")

    PluginMember* object;

public:
    PluginMemberAdaptor (PluginMember* object) : QDBusAbstractAdaptor (object), object (object) {}
    virtual ~PluginMemberAdaptor () {}

    Q_PROPERTY (QDBusObjectPath Plugin READ plugin)
    QDBusObjectPath plugin();

    Q_PROPERTY (QString Type READ type)
    const QString& type() { return object->type(); }

    Q_PROPERTY (QString Name READ name)
    const QString& name() { return object->name(); }
};
}


}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
