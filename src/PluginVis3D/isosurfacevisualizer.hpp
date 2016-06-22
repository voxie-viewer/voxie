#pragma once

#include <PluginVis3D/isosurfaceview.hpp>

#include <Voxie/data/voxeldata.hpp>

#include <Voxie/visualization/visualizer.hpp>

#include <QtOpenGL/QGLWidget>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>

class IsosurfaceVisualizer :
        public voxie::visualization::VolumeDataVisualizer
{
    Q_OBJECT

    voxie::data::DataSet *voxelData_;
    IsosurfaceView *view;

    QDoubleSpinBox* thresholdEdit;
    QComboBox* methodBox;
    QCheckBox* invertedCheck;

    QComboBox* culling;

public:
    explicit IsosurfaceVisualizer(voxie::data::DataSet *voxelData, QWidget *parent = 0);

	virtual voxie::data::DataSet* dataSet() final {
		return this->voxelData_;
	}

private slots:
    void refresh3D();

    void saveAsSTL();
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
