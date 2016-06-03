#include "toolvalueviewer.hpp"

#include <PluginVisSlice/slicevisualizer.hpp>

#include <QtGui/QGuiApplication>
#include <QtGui/QPainter>

ValueViewerTool::ValueViewerTool(QWidget *parent, SliceVisualizer* sv) :
    Visualizer2DTool(parent),
    sv(sv),
    mousePressed(false)
{
    QGridLayout *layout = new QGridLayout(this);
    valueButton = new QPushButton(getIcon(), getName());
    valueButton->setCheckable(true);
    connect(valueButton, &QPushButton::clicked, [=]() {
        sv->switchToolTo(this);
    });
    //zoomButton->setUpdatesEnabled(false);
    layout->addWidget(valueButton,0,0);
    valueButton->show();
    this->setLayout(layout);
	persistentImg = QImage(this->sv->canvasSize(), QImage::Format_ARGB32);
	this->persistentImg.fill(qRgba(0,0,0,0));
}

void ValueViewerTool::activateTool() {
    //qDebug() << "activate value";
    valueButton->setChecked(true);
}

void ValueViewerTool::deactivateTool() {
    //qDebug() << "deactivate value";
	this->sv->addToDrawStack(this, this->persistentImg); // temporary alternative to emit sliceImageChanged
    this->sv->redraw();
    valueButton->setChecked(false);
    mousePosValid = false;
}

void ValueViewerTool::toolMousePressEvent(QMouseEvent *ev)
{
	if(ev->button() == Qt::LeftButton){
		this->mousePressed = true;
		this->startPos = ev->pos();
	} else if(ev->button() == Qt::RightButton) {
		this->persistentImg = QImage(this->sv->canvasSize(), QImage::Format_ARGB32);
		this->persistentImg.fill(qRgba(0,0,0,0));
		sv->addToDrawStack(this, this->persistentImg);
	    //sv->redraw();
        toolMouseMoveEvent(ev);
	}
}

void ValueViewerTool::sliceImageChanged(voxie::data::SliceImage& si) {
	Q_UNUSED(si);
	this->persistentImg = QImage(this->sv->canvasSize(), QImage::Format_ARGB32);
	this->persistentImg.fill(qRgba(0,0,0,0));
	sv->addToDrawStack(this, this->persistentImg);
    sv->redraw();

}

void ValueViewerTool::toolMouseReleaseEvent(QMouseEvent* ev)
{
	if(ev->button() == Qt::LeftButton){
		this->mousePressed = false;
		this->persistentImg = tempImg;
	}
}

void ValueViewerTool::toolMouseMoveEvent(QMouseEvent * ev)
{
    if(this->mousePressed && (ev->pos() - this->startPos).manhattanLength() > 2){
        mousePosValid = false;

        this->tempImg = persistentImg;

        // meassure
        QPoint endPos = ev->pos();
        float distance = this->sv->sliceImage().distanceInMeter(this->startPos, endPos);

        QPainter painter(&tempImg);
        painter.setRenderHint(QPainter::Antialiasing,true);
        QPen pen(QColor(0,0,0,96));
        pen.setWidth(2);
        painter.setPen(pen);
        painter.drawLine(this->startPos, endPos);
        pen.setColor(QColor(255,255,0));
        pen.setWidth(1);
        pen.setStyle(Qt::DashLine);
        painter.setPen(pen);
        painter.drawLine(this->startPos, endPos);

        QString txt = QString::number(distance);
        QPoint txtPos = this->startPos + (endPos - this->startPos)*0.5f;
        QRect txtRect = QRect(txtPos, QFontMetrics(QFont()).boundingRect(txt).size()+QSize(3,0));
        painter.fillRect(txtRect, QColor(0,0,0,96));
        pen.setStyle(Qt::SolidLine);
        painter.setPen(pen);
        painter.drawText(txtRect, Qt::TextSingleLine, txt);

        this->sv->addToDrawStack(this, this->tempImg); // temporary alternative to emit sliceImageChanged
        this->sv->redraw();
    } else {
        mousePos = ev->pos();
        mousePosValid = true;
        updateValueAtMouse(QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier));
    }
}

void ValueViewerTool::updateValueAtMouse(bool showFilteredImage) {
    if (!mousePosValid)
        return;

    this->tempImg = persistentImg;

    // view value
    voxie::data::SliceImage sliceImg = (showFilteredImage ? this->sv->filteredSliceImage() : this->sv->sliceImage());
    QPoint pos = mousePos;
    QString txt = "";
    auto planePoint = sliceImg.pixelToPlanePoint(pos, true);
    if(pos.x() >= 0 && ((size_t)pos.x()) < sliceImg.getWidth() && pos.y() >= 0 && ((size_t)pos.y()) < sliceImg.getHeight())
        txt += QString::number(sliceImg.getBufferCopy()[(sliceImg.getHeight()-1-pos.y())*sliceImg.getWidth() + pos.x()]);
    txt += " [";
    txt += QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier) ? QString::number(planePoint.toPoint().x()) : QString::number(planePoint.x());
    txt += " | ";
    txt += QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier) ? QString::number(planePoint.toPoint().y()) : QString::number(planePoint.y());
    txt += "]";
    QPainter painter(&tempImg);
    QPen pen;
    pen.setStyle( Qt::SolidLine );
    pen.setColor( QColor(0,255,255));
    painter.setPen(pen);
    QFont font; // applications default
    painter.setFont(font);
    QRect txtRect = QRect(mousePos+QPoint(5,-15), QFontMetrics(font).boundingRect(txt).size()+QSize(10,0));
    painter.fillRect(txtRect, QColor(0,0,0,96));
    painter.drawText(txtRect, Qt::TextSingleLine, txt);
    painter.drawPoint(mousePos);
    QPoint left = pos;
    QPoint right = pos;
    QPoint top = pos;
    QPoint bottom = pos;
    left.setX(left.x()-1);
    right.setX(right.x()+1);
    top.setY(top.y()-1);
    bottom.setY(bottom.y()+1);
    painter.drawLine(left, right);
    painter.drawLine(top, bottom);

    this->sv->addToDrawStack(this, this->tempImg); // temporary alternative to emit sliceImageChanged
    this->sv->redraw();
}

void ValueViewerTool::toolKeyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_Shift)
        updateValueAtMouse(true);
}


void ValueViewerTool::toolKeyReleaseEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_Shift)
        updateValueAtMouse(false);
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
