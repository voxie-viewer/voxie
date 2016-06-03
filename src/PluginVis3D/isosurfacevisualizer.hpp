#pragma once

#include <PluginVis3D/isosurfaceview.hpp>

#include <Voxie/data/voxeldata.hpp>

#include <Voxie/visualization/visualizer.hpp>

#include <QtOpenGL/QGLWidget>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSlider>

class IsosurfaceVisualizer :
        public voxie::visualization::VolumeDataVisualizer
{
    Q_OBJECT
private:
    voxie::data::DataSet *voxelData_;
    IsosurfaceView *view;

    QLineEdit *thresholdEdit;
    QSlider *thresholdSlider;
    QComboBox *methodBox;
    QCheckBox *invertedCheck;
public:
    explicit IsosurfaceVisualizer(voxie::data::DataSet *voxelData, QWidget *parent = 0);

	virtual voxie::data::DataSet* dataSet() final {
		return this->voxelData_;
	}

private slots:
    void refresh3D();

    void refreshTextEdit(int value);

    void refreshSlider(const QString &text);

signals:

public slots:

};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
