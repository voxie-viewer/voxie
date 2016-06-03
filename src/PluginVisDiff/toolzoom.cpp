#include "toolzoom.hpp"

#include <PluginVisDiff/diffvisualizer.hpp>

#include <QtWidgets/QGridLayout>

ToolZoom::ToolZoom(QWidget *parent, DiffVisualizer* sv) :
    Visualizer2DTool(parent),
    sv(sv)
{
    QGridLayout *layout = new QGridLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    layout->setMargin(0);
    zoomButton = new QPushButton(getIcon(), getName());
    zoomButton->setCheckable(true);
    connect(zoomButton, &QPushButton::clicked, [=]() {
        this->sv->switchToolTo(this);
    });
    //zoomButton->setUpdatesEnabled(false);
    layout->addWidget(zoomButton,0,0);
    zoomButton->show();

    dragRedraw = new QCheckBox("Drag redraw?");
    dragRedraw->setChecked(true);
    layout->addWidget(dragRedraw,0,2);
    dragRedraw->hide();

    this->setLayout(layout);

    dragStart = QPoint(-1,-1);
}

ToolZoom::~ToolZoom() {
}

QIcon ToolZoom::getIcon() {
    return QIcon(":/icons/arrow-move");
}

QString ToolZoom::getName() {
    return "&Zoom / Move";
}

void ToolZoom::activateTool() {
    //qDebug() << "activate zoom";
    zoomButton->setChecked(true);
    dragRedraw->show();
}

void ToolZoom::deactivateTool() {
    //qDebug() << "deactivate zoom";
    zoomButton->setChecked(false);
    dragRedraw->hide();
}

void ToolZoom::toolWheelEvent(QWheelEvent* e) {
    qreal zoom = (e->delta() > 0) ?  0.9f : 1.1f;
    sv->zoomPlaneArea(zoom);
    sv->signalRequestSliceImageUpdate();
}

void ToolZoom::toolMousePressEvent(QMouseEvent *e) {
    dragStart = e->pos();
}

void ToolZoom::toolMouseMoveEvent(QMouseEvent *e) {
    if(dragRedraw->isChecked()) {
        if(dragStart.x() >= 0) {
            int deltaX = e->pos().x() - dragStart.x();
            int deltaY = e->pos().y() - dragStart.y();
            sv->moveArea(deltaX, deltaY);
            sv->signalRequestSliceImageUpdate();
            dragStart = e->pos();
        }
    }
}

void ToolZoom::toolMouseReleaseEvent(QMouseEvent *e) {
    if(!dragRedraw->isChecked()) {
        if(dragStart.x() >= 0) {
            int deltaX = e->pos().x() - dragStart.x();
            int deltaY = e->pos().y() - dragStart.y();
            sv->moveArea(deltaX, deltaY);
            sv->signalRequestSliceImageUpdate();
        }
    }
    dragStart = QPoint(-1,-1);
}

void ToolZoom::sliceChanged(const voxie::data::Plane &oldPlane, const voxie::data::Plane &newPlane, bool equivalent) {
    Q_UNUSED(oldPlane);
    Q_UNUSED(newPlane);
    Q_UNUSED(equivalent);
}

void ToolZoom::sliceImageChanged(voxie::data::SliceImage &si) {
    Q_UNUSED(si);
}


// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
