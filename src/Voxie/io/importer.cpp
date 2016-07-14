#include "importer.hpp"

#include <Voxie/data/dataset.hpp>
#include <Voxie/data/voxeldata.hpp>

#include <Voxie/scripting/scriptingexception.hpp>

using namespace voxie::io;
using namespace voxie::io::internal;

Importer::Importer(QObject *parent) :
    voxie::plugin::PluginMember("Importer", parent)
{
    new ImporterAdaptor (this);
}

Importer::~Importer()
{

}

voxie::data::DataSet* Importer::import() {
    return registerVoxelData(importImpl());
}

voxie::data::DataSet* Importer::registerVoxelData(const QSharedPointer<voxie::data::VoxelData>& data) {
    voxie::data::DataSet* dataSet = new voxie::data::DataSet(data);
    dataSet->setObjectName(data->objectName());
    emit dataLoaded(dataSet);
    return dataSet;
}

QDBusObjectPath ImporterAdaptor::Import(const QMap<QString, QVariant>& options) {
    try {
        voxie::scripting::ScriptableObject::checkOptions(options);
        return voxie::scripting::ScriptableObject::getPath(object->import());
    } catch (voxie::scripting::ScriptingException& e) {
        e.handle(object);
        return voxie::scripting::ScriptableObject::getPath(nullptr);
    }
}


// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
