#include "metavisualizer.hpp"

#include <Voxie/ivoxie.hpp>

#include <Voxie/scripting/scriptingexception.hpp>

#include <QtDBus/QDBusConnection>

#include <QtGui/QIcon>

using namespace voxie;
using namespace voxie::plugin;
using namespace voxie::plugin::internal;
using namespace voxie::data;
using namespace voxie::visualization;

MetaVisualizer::MetaVisualizer(QObject *parent) :
    voxie::plugin::PluginMember("VisualizerFactory", parent)
{
    new MetaVisualizerAdaptor (this);
}

MetaVisualizer::~MetaVisualizer()
{

}

visualization::Visualizer *MetaVisualizer::create(const QVector<data::DataSet*> &dataSets, const QVector<data::Slice*> &slices)
{
    if(!this->requiredDataSetCount().contains(dataSets.size()))
        throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.IncorrectNumberOfDataSets", "Incorrect number of DataSets");
    if(!this->requiredSliceCount().contains(slices.size()))
        throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.IncorrectNumberOfSlice", "Incorrect number of Slices");

    Visualizer *visualizer = this->createVisualizer(dataSets, slices);
    if (!visualizer)
        throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.Error", "Error creating visualizer");

    if (visualizer->displayName() == "")
        visualizer->setDisplayName(visualizer->metaObject()->className());

    for(DataSet *dataSet : dataSets)
        connect(dataSet, &QObject::destroyed, visualizer, &QObject::deleteLater);
    for(Slice *slice : slices)
        connect(slice, &QObject::destroyed, visualizer, &QObject::deleteLater);

    voxieRoot().registerVisualizer(visualizer);

    for (auto parent : dataSets)
        parent->addChildObject(visualizer);
    for (auto parent : slices)
        parent->addChildObject(visualizer);
	voxie::voxieRoot().registerDataObject(visualizer);

    return visualizer;
}

QDBusObjectPath MetaVisualizerAdaptor::Create(const QVector<QDBusObjectPath>& dataSets, const QVector<QDBusObjectPath>& slices, const QMap<QString, QVariant>& options) {
    try {
        voxie::scripting::ScriptableObject::checkOptions(options);
        QVector<data::DataSet*> dataSetObjects;
        QVector<data::Slice*> sliceObjects;

        for (const QDBusObjectPath& path : dataSets) {
            data::DataSet* obj = qobject_cast<data::DataSet*> (voxie::scripting::ScriptableObject::lookupWeakObject(path));
            if (!obj)
                throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.ObjectNotFound", "Cannot find DataSet object");
            dataSetObjects.push_back (obj);
        }
        for (const QDBusObjectPath& path : slices) {
            data::Slice* obj = qobject_cast<data::Slice*> (voxie::scripting::ScriptableObject::lookupWeakObject(path));
            if (!obj)
                throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.ObjectNotFound", "Cannot find Slice object");
            sliceObjects.push_back (obj);
        }

        visualization::Visualizer* visualizer = object->create(dataSetObjects, sliceObjects);
        return voxie::scripting::ScriptableObject::getPath(visualizer);
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
