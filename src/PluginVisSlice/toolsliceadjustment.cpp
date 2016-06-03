#include "toolsliceadjustment.hpp"

#include <PluginVisSlice/slicevisualizer.hpp>

#include <Voxie/data/voxeldata.hpp>

#include <QtCore/QtMath>

//fwd declaration
void rotateSlice(voxie::data::Slice* slice, const QVector3D& rotationAxis, float rotationAngle);
void moveSlice(voxie::data::Slice* slice, float steps);


SliceAdjustmentTool::SliceAdjustmentTool(QWidget *parent, SliceVisualizer *sv) : Visualizer2DTool(parent), sv(sv), shiftDown(false)
{
    this->slice = sv->slice();
    QGridLayout *layout = new QGridLayout(this);
    adjustButton = new QPushButton(getIcon(), getName());
    adjustButton->setCheckable(true);
    connect(adjustButton, &QPushButton::clicked, [=]() {
        sv->switchToolTo(this);
    });
    //zoomButton->setUpdatesEnabled(false);
    layout->addWidget(adjustButton,0,0);
    adjustButton->show();
    this->setLayout(layout);
}

void SliceAdjustmentTool::activateTool() {
    //qDebug() << "activate adjust";
    this->isActive = true;
    adjustButton->setChecked(true);

    this->draw();
}

void SliceAdjustmentTool::deactivateTool() {
    //qDebug() << "deactivate adjust";
    this->isActive = false;
    adjustButton->setChecked(false);

    sv->addToDrawStack(this, QImage()); // temporary alternative to emit sliceImageChanged
    sv->redraw();
}

void
SliceAdjustmentTool::toolWheelEvent(QWheelEvent * ev)
{
    qreal direction = ev->delta() > 0 ? 1:-1;
    moveSlice(this->slice, direction * (shiftDown ? this->fineAdjustmentFactor:1));
}

void
SliceAdjustmentTool::toolMousePressEvent(QMouseEvent * ev)
{
    if(ev->button() == Qt::LeftButton){
        if(this->ctrlDown){
            // rotation Adjustment
            this->rotatingInProgress = true;
            QVector2D cursorOnPlane = QVector2D(this->sv->sliceImage().pixelToPlanePoint(ev->pos(), true)).normalized();
            this->rotationHypotenuse = cursorOnPlane;
        } else {
            // tilt adjustment
            float tiltAngle = this->tiltAngle; // in degrees
            if(shiftDown)
                tiltAngle *= fineAdjustmentFactor;

            QPointF cursorOnPlane = this->sv->sliceImage().pixelToPlanePoint(ev->pos(), true);
            QVector3D cursorInVolume = this->slice->getCuttingPlane().get3DPoint(cursorOnPlane).normalized();
            QVector3D rotationAxis = QVector3D::crossProduct(cursorInVolume, this->slice->normal());

            rotateSlice(this->slice, rotationAxis, tiltAngle);
        }
    } else if(ev->button() == Qt::RightButton){
        // set origin to cursor
        QPointF cursorOnPlane = this->sv->sliceImage().pixelToPlanePoint(ev->pos(), true);
        this->slice->setOrigin(cursorOnPlane);
    }
}

void
SliceAdjustmentTool::toolMouseMoveEvent(QMouseEvent *ev)
{
    if(this->dragUpdates){
        if(this->ctrlDown){
            if(this->rotatingInProgress){
                QVector2D cursorOnPlane = QVector2D(this->sv->sliceImage().pixelToPlanePoint(ev->pos(), true)).normalized();
                float angle = (float) qAcos(QVector3D::dotProduct(this->rotationHypotenuse, cursorOnPlane));
                // check clockwise or counterclockwise rotation
                float sign = 1;
                {
                    /* to know if cursor is left of hypotenuse rotate their coordinatesystem so that
                     * hypotenuse points in xAxis direction and check if cursor.y is positive.
                     * Rotation matrix for this can be obtained from hypotenuse since its normalized
                     *      counterclockwise                clockwise
                     *      cos(a)  -sin(a)                 cos(a)/det   sin(a)/det
                     *      sin(a)   cos(a)                -sin(a)/det   cos(a)/det
                     *
                     * with cos(a) = hypotenuse.x , sin(a) = hypotenuse.y , det = determinant(clockwise rotMat)
                     */
                    qreal cos = this->rotationHypotenuse.x();
                    qreal sin = this->rotationHypotenuse.y();
                    qreal det = cos*cos + sin*sin;
                    // y-part of matrix multiplication (clockwise*cursor)
                    qreal rotCursorY = -sin/det * cursorOnPlane.x() + cos/det * cursorOnPlane.y();
                    sign = rotCursorY > 0 ? 1:-1;
                }
                QVector3D rotationAxis = this->slice->normal();
                if(xDown)
                    rotationAxis = this->slice->xAxis();
                if(yDown)
                    rotationAxis = this->slice->yAxis();
                rotateSlice(this->slice, rotationAxis, ((angle)/3.1415f) * 180 * sign);
                this->rotationHypotenuse = cursorOnPlane;
            }
        }
    }
}

void SliceAdjustmentTool::toolMouseReleaseEvent(QMouseEvent *ev)
{
    if(this->ctrlDown){
        if(this->rotatingInProgress){
            this->rotatingInProgress = false;
            QVector2D cursorOnPlane = QVector2D(this->sv->sliceImage().pixelToPlanePoint(ev->pos(), true)).normalized();
            float angle = (float) qAcos(QVector3D::dotProduct(this->rotationHypotenuse, cursorOnPlane));
            // check clockwise or counterclockwise rotation
            float sign = 1;
            {
                /* to know if cursor is left of hypotenuse rotate their coordinatesystem so that
                 * hypotenuse is points in xAxis direction and check if cursor.y is positive.
                 * Rotation matrix for this can be obtained from hypotenuse since its normalized
                 *      counterclockwise                clockwise
                 *      cos(a)  -sin(a)                 cos(a)/det   sin(a)/det
                 *      sin(a)   cos(a)                -sin(a)/det   cos(a)/det
                 *
                 * with cos(a) = hypotenuse.x , sin(a) = hypotenuse.y , det = determinant(clockwise rotMat)
                 */
                qreal cos = this->rotationHypotenuse.x();
                qreal sin = this->rotationHypotenuse.y();
                qreal det = cos*cos + sin*sin;
                // y-part of matrix multiplication (clockwise*cursor)
                qreal rotCursorY = -sin/det * cursorOnPlane.x() + cos/det * cursorOnPlane.y();
                sign = rotCursorY > 0 ? 1:-1;
            }
            QVector3D rotationAxis = this->slice->normal();
            if(xDown)
                rotationAxis = this->slice->xAxis();
            if(yDown)
                rotationAxis = this->slice->yAxis();
            rotateSlice(this->slice, rotationAxis, ((angle)/3.1415f) * 180 * sign);
        }
    }
}

void
SliceAdjustmentTool::toolKeyPressEvent(QKeyEvent *ev)
{
    switch (ev->key()) {
    case Qt::Key_Shift:
        this->shiftDown = true;
        break;
    case Qt::Key_Control:
        this->ctrlDown = true;
        break;
    case Qt::Key_X:
        this->yDown = false;
        this->xDown = true;
        break;
    case Qt::Key_Y:
        this->xDown = false;
        this->yDown = true;
        break;
    // arrow keys
    case Qt::Key_Up:
        rotateSlice(this->slice, this->slice->xAxis(), this->tiltAngle * (this->shiftDown ? this->fineAdjustmentFactor : 1) );
        break;
    case Qt::Key_Down:
        rotateSlice(this->slice, -this->slice->xAxis(), this->tiltAngle * (this->shiftDown ? this->fineAdjustmentFactor : 1) );
        break;
    case Qt::Key_Right:
        rotateSlice(this->slice, this->slice->yAxis(), this->tiltAngle * (this->shiftDown ? this->fineAdjustmentFactor : 1) );
        break;
    case Qt::Key_Left:
        rotateSlice(this->slice, -this->slice->yAxis(), this->tiltAngle * (this->shiftDown ? this->fineAdjustmentFactor : 1) );
        break;
    case Qt::Key_PageUp:
        moveSlice(this->slice, (shiftDown ? this->fineAdjustmentFactor : 1) );
        break;
    case Qt::Key_PageDown:
        moveSlice(this->slice, (shiftDown ? -this->fineAdjustmentFactor : -1) );
        break;
    default:
        break;
    }
}

void
SliceAdjustmentTool::toolKeyReleaseEvent(QKeyEvent *ev)
{
    switch (ev->key()) {
    case Qt::Key_Shift:
        this->shiftDown = false;
        break;
    case Qt::Key_Control:
        this->ctrlDown = false;
        this->rotatingInProgress = false;
        break;
    case Qt::Key_X:
        this->xDown = false;
        break;
    case Qt::Key_Y:
        this->yDown = false;
        break;
    default:
        break;
    }
}


void
SliceAdjustmentTool::draw()
{
    if(!this->isActive){
        return;
    }
    QImage img(this->sv->canvasSize(), QImage::Format_ARGB32);
    img.fill(qRgba(0,0,0,0));
    QPainter painter(&img);

    QRectF area = this->sv->currentPlaneArea();
    qreal origX = -area.left();
    //qreal origY = -area.top();
    qreal origY = area.bottom();
    // normalize x & y
    origX /= area.width();
    origY /= area.height();
    // stretch x & y to canvas
    origX *= this->sv->canvasWidth();
    origY *= this->sv->canvasHeight();

    // draw x axis
    QPen pen(QColor(0x00,0xff,0x00));
    painter.setPen(pen);
    painter.drawLine(0,(int)origY, this->sv->canvasWidth(), (int)origY);
    // draw y axis
    pen.setColor(QColor(0x00,0x00,0xff));
    painter.setPen(pen);
    painter.drawLine((int)origX, 0, (int)origX, this->sv->canvasHeight());

    sv->addToDrawStack(this, img); // temporary alternative to emit sliceImageChanged
    sv->redraw();
}


void rotateSlice(voxie::data::Slice* slice, const QVector3D& rotationAxis, float rotationAngle)
{
    if(isnan(rotationAngle)){
        return;
    }
    QQuaternion currentRotation = slice->rotation();
    QQuaternion rotationAdjustment = QQuaternion::fromAxisAndAngle(rotationAxis, rotationAngle);
    slice->setRotation(rotationAdjustment * currentRotation);
}

void moveSlice(voxie::data::Slice* slice, float steps){
    qreal volumeDiagonal = slice->getDataset()->size().length();
    qreal resolution = slice->getDataset()->filteredData()->getDimensions().toQVector3D().length();
    slice->translateAlongNormal(steps*(volumeDiagonal/resolution));
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
