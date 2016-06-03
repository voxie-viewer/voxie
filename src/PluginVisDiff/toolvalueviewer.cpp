#include "toolvalueviewer.hpp"

#include <PluginVisDiff/diffvisualizer.hpp>

#include <QtGui/QGuiApplication>
#include <QtGui/QPainter>

ValueViewerTool::ValueViewerTool(QWidget *parent, DiffVisualizer* sv) :
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
    valueButton->setChecked(false);
    sv->addToDrawStack(this, QImage()); // temporary alternative to emit sliceImageChanged
    sv->redraw();
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
        sv->redraw();
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
    this->tempImg = persistentImg;
    if(this->mousePressed && (ev->pos() - this->startPos).manhattanLength() > 2){
        // meassure
        QPoint endPos = ev->pos();
        float distance_first = this->sv->sliceImageFirst().distanceInMeter(this->startPos, endPos);
        float distance_second = this->sv->sliceImageSecond().distanceInMeter(this->startPos, endPos);

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

        QString txt = QString::number(distance_first) + "|" + QString::number(distance_second);
        QPoint txtPos = this->startPos + (endPos - this->startPos)*0.5f;
        QRect txtRect = QRect(txtPos, QFontMetrics(QFont()).boundingRect(txt).size()+QSize(3,0));
        painter.fillRect(txtRect, QColor(0,0,0,96));
        pen.setStyle(Qt::SolidLine);
        painter.setPen(pen);
        painter.drawText(txtRect, txt);
    } else {
        // view value
        // shift down? -> use filterd image
        if(QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier))
        {
            voxie::data::SliceImage sliceImg = this->sv->filteredSliceImage();
            QPoint pos = ev->pos();
            QString txt = "";
            auto planePoint = sliceImg.pixelToPlanePoint(pos, true);
            if(pos.x() >= 0 && ((size_t)pos.x()) < sliceImg.getWidth() && pos.y() >= 0 && ((size_t)pos.y()) < sliceImg.getHeight())
                txt += QString::number(sliceImg.getBufferCopy()[pos.y()*sliceImg.getWidth() + pos.x()]);
            txt += " [";
            txt += QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier) ? QString::number(planePoint.toPoint().x()) : QString::number(planePoint.x());
            txt += " / ";
            txt += QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier) ? QString::number(planePoint.toPoint().y()) : QString::number(planePoint.y());
            txt += "]";
            QPainter painter(&tempImg);
            QPen pen;
            pen.setStyle( Qt::SolidLine );
            pen.setColor( QColor(0,255,255));
            painter.setPen(pen);
            QFont font; // applications default
            painter.setFont(font);
            QRect txtRect = QRect(ev->pos()+QPoint(5,-15), QFontMetrics(QFont()).boundingRect(txt).size()+QSize(3,0));
            painter.fillRect(txtRect, QColor(0,0,0,96));
            painter.drawText(txtRect, txt);
            painter.drawPoint(ev->pos());
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
        }
        // else -> use both unfiltered slice images
        else
        {
            voxie::data::SliceImage firstSliceImg = this->sv->sliceImageFirst();
            QPoint pos = ev->pos();
            QString txt = "";
            // first slice
            auto planePoint = firstSliceImg.pixelToPlanePoint(pos, true);
            if(pos.x() >= 0 && ((size_t)pos.x()) < firstSliceImg.getWidth() && pos.y() >= 0 && ((size_t)pos.y()) < firstSliceImg.getHeight())
                txt += QString::number(firstSliceImg.getBufferCopy()[pos.y()*firstSliceImg.getWidth() + pos.x()]);
            txt += " [";
            txt += QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier) ? QString::number(planePoint.toPoint().x()) : QString::number(planePoint.x());
            txt += " / ";
            txt += QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier) ? QString::number(planePoint.toPoint().y()) : QString::number(planePoint.y());
            txt += "]";
            //second slice
            voxie::data::SliceImage secondSliceImg = this->sv->sliceImageSecond();
            planePoint = secondSliceImg.pixelToPlanePoint(pos, true);
            if(pos.x() >= 0 && ((size_t)pos.x()) < secondSliceImg.getWidth() && pos.y() >= 0 && ((size_t)pos.y()) < secondSliceImg.getHeight())
                txt += "\n" + QString::number(secondSliceImg.getBufferCopy()[pos.y()*secondSliceImg.getWidth() + pos.x()]);
            txt += " [";
            txt += QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier) ? QString::number(planePoint.toPoint().x()) : QString::number(planePoint.x());
            txt += " / ";
            txt += QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier) ? QString::number(planePoint.toPoint().y()) : QString::number(planePoint.y());
            txt += "]";
            QPainter painter(&tempImg);
            QPen pen;
            pen.setStyle( Qt::SolidLine );
            pen.setColor( QColor(0,255,255));
            painter.setPen(pen);
            QFont font; // applications default
            painter.setFont(font);
            QRect txtRect = QRect(ev->pos()+QPoint(5,-15), QFontMetrics(QFont()).boundingRect(txt).size()+QSize(3,15));
            painter.drawText(txtRect, txt);
        }
    }
    sv->addToDrawStack(this, tempImg); // temporary alternative to emit sliceImageChanged
    sv->redraw();
}

void ValueViewerTool::toolKeyPressEvent(QKeyEvent *e)
{
    Q_UNUSED(e);
}


void ValueViewerTool::toolKeyReleaseEvent(QKeyEvent *e)
{
    Q_UNUSED(e);
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
