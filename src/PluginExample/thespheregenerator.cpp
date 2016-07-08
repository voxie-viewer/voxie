#include "thespheregenerator.hpp"

#include <Voxie/data/dataset.hpp>
#include <Voxie/data/voxeldata.hpp>

#include <Voxie/scripting/scriptingexception.hpp>

#include <algorithm>

#include <math.h>

using namespace voxie::data;
using namespace voxie::io;

using namespace ::internal;

TheSphereGenerator::TheSphereGenerator(QObject *parent) :
	Importer(parent)
{
    new TheSphereGeneratorAdaptor (this);
}

TheSphereGenerator::~TheSphereGenerator()
{

}

QSharedPointer<voxie::data::VoxelData> TheSphereGenerator::importImpl()
{
	return this->genSphereImpl(129);
}

QSharedPointer<voxie::data::VoxelData> TheSphereGenerator::genSphereImpl(int size)
{
	QVector3D origin(-(size - 1) / 2.0f, -(size - 1) / 2.0f, -(size - 1) / 2.0f);

	auto data = VoxelData::create(size, size, size);
	data->setFirstVoxelPos(origin);
	srand(1337); // Seed some random data

    data->transformCoordinate([origin, size](size_t x, size_t y, size_t z, Voxel) -> Voxel
    {
        float px = (int(x) + origin.x()) / (size - 1) / 2.0f;
        float py = (int(y) + origin.y()) / (size - 1) / 2.0f;
        float pz = (int(z) + origin.z()) / (size - 1) / 2.0f;

        Voxel voxel = fmax(1.0f - sqrtf(px*px + py*py + pz*pz), 0.0f); // Get distance
        voxel = fmax(voxel - 0.1f, 0.0f); // 0.2 Thick radius
        voxel = powf(voxel, 1.8f); // Pretty strong falloff

        // Make some noise!
        voxel += 0.04f * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) - 0.5f);

        return voxel;
    });

    QPair<Voxel, Voxel> range = data->getMinMaxValue();

    data->transform([range](Voxel v) -> Voxel { return (v - range.first) / (range.second - range.first); });

    data->setObjectName("thesphere");
    data->setSpacing(QVector3D(0.7f,0.7f,0.7f));
    return data;
}

voxie::data::DataSet* TheSphereGenerator::genSphere(int size) {
    return registerVoxelData(genSphereImpl(size));
}

QDBusObjectPath TheSphereGeneratorAdaptor::GenerateSphere(int size, const QMap<QString, QVariant>& options) {
    try {
        voxie::scripting::ScriptingContainerBase::checkOptions(options);

        return voxie::scripting::ScriptingContainerBase::getPath(object->genSphere(size));
    } catch (voxie::scripting::ScriptingException& e) {
        e.handle(object);
        return voxie::scripting::ScriptingContainerBase::getPath(nullptr);
    }
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
