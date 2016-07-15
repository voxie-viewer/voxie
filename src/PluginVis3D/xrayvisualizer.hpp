#pragma once

#include <Voxie/data/voxeldata.hpp>

#include <Voxie/visualization/view3d.hpp>
#include <Voxie/visualization/visualizer.hpp>

#include <Voxie/lib/CL/cl.hpp>

#include <QtGui/QImage>

#include <QtWidgets/QLineEdit>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSlider>

class XRayVisualizer;

class XRayView : public QWidget {
    Q_OBJECT

    friend class XRayVisualizer;

private:
    XRayVisualizer* visualizer;

    QImage image;
    cl::Kernel kernel;
    cl::Image2D clImage;
    QPoint mouseLast;
    voxie::visualization::View3D* view3d;

    QWidget *sidePanel;
    QRadioButton *radioQ0, *radioQ1, *radioQ2, *radioQ3;
    QSlider *minSlider, *maxSlider, *scaleSlider;

public:
    explicit XRayView(XRayVisualizer* visualizer);

    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void wheelEvent(QWheelEvent *event) override;
    virtual void paintEvent(QPaintEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;

private:
    void updateButton(bool stub) { (void)stub; this->update(); }
    void updateSlider(int stub) { (void)stub; this->update(); }

    inline voxie::data::DataSet* dataSet();
};

class XRayVisualizer : public voxie::visualization::VolumeDataVisualizer {
    Q_OBJECT
private:
    voxie::data::DataSet *dataSet_;
    XRayView* view;

public:
    explicit XRayVisualizer(voxie::data::DataSet *dataSet, QWidget *parent = 0);

    voxie::plugin::MetaVisualizer* type() const override;

	virtual voxie::data::DataSet* dataSet() final {
		return this->dataSet_;
	}

    QWidget* mainView() override {
        return view;
    }
};

inline voxie::data::DataSet* XRayView::dataSet() {
    return this->visualizer->dataSet();
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
