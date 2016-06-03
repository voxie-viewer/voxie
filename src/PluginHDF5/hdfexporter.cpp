#include "hdfexporter.hpp"

#include <PluginHDF5/CT/DataFiles.hpp>

#include <Voxie/data/dataset.hpp>
#include <Voxie/data/voxeldata.hpp>

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

using namespace voxie::data;

HDFExporter::HDFExporter(QObject* parent) :
	VoxelExporter(parent)
{
}

HDFExporter::~HDFExporter()
{
}

void HDFExporter::exportGui(voxie::data::DataSet *dataSet)
{
    QString filename = QFileDialog::getSaveFileName(
        NULL,
        tr("Save Document"),
        QDir::currentPath(),
        "HDF5-File (*.hdf5 *.h5)" );
    if( !filename.isNull() )
    {
        write(filename, dataSet);
        //qDebug() << filename;
    }
}

void HDFExporter::write(const QString &fileName, voxie::data::DataSet* dataSet) {
    VolumeGen<float, true> volume;
    volume.Type = "Volume";
    volume.GridOrigin = Math::Vector3<ldouble>(dataSet->filteredData()->getFirstVoxelPosition().x(), dataSet->filteredData()->getFirstVoxelPosition().y(), dataSet->filteredData()->getFirstVoxelPosition().z());
    volume.GridSpacing = Math::DiagMatrix3<ldouble>(dataSet->filteredData()->getSpacing().x(), dataSet->filteredData()->getSpacing().y(), dataSet->filteredData()->getSpacing().z());
    volume.Volume.size[0] = dataSet->filteredData()->getDimensions().x;
    volume.Volume.size[1] = dataSet->filteredData()->getDimensions().y;
    volume.Volume.size[2] = dataSet->filteredData()->getDimensions().z;
    
    try {
        HDF5::matlabSerialize (fileName.toUtf8().data(), volume);
        size_t shape[3] = { dataSet->filteredData()->getDimensions().x, dataSet->filteredData()->getDimensions().y, dataSet->filteredData()->getDimensions().z };
        ptrdiff_t stridesBytes[3] = {
            (ptrdiff_t) (sizeof (float)),
            (ptrdiff_t) (dataSet->filteredData()->getDimensions().x * sizeof (float)),
            (ptrdiff_t) (dataSet->filteredData()->getDimensions().x * dataSet->filteredData()->getDimensions().y * sizeof (float))
        };
        Math::ArrayView<const float, 3> view(dataSet->filteredData()->getData(), shape, stridesBytes);
        volume.Volume.write(view);
    } catch (std::exception& e) {
        QMessageBox messageBox;
        messageBox.critical(0, "Error", e.what());
        messageBox.setFixedSize(500,200);
        return;
    }
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
