#pragma once

#include <Voxie/data/voxeldata.hpp>

#include <Voxie/visualization/visualizer.hpp>

#include <Voxie/lib/CL/cl.hpp>

#include <QtGui/QImage>

#include <QtWidgets/QLineEdit>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSlider>

class XRayVisualizer :
        public voxie::visualization::VolumeDataVisualizer
{
    Q_OBJECT
private:
    voxie::data::DataSet *dataSet_;
    QImage image;
    cl::Kernel kernel;
    cl::Image2D clImage;
    float pan, tilt, zoom;
    QPoint mouseLast;
private:
    QRadioButton *radioQ0, *radioQ1, *radioQ2, *radioQ3;
    QSlider *minSlider, *maxSlider, *scaleSlider;
public:
    explicit XRayVisualizer(voxie::data::DataSet *dataSet, QWidget *parent = 0);

    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void wheelEvent(QWheelEvent *event) override;
    virtual void paintEvent(QPaintEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;

	virtual voxie::data::DataSet* dataSet() final {
		return this->dataSet_;
	}

private:
    void updateButton(bool stub) { (void)stub; this->update(); }
    void updateSlider(int stub) { (void)stub; this->update(); }

signals:

public slots:

};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
