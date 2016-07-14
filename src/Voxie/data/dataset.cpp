#include "dataset.hpp"

#include <Voxie/ivoxie.hpp>

#include <Voxie/data/image.hpp>
#include <Voxie/data/slice.hpp>
#include <Voxie/data/voxeldata.hpp>

#include <Voxie/opencl/clinstance.hpp>

#include <QtCore/QRunnable>
#include <QtCore/QThreadPool>
#include <QtCore/QUuid>

using namespace voxie::data;
using namespace voxie::data::internal;

DataSet::DataSet(const QSharedPointer<VoxelData>& data, QObject *parent) :
    voxie::scripting::ScriptableObject("DataSet", parent) {
    new DataSetAdaptor(this);

    originalDataSet = data;
    connect(originalDataSet.data(), &VoxelData::changed, [this]() {
            if (filteredDataSet == nullptr) {
                emit DataSet::changed();
            }
        });
}

DataSet::~DataSet()
{
  //delete[] this->data;
	// slices delete themselves
}

void DataSet::resetData()
{
    if (this->filteredDataSet == nullptr) {
        this->filteredDataSet = originalDataSet->clone();
        connect(filteredDataSet.data(), &VoxelData::changed, [this]() {
                emit DataSet::changed();
            });
    } else {
        memcpy(this->filteredDataSet->getData(), this->originalDataSet->getData(), this->originalDataSet->getByteSize());
        this->filteredDataSet->invalidate();
    }
}


Slice* DataSet::createSlice()
{
	auto slice = new Slice(this);
	voxie::voxieRoot().registerSlice(slice);
    emit sliceCreated(slice);
    return slice;
}

QString DataSet::createSlice(QString sliceName)
{
    auto slice = new Slice(this);
    if(!sliceName.isEmpty())
        slice->setObjectName(sliceName);
    voxie::voxieRoot().registerSlice(slice);
    emit sliceCreated(slice);
    return slice->objectName();
}

QVector3D DataSet::origin() const {
    return originalData()->getFirstVoxelPosition();
}

QVector3D DataSet::size() const {
    return originalData()->getDimensionsMetric();
}

float DataSet::diagonalSize() const {
    return size().length();
}

QVector3D DataSet::volumeCenter() const {
    return origin() + size() / 2;
}

DataSet* DataSet::getTestDataSet()
{
    QSharedPointer<VoxelData> voxelData = VoxelData::create(100, 100, 100);
	DataSet* dataset = new DataSet(voxelData);
	Voxel* data = dataset->originalData()->getData();
	for(size_t i = 0; i < 100*100*100; i++){
		data[i] = i / (100.0f*100*100);
	}
	return dataset;
}

QList<Slice*> DataSet::getSlices()
{
	return this->findChildren<Slice*>();
}

QDBusObjectPath DataSetAdaptor::originalData () {
    return voxie::scripting::ScriptableObject::getPath(object->originalData().data());
}

QDBusObjectPath DataSetAdaptor::filteredData () {
    return voxie::scripting::ScriptableObject::getPath(object->filteredData().data());
}

QString DataSetAdaptor::displayName () {
    return object->objectName();
}

QDBusObjectPath DataSetAdaptor::CreateSlice(const QMap<QString, QVariant>& options) {
    try {
        voxie::scripting::ScriptableObject::checkOptions(options);
        return voxie::scripting::ScriptableObject::getPath(object->createSlice());
    } catch (voxie::scripting::ScriptingException& e) {
        e.handle(object);
        return voxie::scripting::ScriptableObject::getPath(nullptr);
    }
}

QList<QDBusObjectPath> DataSetAdaptor::ListSlices () {
    QList<QDBusObjectPath> paths;

    for (Slice* slice : object->getSlices ()) {
        paths.append (voxie::scripting::ScriptableObject::getPath (slice));
    }

    return paths;
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
