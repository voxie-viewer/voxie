#include "hdfloader.hpp"

#include <PluginHDF5/CT/DataFiles.hpp>

#include <Voxie/scripting/scriptingexception.hpp>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTreeWidgetItemIterator>

using namespace voxie::data;
using namespace voxie::scripting;

using namespace ::internal;

HDFLoader::HDFLoader(QObject* parent) :
    Loader(Filter("HDF5 Files", {"*.h5", "*.hdf5"}), parent)
{
}

HDFLoader::~HDFLoader()
{
}

static QVector3D toQVector (Math::Vector3<double> vec) {
    return QVector3D(vec.x(), vec.y(), vec.z());
}
static QVector3D toQVector (Math::DiagMatrix3<double> vec) {
    return QVector3D(vec.m11(), vec.m22(), vec.m33());
}

voxie::data::VoxelData* HDFLoader::loadImpl(const QString &fileName) {
    // check if file exists
    QFile qFile(fileName);
    if (!qFile.exists()) {
        throw ScriptingException("de.uni_stuttgart.Voxie.HDFLoader.FileNotFound", "File not found");
    }

    QScopedPointer<VoxelData> voxelData;
    try {
        boost::shared_ptr<VolumeGen<float, true> > volume = HDF5::matlabDeserialize<VolumeGen<float, true> >(fileName.toUtf8().data());
        Math::Vector3<size_t> size = getSize(*volume);

        // create and fill the voxel data object
        voxelData.reset(new VoxelData(size.x(), size.y(), size.z(), nullptr));
        size_t shape[3] = { size.x(), size.y(), size.z() };
        ptrdiff_t stridesBytes[3] = {
            (ptrdiff_t) (sizeof (float)),
            (ptrdiff_t) (size.x() * sizeof (float)),
            (ptrdiff_t) (size.x() * size.y() * sizeof (float))
        };
        Math::ArrayView<float, 3> view(voxelData->getData(), shape, stridesBytes);
        loadAndTransformTo<float /*should be float even if half or integer type is used for volume data*/>(*volume, view);

        // read meta data
        if (volume->GridOrigin)
            voxelData->setFirstVoxelPos(toQVector(*volume->GridOrigin));
        else
            voxelData->setFirstVoxelPos(QVector3D(0, 0, 0));
        if (volume->GridSpacing)
            voxelData->setSpacing(toQVector(*volume->GridSpacing));
        else
            voxelData->setSpacing(QVector3D(1, 1, 1));

        voxelData->setObjectName(QFileInfo(fileName).fileName());
    } catch (std::exception& e) {
        throw ScriptingException("de.uni_stuttgart.Voxie.HDFLoader.Error", QString() + "Failure while loading file: " + e.what());
    }

    return voxelData.take();
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
