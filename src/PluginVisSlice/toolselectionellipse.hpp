#pragma once

#include <PluginVisSlice/toolvisualizer2d.hpp>

#include <QtWidgets/QPushButton>

//class ToolSelectionEllipse : public Visualizer2DTool
//{
//    Q_OBJECT
//public:
//    ToolSelectionEllipse(QWidget *parent, SliceVisualizer* sv);
//    ~ToolSelectionEllipse();

//    QIcon getIcon() override {return QIcon(":/icons/ruler--pencil.png");}
//    QString getName() override{return "selection ellipse";}
//public slots:
//    void activateTool() override;
//    void deactivateTool() override;
//    void toolMousePressEvent(QMouseEvent *) override {}
//    void toolMouseReleaseEvent(QMouseEvent *) override {}
//    void toolKeyPressEvent(QKeyEvent *) override {}
//    void toolKeyReleaseEvent(QKeyEvent *) override {}
//    void toolWheelEvent(QWheelEvent *) override;
//    void sliceChanged(const voxie::data::Plane &oldPlane, const voxie::data::Plane &newPlane, bool equivalent) override
//        {Q_UNUSED(oldPlane);Q_UNUSED(newPlane);Q_UNUSED(equivalent);}
//    void sliceImageChanged(voxie::data::SliceImage& si) override {Q_UNUSED(si);}
//    virtual void requestedImageUpdate(size_t width, size_t height, qreal zoom, qreal x, qreal y) override
//        {Q_UNUSED(width); Q_UNUSED(height); Q_UNUSED(zoom); Q_UNUSED(x); Q_UNUSED(y);}
//signals:
//    void signalImageDrawn(QImage im);
//private:
//    SliceVisualizer* sv;
//    QPushButton* selectionButton;
//    bool mousePressed;
//};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
