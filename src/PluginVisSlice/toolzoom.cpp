#include "toolzoom.hpp"

#include <PluginVisSlice/slicevisualizer.hpp>

#include <QtWidgets/QGridLayout>

ToolZoom::ToolZoom(QWidget *parent, SliceVisualizer* sv) :
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

//	zoomBox = new QDoubleSpinBox();
//	zoomBox->setSingleStep(0.1);
//	zoomBox->setPrefix("zoom : ");
//	zoomBox->setValue(this->sv->currentZoom());
//	zoomBox->setMinimum(0);
//	zoomBox->setMaximum(std::numeric_limits <double>::max());
//	zoomBox->setFixedWidth(80);
//	connect(zoomBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [=](double d) {
//		if(zoomBox->hasFocus()) {
//			this->sv->signalRequestSliceImageUpdate(
//				this->sv->canvasWidth(),
//				this->sv->canvasHeight(),
//				(qreal)d,
//				0,
//				0);
//		}
//	});
//	layout->addWidget(zoomBox,0,1);
//	zoomBox->show();

    dragRedraw = new QCheckBox("Drag redraw?");
    dragRedraw->setChecked(true);
    layout->addWidget(dragRedraw,0,2);
    dragRedraw->hide();

//	xBox = new QDoubleSpinBox();
//	xBox->setPrefix("x : ");
//	xBox->setSuffix("px");
//	xBox->setSingleStep(1);
//	xBox->setRange(std::numeric_limits <double>::lowest(), std::numeric_limits <double>::max());
//	xBox->setValue(this->sv->currentPixelDeltaX());
//	xBox->setFixedWidth(80);
//	connect(xBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [=](double v) {
//		if(xBox->hasFocus()) {
//			this->sv->signalRequestSliceImageUpdate(
//				this->sv->canvasWidth(),
//				this->sv->canvasHeight(),
//				this->sv->currentZoom(),
//                v-this->sv->currentPixelDeltaX(),
//				0);
//		}
//	});
//	layout->addWidget(xBox,1,0);
//	xBox->show();

//	yBox = new QDoubleSpinBox();
//	yBox->setPrefix("y : ");
//	yBox->setSuffix("px");
//	yBox->setSingleStep(1);
//	yBox->setRange(std::numeric_limits <double>::lowest(), std::numeric_limits <double>::max());
//	yBox->setValue(this->sv->currentPixelDeltaY());
//	yBox->setFixedWidth(80);
//	connect(yBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [=](double v) {
//		if(yBox->hasFocus()) {
//			this->sv->signalRequestSliceImageUpdate(
//				this->sv->canvasWidth(),
//				this->sv->canvasHeight(),
//				this->sv->currentZoom(),
//				0,
//                v-this->sv->currentPixelDeltaY());
//		}
//	});
//	layout->addWidget(yBox,1,1);
//	yBox->show();

	this->setLayout(layout);
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
    //qreal zoom = (e->delta() > 0) ?  1.1f : 0.9f;
    qreal zoom = (e->delta() > 0) ?  1.1f : (1/1.1f);
    sv->zoomPlaneArea(zoom);
    sv->signalRequestSliceImageUpdate();
}

void ToolZoom::toolMousePressEvent(QMouseEvent *e) {
    dragStart = e->pos();
    dragStartValid = true;
}

void ToolZoom::toolMouseMoveEvent(QMouseEvent *e) {
    if(dragRedraw->isChecked()) {
        if(dragStartValid) {
            int deltaX = e->pos().x() - dragStart.x();
            int deltaY = e->pos().y() - dragStart.y();
            sv->moveArea(deltaX, -deltaY);
            sv->signalRequestSliceImageUpdate();
            dragStart = e->pos();
        }
    }
}

void ToolZoom::toolMouseReleaseEvent(QMouseEvent *e) {
    Q_UNUSED(e);
    dragStartValid = false;
}

void ToolZoom::sliceChanged(const voxie::data::Plane &oldPlane, const voxie::data::Plane &newPlane, bool equivalent) {
    Q_UNUSED(oldPlane);
    Q_UNUSED(newPlane);
    Q_UNUSED(equivalent);
}

void ToolZoom::sliceImageChanged(voxie::data::SliceImage &si) {
//    xBox->setValue(sv->currentPixelDeltaX());
//    yBox->setValue(sv->currentPixelDeltaY());
//    zoomBox->setValue(sv->currentZoom());
	Q_UNUSED(si);
}


// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
