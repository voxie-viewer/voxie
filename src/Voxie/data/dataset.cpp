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

DataSet::DataSet(QScopedPointer<VoxelData>& data, QObject *parent) :
    voxie::scripting::ScriptingContainer("DataSet", parent) {
    new DataSetAdaptor(this);

    originalDataSet = data.take();
    if (originalDataSet->parent() == nullptr)
        originalDataSet->setParent(this);
    else
        qCritical() << "Warning: VoxelData already has a parent";
    connect(originalDataSet, &VoxelData::changed, [this]() {
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
        this->filteredDataSet->setParent(this);
        connect(filteredDataSet, &VoxelData::changed, [this]() {
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
    QScopedPointer<VoxelData> voxelData(new VoxelData(100,100,100,nullptr));
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
    return voxie::scripting::ScriptingContainerBase::getPath(object->originalData());
}

QDBusObjectPath DataSetAdaptor::filteredData () {
    return voxie::scripting::ScriptingContainerBase::getPath(object->filteredData());
}

QString DataSetAdaptor::displayName () {
    return object->objectName();
}

QDBusObjectPath DataSetAdaptor::CreateSlice(const QMap<QString, QVariant>& options) {
    try {
        voxie::scripting::ScriptingContainerBase::checkOptions(options);
        return voxie::scripting::ScriptingContainerBase::getPath(object->createSlice());
    } catch (voxie::scripting::ScriptingException& e) {
        e.handle(object);
        return voxie::scripting::ScriptingContainerBase::getPath(nullptr);
    }
}

QList<QDBusObjectPath> DataSetAdaptor::ListSlices () {
    QList<QDBusObjectPath> paths;

    for (Slice* slice : object->getSlices ()) {
        paths.append (voxie::scripting::ScriptingContainerBase::getPath (slice));
    }

    return paths;
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
