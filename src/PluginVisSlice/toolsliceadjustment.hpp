#pragma once

#include <PluginVisSlice/toolvisualizer2d.hpp>

#include <QtWidgets/QPushButton>

/**
 * @brief The SliceAdjustmentTool class provides ways to modify the associated slice using simple user input.
 */

class SliceAdjustmentTool : public Visualizer2DTool
{
    Q_OBJECT
public:
    SliceAdjustmentTool(QWidget *parent, SliceVisualizer* sv);
    ~SliceAdjustmentTool(){}

    QIcon getIcon() override {return QIcon(":/icons/controller-d-pad.png");}
    QString getName() override{return "Slice Adjustment";}
public slots:
    void activateTool() override;
    void deactivateTool() override;
    void toolMousePressEvent(QMouseEvent *ev) override;
    void toolMouseReleaseEvent(QMouseEvent *ev) override;
    void toolMouseMoveEvent(QMouseEvent *ev) override;
    void toolKeyPressEvent(QKeyEvent *ev) override;
    void toolKeyReleaseEvent(QKeyEvent *ev) override;
    void toolWheelEvent(QWheelEvent *ev) override;

    void sliceChanged(const voxie::data::Plane &oldPlane, const voxie::data::Plane &newPlane, bool equivalent) override
        {Q_UNUSED(slice);Q_UNUSED(oldPlane);Q_UNUSED(newPlane);Q_UNUSED(equivalent); this->draw();}
    void sliceImageChanged(voxie::data::SliceImage& si) override {Q_UNUSED(si);}
    virtual void onResize() override {this->draw();}
private:
    SliceVisualizer* sv;
    voxie::data::Slice* slice;
    QPushButton* adjustButton;
    bool isActive = false;
    bool dragUpdates = true;

    bool shiftDown = false;
    bool ctrlDown = false;
    bool xDown = false;
    bool yDown = false;

    bool rotatingInProgress = false;
    QVector2D rotationHypotenuse;

    //
    float tiltAngle = 1;
    float fineAdjustmentFactor = 0.1f;

    void draw();
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
