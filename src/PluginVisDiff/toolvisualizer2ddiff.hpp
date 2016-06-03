#pragma once

#include <PluginVisDiff/toolvisualizer2d.hpp>

#include <Voxie/data/slice.hpp>
#include <Voxie/data/sliceimage.hpp>

#include <QtCore/QObject>

#include <QtGui/QIcon>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>

#include <QtWidgets/QWidget>

/*class DiffVisualizer;

class Visualizer2DDiffTool : public Visualizer2DTool
{
    Q_OBJECT
public:
    Visualizer2DDiffTool(QWidget *parent);
    ~Visualizer2DDiffTool();

public slots:
    virtual void sliceChanged(const voxie::data::Plane &oldPlane, const voxie::data::Plane &newPlane, bool equivalent) override {
        Q_UNUSED(oldPlane); Q_UNUSED(newPlane); Q_UNUSED(equivalent);
    }
    virtual void firstSliceChanged(const voxie::data::Plane &oldPlane, const voxie::data::Plane &newPlane, bool equivalent) = 0;
    virtual void secondSliceChanged(const voxie::data::Plane &oldPlane, const voxie::data::Plane &newPlane, bool equivalent) = 0;
};

*/

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
