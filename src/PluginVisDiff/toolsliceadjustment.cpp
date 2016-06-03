#include "toolsliceadjustment.hpp"

#include <PluginVisDiff/diffvisualizer.hpp>

#include <Voxie/data/voxeldata.hpp>

#include <QtCore/QtMath>

//fwd declaration
void rotateSlice(voxie::data::Slice* slice, const QVector3D& rotationAxis, float rotationAngle);
void moveSlice(voxie::data::Slice* slice, float steps);


SliceAdjustmentTool::SliceAdjustmentTool(QWidget *parent, DiffVisualizer *sv) : Visualizer2DTool(parent), sv(sv), shiftDown(false)
{
    this->slice = sv->slices().at(0);
    this->currentSlice = 0;

    QGridLayout *layout = new QGridLayout(this);
    adjustButton = new QPushButton(getIcon(), getName());
    adjustButton->setCheckable(true);
    connect(adjustButton, &QPushButton::clicked, [=]() {
        sv->switchToolTo(this);
    });
    layout->addWidget(adjustButton,0,0);
    adjustButton->show();
    this->setLayout(layout);

    switchSliceButton = new QPushButton(getIcon(), "Switch Slice");
    connect(switchSliceButton, &QPushButton::clicked, [=]() {
        this->switchSlice();
    });
    layout->addWidget(switchSliceButton,0,1);
    switchSliceButton->hide();

    this->selectedBoth = false;
    selectBothButton = new QPushButton(getIcon(), "Adjust all Slices");
    connect(selectBothButton, &QPushButton::clicked, [=]() {
        this->selectBothSlices();
    });
    layout->addWidget(selectBothButton,1,1);
    selectBothButton->hide();
}

void SliceAdjustmentTool::activateTool() {
    //qDebug() << "activate adjust";
    adjustButton->setChecked(true);
    this->isActive = true;
    switchSliceButton->show();
    selectBothButton->show();

    this->draw();
}

void SliceAdjustmentTool::deactivateTool() {
    //qDebug() << "deactivate adjust";
    adjustButton->setChecked(false);
    this->isActive = false;
    switchSliceButton->hide();
    selectBothButton->hide();

    sv->addToDrawStack(this, QImage()); // temporary alternative to emit sliceImageChanged
    sv->redraw();
}

void
SliceAdjustmentTool::toolWheelEvent(QWheelEvent * ev)
{
    qreal direction = ev->delta() > 0 ? 1:-1;
    moveSlice(this->slice, direction * (shiftDown ? this->fineAdjustmentFactor:1));
    if(this->selectedBoth)
        moveSlice((currentSlice == 0) ? this->sv->slices().at(0) : this->sv->slices().at(1), direction * (shiftDown ? this->fineAdjustmentFactor:1));
}

void
SliceAdjustmentTool::toolMousePressEvent(QMouseEvent * ev)
{
    voxie::data::SliceImage& currentImg = (currentSlice == 0) ? this->sv->sliceImageFirst() : this->sv->sliceImageSecond();
    voxie::data::SliceImage& otherImg   = (currentSlice == 1) ? this->sv->sliceImageFirst() : this->sv->sliceImageSecond();
    voxie::data::Slice* otherSlice = (currentSlice == 0) ? this->sv->slices().at(0) : this->sv->slices().at(1);
    if(ev->button() == Qt::LeftButton){
        if(this->ctrlDown){
            // rotation Adjustment
            this->rotatingInProgress = true;
            QVector2D cursorOnPlane = QVector2D(currentImg.pixelToPlanePoint(ev->pos(), true)).normalized();
            this->rotationHypotenuse = cursorOnPlane;
            if(this->selectedBoth)
            {
                QVector2D cursorOnOtherPlane = QVector2D(otherImg.pixelToPlanePoint(ev->pos(), true)).normalized();
                this->rotationHypotenuse2 = cursorOnOtherPlane;
            }
        } else {
            // tilt adjustment
            float tiltAngle = this->tiltAngle; // in degrees
            if(shiftDown)
                tiltAngle *= fineAdjustmentFactor;

            QPointF cursorOnPlane = currentImg.pixelToPlanePoint(ev->pos(), true);
            QVector3D cursorInVolume = this->slice->getCuttingPlane().get3DPoint(cursorOnPlane).normalized();
            QVector3D rotationAxis = QVector3D::crossProduct(cursorInVolume, this->slice->normal());

            rotateSlice(this->slice, rotationAxis, tiltAngle);

            if(this->selectedBoth)
            {
                QPointF cursorOnPlane = otherImg.pixelToPlanePoint(ev->pos(), true);
                QVector3D cursorInVolume = otherSlice->getCuttingPlane().get3DPoint(cursorOnPlane).normalized();
                QVector3D rotationAxis = QVector3D::crossProduct(cursorInVolume, otherSlice->normal());

                rotateSlice(otherSlice, rotationAxis, tiltAngle);
            }
        }
    } else if(ev->button() == Qt::RightButton){
        // set origin to cursor
        QPointF cursorOnPlane = currentImg.pixelToPlanePoint(ev->pos(), true);
        this->slice->setOrigin(cursorOnPlane);
        if(this->selectedBoth)
        {
            QPointF cursorOnPlane = otherImg.pixelToPlanePoint(ev->pos(), true);
            otherSlice->setOrigin(cursorOnPlane);
        }
    }
}

void //TODO: second plane
SliceAdjustmentTool::toolMouseMoveEvent(QMouseEvent *ev)
{
    voxie::data::SliceImage& currentImg = (currentSlice == 0) ? this->sv->sliceImageFirst() : this->sv->sliceImageSecond();
    voxie::data::SliceImage& otherImg   = (currentSlice == 1) ? this->sv->sliceImageFirst() : this->sv->sliceImageSecond();
    voxie::data::Slice* otherSlice = (currentSlice == 0) ? this->sv->slices().at(0) : this->sv->slices().at(1);

    if(this->dragUpdates){
        if(this->ctrlDown){
            if(this->rotatingInProgress){
                QVector2D cursorOnPlane = QVector2D(currentImg.pixelToPlanePoint(ev->pos(), true)).normalized();
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
                if(this->selectedBoth)
                {
                    QVector2D cursorOnPlane = QVector2D(otherImg.pixelToPlanePoint(ev->pos(), true)).normalized();
                    float angle = (float) qAcos(QVector3D::dotProduct(this->rotationHypotenuse2, cursorOnPlane));
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
                        qreal cos = this->rotationHypotenuse2.x();
                        qreal sin = this->rotationHypotenuse2.y();
                        qreal det = cos*cos + sin*sin;
                        // y-part of matrix multiplication (clockwise*cursor)
                        qreal rotCursorY = -sin/det * cursorOnPlane.x() + cos/det * cursorOnPlane.y();
                        sign = rotCursorY > 0 ? 1:-1;
                    }
                    rotateSlice(otherSlice, otherSlice->normal(), ((angle)/3.1415f) * 180 * sign);
                    this->rotationHypotenuse2 = cursorOnPlane;
                }
                rotateSlice(this->slice, this->slice->normal(), ((angle)/3.1415f) * 180 * sign);
                this->rotationHypotenuse = cursorOnPlane;
            }
        }
    }
}

// TODO: second plane
void SliceAdjustmentTool::toolMouseReleaseEvent(QMouseEvent *ev)
{
    voxie::data::SliceImage& currentImg = (currentSlice == 0) ? this->sv->sliceImageFirst() : this->sv->sliceImageSecond();
    voxie::data::SliceImage& otherImg   = (currentSlice == 1) ? this->sv->sliceImageFirst() : this->sv->sliceImageSecond();
    voxie::data::Slice* otherSlice = (currentSlice == 0) ? this->sv->slices().at(0) : this->sv->slices().at(1);

    if(this->ctrlDown){
        if(this->rotatingInProgress){
            this->rotatingInProgress = false;
            QVector2D cursorOnPlane = QVector2D(currentImg.pixelToPlanePoint(ev->pos(), true)).normalized();
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
            if(this->selectedBoth)
            {
                QVector2D cursorOnPlane = QVector2D(otherImg.pixelToPlanePoint(ev->pos(), true)).normalized();
                float angle = (float) qAcos(QVector3D::dotProduct(this->rotationHypotenuse2, cursorOnPlane));
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
                    qreal cos = this->rotationHypotenuse2.x();
                    qreal sin = this->rotationHypotenuse2.y();
                    qreal det = cos*cos + sin*sin;
                    // y-part of matrix multiplication (clockwise*cursor)
                    qreal rotCursorY = -sin/det * cursorOnPlane.x() + cos/det * cursorOnPlane.y();
                    sign = rotCursorY > 0 ? 1:-1;
                }
                rotateSlice(otherSlice, otherSlice->normal(), ((angle)/3.1415f) * 180 * sign);
            }
            rotateSlice(this->slice, this->slice->normal(), ((angle)/3.1415f) * 180 * sign);
        }
    }
}


void
SliceAdjustmentTool::toolKeyPressEvent(QKeyEvent *ev)
{
    voxie::data::Slice* otherSlice = (currentSlice == 0) ? this->sv->slices().at(1) : this->sv->slices().at(0);
    switch (ev->key()) {
    case Qt::Key_Shift:
        this->shiftDown = true;
        break;
    case Qt::Key_Control:
        this->ctrlDown = true;
        break;
        // arrow keys
    case Qt::Key_Up:
        rotateSlice(this->slice, this->slice->xAxis(), this->tiltAngle * (this->shiftDown ? this->fineAdjustmentFactor : 1) );
        if(this->selectedBoth) rotateSlice(otherSlice, otherSlice->xAxis(), this->tiltAngle * (this->shiftDown ? this->fineAdjustmentFactor : 1) );
        break;
    case Qt::Key_Down:
        rotateSlice(this->slice, -this->slice->xAxis(), this->tiltAngle * (this->shiftDown ? this->fineAdjustmentFactor : 1) );
        if(this->selectedBoth) rotateSlice(otherSlice, -otherSlice->xAxis(), this->tiltAngle * (this->shiftDown ? this->fineAdjustmentFactor : 1) );
        break;
    case Qt::Key_Right:
        rotateSlice(this->slice, this->slice->yAxis(), this->tiltAngle * (this->shiftDown ? this->fineAdjustmentFactor : 1) );
        if(this->selectedBoth) rotateSlice(otherSlice, otherSlice->yAxis(), this->tiltAngle * (this->shiftDown ? this->fineAdjustmentFactor : 1) );
        break;
    case Qt::Key_Left:
        rotateSlice(this->slice, -this->slice->yAxis(), this->tiltAngle * (this->shiftDown ? this->fineAdjustmentFactor : 1) );
        if(this->selectedBoth) rotateSlice(otherSlice, -otherSlice->yAxis(), this->tiltAngle * (this->shiftDown ? this->fineAdjustmentFactor : 1) );
        break;
    case Qt::Key_PageUp:
        moveSlice(this->slice, (shiftDown ? this->fineAdjustmentFactor : 1) );
        if(this->selectedBoth) moveSlice(otherSlice, (shiftDown ? this->fineAdjustmentFactor : 1) );
        break;
    case Qt::Key_PageDown:
        moveSlice(this->slice, (shiftDown ? -this->fineAdjustmentFactor : -1) );
        if(this->selectedBoth) moveSlice(otherSlice, (shiftDown ? -this->fineAdjustmentFactor : -1) );
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
    default:
        break;
    }
}

void SliceAdjustmentTool::switchSlice()
{
    if(selectedBoth)
        selectedBoth = false;

    if(this->currentSlice == 0)
    {
        //qDebug() << "switch to first slice" << endl;
        this->slice = sv->slices().at(1);
        this->currentSlice = 1;
    }
    else
    {
        //qDebug() << "switch to second slice" << endl;
        this->slice = sv->slices().at(0);
        this->currentSlice = 0;
    }
    // swap rotationHyotenuse values
    QVector2D tmp = this->rotationHypotenuse;
    this->rotationHypotenuse = this->rotationHypotenuse2;
    this->rotationHypotenuse2 = tmp;

    this->draw();
}

void SliceAdjustmentTool::selectBothSlices()
{
    selectedBoth = !selectedBoth;
}

void //TODO:
SliceAdjustmentTool::draw()
{
    if(!this->isActive){
        return;
    }
    QRectF currentPlaneArea = (currentSlice == 0) ? this->sv->currentPlaneAreaFirstSlice() : this->sv->currentPlaneAreaSecondSlice();
    QRectF otherPlaneArea = (currentSlice == 1) ? this->sv->currentPlaneAreaFirstSlice() : this->sv->currentPlaneAreaSecondSlice();

    QImage img(this->sv->canvasSize(), QImage::Format_ARGB32);
    img.fill(qRgba(0,0,0,0));
    QPainter painter(&img);

    // First Slice
    qreal origX = -currentPlaneArea.left();
    qreal origY = -currentPlaneArea.top();
    // normalize x & y
    origX /= currentPlaneArea.width();
    origY /= currentPlaneArea.height();
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

    // Second Slice
    origX = -otherPlaneArea.left();
    origY = -otherPlaneArea.top();
    // normalize x & y
    origX /= otherPlaneArea.width();
    origY /= otherPlaneArea.height();
    // stretch x & y to canvas
    origX *= this->sv->canvasWidth();
    origY *= this->sv->canvasHeight();

    // draw x axis
    pen.setStyle(Qt::DashLine);
    pen.setColor(QColor(0x00,0xff,0x00));
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
