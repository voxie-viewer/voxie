#pragma once

#include <PluginVisSlice/toolvisualizer2d.hpp>

#include <QtWidgets/QPushButton>

/**
 * @brief The ValueViewerTool class provides ways to measure and display values in an image.
 * @author David Haegele, Hans Martin Berner
 */

class ValueViewerTool: public Visualizer2DTool
{
    Q_OBJECT
public:
    ValueViewerTool(QWidget *parent, SliceVisualizer* sv);
    ~ValueViewerTool(){}



    QIcon getIcon() override {return QIcon(":/icons/ruler-triangle.png");}
    QString getName() override{return "Value Viewer";}
public slots:
    void activateTool() override;
    void deactivateTool() override;
    void toolMousePressEvent(QMouseEvent * e) override;
    void toolMouseReleaseEvent(QMouseEvent * ev) override;
    void toolMouseMoveEvent(QMouseEvent * e) override;
    void toolKeyPressEvent(QKeyEvent * e) override;
    void toolKeyReleaseEvent(QKeyEvent * e) override;
    void toolWheelEvent(QWheelEvent * e) {Q_UNUSED(e)}
    void sliceChanged(const voxie::data::Plane &oldPlane, const voxie::data::Plane &newPlane, bool equivalent) override
        {Q_UNUSED(oldPlane);Q_UNUSED(newPlane);Q_UNUSED(equivalent);}
    void sliceImageChanged(voxie::data::SliceImage& si);

private:
    void updateValueAtMouse(bool showFilteredImage);

private:
    SliceVisualizer* sv;
    QPushButton* valueButton;
    bool mousePressed;
    QPoint startPos;
	QImage persistentImg;
	QImage tempImg;

    QPoint mousePos;
    bool mousePosValid = false;
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
