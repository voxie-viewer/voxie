#pragma once

#include <PluginVisSlice/slicevisualizer.hpp>
#include <PluginVisSlice/toolvisualizer2d.hpp>

#include <QtGui/QIcon>

#include <QtWidgets/QPushButton>

/**
 * @brief The ToolExport class provides a button that can be used to export the currently displayed image to an image file.
 * @author Hans Martin Berner
 */

class ToolExport : public Visualizer2DTool
{
public:
	Q_OBJECT
public:
    ToolExport(QWidget *parent, SliceVisualizer* sv);
    ~ToolExport(){}

    QIcon getIcon() override {return QIcon(":/icons/image-export.png");}
	QString getName();
public slots:
	void activateTool() override;
	void deactivateTool() override {}
	void toolMousePressEvent(QMouseEvent * e) override {Q_UNUSED(e)}
    void toolMouseReleaseEvent(QMouseEvent * e) override {Q_UNUSED(e)}
    void toolMouseMoveEvent(QMouseEvent * e) override {Q_UNUSED(e)}
    void toolKeyPressEvent(QKeyEvent * e) override {Q_UNUSED(e)}
    void toolKeyReleaseEvent(QKeyEvent * e) override {Q_UNUSED(e)}
    void toolWheelEvent(QWheelEvent * e) override {Q_UNUSED(e)}
    void sliceChanged(const voxie::data::Plane &oldPlane, const voxie::data::Plane &newPlane, bool equivalent) override
        {Q_UNUSED(oldPlane);Q_UNUSED(newPlane);Q_UNUSED(equivalent);}
    void sliceImageChanged(voxie::data::SliceImage& si) override {Q_UNUSED(si);}
private:
	void saveDialog();
    SliceVisualizer* sv;
	QPushButton* saveButton;
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
