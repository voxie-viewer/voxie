#include "hdfsliceexporter.hpp"

#include <Voxie/data/slice.hpp>

#include <hdf5.h>

#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLayout>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>

using namespace voxie::data;

HDFSliceExporter::HDFSliceExporter(QObject* parent) :
	SliceExporter(parent)
{
}

HDFSliceExporter::~HDFSliceExporter()
{
}

void HDFSliceExporter::exportGui(Slice *slice)
{
	QFileDialog fileDialog(nullptr, Qt::Dialog);
	fileDialog.setOption(QFileDialog::DontUseNativeDialog);
	fileDialog.setAcceptMode(QFileDialog::AcceptSave);
	fileDialog.setNameFilter("HDF5 files (*.h5 *.hdf5)");
	fileDialog.setDefaultSuffix(".h5");

	QGroupBox interpolationBox("Interpolation");
	QComboBox comboBox;
	comboBox.addItem("Linear");
	comboBox.addItem("Nearest Neighbor");
	QBoxLayout test(QBoxLayout::LeftToRight);
	interpolationBox.setLayout(&test);
	interpolationBox.layout()->addWidget(&comboBox);
	fileDialog.layout()->addWidget(&interpolationBox);

	QGroupBox boundBox("Bounds");
	QGridLayout boundsLayout(&boundBox);
	boundsLayout.setColumnStretch(0, 10);
	boundsLayout.setColumnStretch(1, 1000);
	boundsLayout.setColumnStretch(2, 1000);
	QDoubleSpinBox xBox;
	QDoubleSpinBox yBox;
	xBox.setMinimum(-9999);
	yBox.setMinimum(-9999);
	xBox.setMaximum(9999);
	yBox.setMaximum(9999);
	xBox.setValue(slice->getBoundingRectangle().x());
	yBox.setValue(slice->getBoundingRectangle().y());

	QDoubleSpinBox xMaxBox;
	QDoubleSpinBox yMaxBox;
	xMaxBox.setMinimum(-9999);
	yMaxBox.setMinimum(-9999);
	xMaxBox.setMaximum(9999);
	yMaxBox.setMaximum(9999);
	xMaxBox.setValue(slice->getBoundingRectangle().x() + slice->getBoundingRectangle().width());
	yMaxBox.setValue(slice->getBoundingRectangle().y() + slice->getBoundingRectangle().height());
	QLabel xLabel("x");
	QLabel yLabel("y");
	boundsLayout.addWidget(&xLabel);
	boundsLayout.addWidget(&xBox);
	boundsLayout.addWidget(&xMaxBox);
	boundsLayout.addWidget(&yLabel);
	boundsLayout.addWidget(&yBox);
	boundsLayout.addWidget(&yMaxBox);

	fileDialog.layout()->addWidget(&boundBox);

	QGroupBox sizeBox("Image Size");
	QGridLayout sizeLayout(&sizeBox);
	sizeBox.setLayout(&sizeLayout);
	sizeLayout.setColumnStretch(0, 1);
	sizeLayout.setColumnStretch(1, 100);
	QLabel widthLabel("width");
	QLabel heigthLabel("height");
	QSpinBox widthBox;
	QSpinBox heightBox;
	widthBox.setMaximum(9999);
	heightBox.setMaximum(9999);
	widthBox.setValue(512);
	heightBox.setValue(512);
	sizeLayout.addWidget(&widthLabel);
	sizeLayout.addWidget(&widthBox);
	sizeLayout.addWidget(&heigthLabel);
	sizeLayout.addWidget(&heightBox);
	fileDialog.layout()->addWidget(&sizeBox);

	if (fileDialog.exec()) {
		QString file = fileDialog.selectedFiles().first();
		float x = xBox.value();
		float y = yBox.value();
		float width = xMaxBox.value() - x;
		float height = yMaxBox.value() - y;
		float sizeX = widthBox.value();
		float sizeY = heightBox.value();
		//InterpolationMethod
		if (comboBox.currentIndex() == 0) {
			exportSlice(file, slice, x, y, width, height, sizeX, sizeY, linear);
		} else {
			exportSlice(file, slice, x, y, width, height, sizeX, sizeY, nearestNeighbor);
		}
	}

}

void HDFSliceExporter::exportSlice(QString fileName, Slice *slice, float x, float y, float width, float height, float sizeX, float sizeY, InterpolationMethod interpolation)
{
    SliceImage sliceImg = slice->generateImage(QRectF(x, y, width, height), QSize(sizeX, sizeY), interpolation);

	hid_t file, hdataSet;
	hid_t dataType, dataSpace;

	uint flags = H5F_ACC_TRUNC;
	file = H5Fcreate(fileName.toStdString().c_str(), flags, H5P_DEFAULT, H5P_DEFAULT );
    if (file<0) {
        H5Fclose(file);
        QMessageBox messageBox;
        messageBox.critical(0,"Error","Data could not be written.");
        messageBox.setFixedSize(500,200);
        return;
    }

	dataType = H5Tcopy(H5T_NATIVE_FLOAT);
    if (dataType<0) {
        H5Fclose(file);
        H5Tclose(dataType);
        QMessageBox messageBox;
        messageBox.critical(0,"Error","Datatype could not be created.");
        messageBox.setFixedSize(500,200);
        return;
    }

	// create dimension
	hsize_t dimX = static_cast<hsize_t>(sizeX);
	hsize_t dimY = static_cast<hsize_t>(sizeY);
	hsize_t dimension[2] = {dimX, dimY};

	// create dataspace
	dataSpace = H5Screate_simple(2, dimension, NULL);
    if (dataSpace<0) {
        H5Fclose(file);
        H5Tclose(dataType);
        H5Sclose(dataSpace);
        QMessageBox messageBox;
        messageBox.critical(0,"Error","Dataspace could not be created.");
        messageBox.setFixedSize(500,200);
        return;
    }

	// create dataset
	hdataSet = H5Dcreate1(file, "/Volume", dataType, dataSpace,
						  H5P_DEFAULT);
    if (hdataSet<0) {
        H5Fclose(file);
        H5Tclose(dataType);
        H5Sclose(dataSpace);
        H5Dclose(hdataSet);
        QMessageBox messageBox;
        messageBox.critical(0,"Error","Dataset could not be created.");
        messageBox.setFixedSize(500,200);
        return;
    }

	// create meta data
	hsize_t originDim [] = {3};
	hsize_t spacingDim [] = {3};
	hsize_t areaDim [] = {4};
	hsize_t rotationDim [] = {4};
	hid_t originSpace = H5Screate_simple(1, originDim, NULL);
	hid_t spacingSpace = H5Screate_simple(1, spacingDim, NULL);
	hid_t areaSpace = H5Screate_simple(1, areaDim, NULL);
	hid_t rotationSpace = H5Screate_simple(1, rotationDim, NULL);

	hid_t origin = H5Dcreate1(file, "/Origin", dataType, originSpace, H5P_DEFAULT);
	hid_t spacing = H5Dcreate1(file, "/Spacing", dataType, spacingSpace, H5P_DEFAULT);
	hid_t area = H5Dcreate1(file, "/Area", dataType, areaSpace, H5P_DEFAULT);
	hid_t rotation = H5Dcreate1(file, "/Rotation", dataType, rotationSpace, H5P_DEFAULT);
    if (originSpace<0 || spacingSpace<0 || areaSpace<0 || rotationSpace<0
            || origin<0 || spacing<0 || area<0 || rotation<0) {
        H5Fclose(file);
        H5Tclose(dataType);
        H5Sclose(dataSpace);
        H5Dclose(hdataSet);
        H5Dclose(origin);
        H5Dclose(spacing);
        H5Dclose(area);
        H5Dclose(rotation);
        H5Sclose(originSpace);
        H5Sclose(spacingSpace);
        H5Sclose(areaSpace);
        H5Sclose(rotationSpace);
        QMessageBox messageBox;
        messageBox.critical(0,"Error","Metadata could not be created.");
        messageBox.setFixedSize(500,200);
        return;
    }
    float originArray[] = {sliceImg.context().cuttingPlane.origin.x(), sliceImg.context().cuttingPlane.origin.y(), sliceImg.context().cuttingPlane.origin.z()};
    float spacingArray[] = {sliceImg.context().voxelGridSpacing.x(), sliceImg.context().voxelGridSpacing.y(), sliceImg.context().voxelGridSpacing.z()};
	float areaArray[] = {x, y, width, height};
    float rotationArray[] = {sliceImg.context().cuttingPlane.rotation.scalar(), sliceImg.context().cuttingPlane.rotation.x(), sliceImg.context().cuttingPlane.rotation.y(), sliceImg.context().cuttingPlane.rotation.z()};

    if(H5Dwrite(origin, dataType, originSpace, originSpace, H5P_DEFAULT, originArray)<0
            || H5Dwrite(spacing, dataType, spacingSpace, spacingSpace, H5P_DEFAULT, spacingArray)<0
            || H5Dwrite(area, dataType, areaSpace, areaSpace, H5P_DEFAULT, areaArray)<0
            || H5Dwrite(rotation, dataType, rotationSpace, rotationSpace, H5P_DEFAULT, rotationArray)<0
            || H5Dwrite(hdataSet, dataType, dataSpace, dataSpace, H5P_DEFAULT, sliceImg.getBufferCopy().data())<0) {
        QMessageBox messageBox;
        messageBox.critical(0,"Error","Writing data failed.");
        messageBox.setFixedSize(500,200);
    }

    if(H5Fclose(file)<0
            || H5Tclose(dataType)<0
            || H5Sclose(dataSpace)<0
            || H5Dclose(hdataSet)<0
            || H5Dclose(origin)<0
            || H5Dclose(spacing)<0
            || H5Dclose(area)<0
            || H5Dclose(rotation)<0
            || H5Sclose(originSpace)<0
            || H5Sclose(spacingSpace)<0
            || H5Sclose(areaSpace)<0
            || H5Sclose(rotationSpace)<0) {
        QMessageBox messageBox;
        messageBox.critical(0,"Error","Failed to close file handlers.");
        messageBox.setFixedSize(500,200);
    }
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
