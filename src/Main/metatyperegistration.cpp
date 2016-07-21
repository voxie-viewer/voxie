#include "metatyperegistration.hpp"

#include <Voxie/data/dataset.hpp>
#include <Voxie/data/slice.hpp>
#include <Voxie/data/voxeldata.hpp>

#include <Voxie/visualization/visualizer.hpp>

#include <Voxie/scripting/scriptingexception.hpp>

#include <QtCore/QObject>

void MetatypeRegistration::registerMetatypes()
{
    // library tpyes
    qRegisterMetaType<QVector3D>();

    qRegisterMetaType<voxie::data::VoxelData*>();
    qRegisterMetaType<voxie::data::DataSet*>();
    qRegisterMetaType<voxie::data::Slice*>();
    qRegisterMetaType<voxie::visualization::Visualizer*>();
}


// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
