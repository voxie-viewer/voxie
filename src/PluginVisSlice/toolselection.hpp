#pragma once

#include <PluginVisSlice/toolvisualizer2d.hpp>

#include <Voxie/filtermask/selection2dmask.hpp>

#include <QtGui/QPainterPath>
#include <QtGui/QTransform>

#include <QtWidgets/QPushButton>

using namespace voxie::filter;

/**
 * @brief The ToolSelection class provides ways to define a selection mask to selectively apply filters to a slice.
 */

class ToolSelection : public Visualizer2DTool
{
    Q_OBJECT
public:
    ToolSelection(QWidget *parent, SliceVisualizer* sv);

    QIcon getIcon() override;
    QString getName() override;
public slots:
    void activateTool() override;
    void deactivateTool() override;
    void toolMousePressEvent(QMouseEvent *e) override;
    void toolMouseReleaseEvent(QMouseEvent *e) override;
    void toolMouseMoveEvent(QMouseEvent *e) override;
    void toolKeyPressEvent(QKeyEvent *e) override;
    void toolKeyReleaseEvent(QKeyEvent *e) override{Q_UNUSED(e);}
    void toolWheelEvent(QWheelEvent *e) override{Q_UNUSED(e);}
    void setMask(Selection2DMask* mask);
    void sliceChanged(const voxie::data::Plane &oldPlane, const voxie::data::Plane &newPlane, bool equivalent) override
        {Q_UNUSED(oldPlane);Q_UNUSED(newPlane);Q_UNUSED(equivalent);}
    void sliceImageChanged(voxie::data::SliceImage& si) override { Q_UNUSED(si); draw(); }


private:
    SliceVisualizer* sv;
    bool toolActive = false;

    QPushButton* rectangleButton;
    QPushButton* ellipseButton;
    QPushButton* polygonButton;
    QPushButton* deleteButton;

    Selection2DMask* mask = nullptr;

    bool rectangleActive = false;
    bool ellipseActive = false;
    bool polygonActive = false;
    bool deleteActive = false;
    bool previewActive = false;
    bool mousePressed = false;

    QVector<QPointF> previewPolygon;
    QVector<QPointF> polygon;
    QPointF startRect;
    QPointF middlePointEllipse;
    QPoint start;
    bool firstValue = true;
    QPainterPath preview;

    void draw();
    QPainterPath planeToImage(QPainterPath path);
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
