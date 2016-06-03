#pragma once

#include <PluginVisSlice/toolvisualizer2d.hpp>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>

/**
 * @brief The ToolZoom class provides methods to zoom and drag the currently displayed image.
 * @author Hans Martin Berner, David Haegele
 */

class ToolZoom : public Visualizer2DTool
{
    Q_OBJECT
public:
    ToolZoom(QWidget *parent, SliceVisualizer* sv);
    ~ToolZoom();

    QIcon getIcon() override;
    QString getName() override;
private:
    QPushButton* zoomButton;
    SliceVisualizer* sv;
    QPoint dragStart;
    bool dragStartValid = false;
    //QDoubleSpinBox* zoomBox;
    QCheckBox* dragRedraw;
    //QDoubleSpinBox* xBox;
    //QDoubleSpinBox* yBox;
public slots:
    void activateTool() override;
    void deactivateTool() override;
    void toolMousePressEvent(QMouseEvent * e) override;
    void toolMouseReleaseEvent(QMouseEvent * e) override;
    void toolMouseMoveEvent(QMouseEvent * e) override;
    void toolKeyPressEvent(QKeyEvent * e) {Q_UNUSED(e)}
    void toolKeyReleaseEvent(QKeyEvent * e) {Q_UNUSED(e)}
    void toolWheelEvent(QWheelEvent * e) override;
    void sliceChanged(const voxie::data::Plane &oldPlane, const voxie::data::Plane &newPlane, bool equivalent) override;
    void sliceImageChanged(voxie::data::SliceImage& si) override;
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
