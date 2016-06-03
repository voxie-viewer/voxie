#include "toolselection.hpp"

#include <PluginVisDiff/diffvisualizer.hpp>

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QMessageBox>

ToolSelection::ToolSelection(QWidget *parent, DiffVisualizer* sv):
    Visualizer2DTool(parent),
    sv(sv)
{
    QGridLayout* layout = new QGridLayout(this);
    layout->setSpacing(2);
    rectangleButton = new QPushButton(QIcon(":/icons/layer-shape.png"), "Rectangle");
    rectangleButton->setCheckable(true);
    connect(rectangleButton, &QPushButton::clicked, [=]() {
        rectangleActive = true;
        ellipseActive = false;
        this->deleteButton->setChecked(false);
        polygonActive = false;
        this->sv->switchToolTo(this);
    });
    layout->addWidget(rectangleButton,0, 0);
    rectangleButton->hide();//show();

    ellipseButton = new QPushButton(QIcon(":/icons/layer-shape-ellipse.png"), "Ellipse");
    ellipseButton->setCheckable(true);
    connect(ellipseButton, &QPushButton::clicked, [=]() {
        ellipseActive = true;
        rectangleActive = false;
        polygonActive = false;
      //  this->deleteActive = false;
        this->deleteButton->setChecked(false);
        this->sv->switchToolTo(this);
    });
    layout->addWidget(ellipseButton,0, 1);
    ellipseButton->hide();//show();

    polygonButton = new QPushButton(QIcon(":/icons/layer-shape-polygon.png"), "Polygon");
    polygonButton->setCheckable(true);
    connect(polygonButton, &QPushButton::clicked, [=]() {
        this->polygonActive = true;
        this->ellipseActive = false;
        this->rectangleActive = false;
        this->deleteButton->setChecked(false);
        this->deleteActive = false;
        this->sv->switchToolTo(this);
    });
    layout->addWidget(polygonButton,1, 0);
    polygonButton->hide();//show();

    deleteButton = new QPushButton(QIcon(":/icons/layer-shade.png"), "Clear");
    deleteButton->setCheckable(true);
    connect(deleteButton, &QPushButton::clicked, [=]() {
        this->polygonActive = false;
        this->ellipseActive = false;
        this->rectangleActive = false;
       // this->deleteActive = true;
        if(mask != nullptr){
            mask->clearMask();
            this->polygonButton->setChecked(false);
            this->rectangleButton->setChecked(false);
            this->ellipseButton->setChecked(false);
            this->draw();
        }
        this->sv->switchToolTo(this);
    });
    layout->addWidget(deleteButton,1, 1);
    deleteButton->hide();//show();


    this->setLayout(layout);
}

QString ToolSelection::getName()
{
    return "Selection 2D";
}

QIcon ToolSelection::getIcon()
{
    return QIcon(":/icons/ruler--pencil.png");
}

void ToolSelection::activateTool()
{
    if(mask) {
        toolActive = true;
        rectangleButton->show();
        ellipseButton->show();
        polygonButton->show();
        deleteButton->show();
        //
        if (rectangleActive)
        {
            rectangleButton->setChecked(true);
        } else if(ellipseActive)
        {
            ellipseButton->setChecked(true);
        } else if (polygonActive)
        {
            polygonButton->setChecked(true);
        }
        this->draw();
    } else {
        QMessageBox messageBox;
        messageBox.critical(0,"Error","Can't activate mask tool without enabling it via FilterChain first.");
        messageBox.setFixedSize(500,200);
    }
}

void ToolSelection::deactivateTool()
{
    toolActive = false;
    rectangleButton->setChecked(false);
    ellipseButton->setChecked(false);
    polygonButton->setChecked(false);
    deleteButton->setChecked(false);
    firstValue = true;
    //
    rectangleButton->hide();
    ellipseButton->hide();
    polygonButton->hide();
    deleteButton->hide();

    sv->addToDrawStack(this, QImage()); // temporary alternative to emit sliceImageChanged
    sv->redraw();
}


void ToolSelection::setMask(Selection2DMask *mask)
{
    this->mask = mask;
    this->draw();
}

void ToolSelection::toolMousePressEvent(QMouseEvent *e)
{
    if(mask != nullptr) {
        voxie::data::SliceImage sliceImg = voxie::data::SliceImage(sv->floatImage(), false);
        QPoint tempPos = e->pos();
        start = e->pos();
        if (rectangleActive)
        {
            //qDebug() << "bla";
            //if (firstValue)
            //{
            sliceImg.imagePoint2PlanePoint(tempPos.x(), tempPos.y(), QSize(sv->canvasWidth(), sv->canvasHeight()), this->planeArea(), startRect, true);
            // firstValue = false;
            previewActive = true;
            mousePressed = true;
            /*} else {
            qDebug() << startRect;
            qDebug()<< tempPos;
            QPointF endRect;
            sliceImg.imagePoint2PlanePoint(tempPos.x(), tempPos.y(), QSize(sv->canvasWidth(), sv->canvasHeight()), this->planeArea(), endRect);
            mask->addRectangle(startRect.x(), startRect.y(), endRect.x(), endRect.y());
            qDebug() << endRect;
            firstValue = true;
            this->draw();
        }*/
        }

        if (polygonActive)
        {
            this->previewActive = true;
            this->previewPolygon.append(e->pos());
            QPointF polyPoint;
            sliceImg.imagePoint2PlanePoint(tempPos.x(), tempPos.y(), QSize(sv->canvasWidth(), sv->canvasHeight()), this->planeArea(), polyPoint, true);
            this->polygon.append(polyPoint);
            QPainterPath temp;
            temp.addPolygon(QPolygonF(this->previewPolygon));
            this->preview = temp;
            this->draw();
        }

        if(ellipseActive)
        {
            //if (firstValue)
            //{
            sliceImg.imagePoint2PlanePoint(tempPos.x(), tempPos.y(), QSize(sv->canvasWidth(), sv->canvasHeight()), this->planeArea(), middlePointEllipse, true);
            //  firstValue = false;
            previewActive = true;
            mousePressed = true;
            /*} else {
            QPointF radius;
            sliceImg.imagePoint2PlanePoint(tempPos.x(), tempPos.y(), QSize(sv->canvasWidth(), sv->canvasHeight()), this->planeArea(), radius);
            this->mask->addEllipse(this->middlePointEllipse.x(), this->middlePointEllipse.y(), fabs(this->middlePointEllipse.x() - radius.x()), fabs(this->middlePointEllipse.x() - radius.y()));
            firstValue = true;
            this->draw();
        }*/
        }
    }
}

void ToolSelection::toolMouseMoveEvent(QMouseEvent *e)
{
    if (mousePressed)
    {
        if(rectangleActive)
        {
            QPoint tempEnd = e->pos();
            QPoint tempStart = this->start;

            if (start.x() < tempEnd.x() && start.y() > tempEnd.y())
            {
                //startX = endX - fabs(endX - startX);
                tempStart.setY(tempEnd.y());
            }
            //falls startpunkt oben rechts und endpunkt unten links
            else if(start.x() > tempEnd.x() && start.y() < tempEnd.y())
            {
                tempStart.setX(tempEnd.x());
                //startY = startY - fabs(startY - endY);
            }
            //falls startpunkt unten rechts und endpunkt oben links
            else if(start.x() > tempEnd.x() && start.y() > tempEnd.y())
            {
                //int temp = tempStart.x();
                tempStart.setX(tempEnd.x());
                //tempEnd.setX(temp);
                //temp  = tempStart.y();
                tempStart.setY(tempEnd.y());
                //tempEnd.setY(temp);
            }
            QPainterPath temp;
            temp.addRect(QRectF((qreal)tempStart.x(), (qreal)tempStart.y(), fabs((qreal)tempEnd.x() - (qreal)start.x()), fabs((qreal)tempEnd.y() - (qreal)start.y())));
            preview = temp;
            this->draw();

        }
        if (ellipseActive)
        {
            QPainterPath temp;
            temp.addEllipse(QPointF((qreal)start.x(), (qreal)start.y()), fabs((qreal)e->pos().x() - (qreal)start.x()), fabs((qreal)e->pos().y() - (qreal)start.y()));
            preview = temp;
            this->draw();
        }
    }
}

void ToolSelection::toolMouseReleaseEvent(QMouseEvent *e)
{
    if(polygonActive)
    {
        return;
    }
    mousePressed = false;
    previewActive = false;

    voxie::data::SliceImage sliceImg =voxie::data::SliceImage(sv->floatImage(), false);
    QPointF tempEnd;
    sliceImg.imagePoint2PlanePoint(e->pos().x(), e->pos().y(), QSize(sv->canvasWidth(), sv->canvasHeight()), this->planeArea(), tempEnd, true);
    if (rectangleActive)
    {
        mask->addRectangle(startRect.x(), startRect.y(), tempEnd.x(), tempEnd.y());
    }

    if (ellipseActive)
    {
        mask->addEllipse(this->middlePointEllipse.x(), this->middlePointEllipse.y(), fabs(tempEnd.x() - this->middlePointEllipse.x()), fabs(tempEnd.y() - this->middlePointEllipse.y()));
    }

    this->draw();
}

void ToolSelection::toolKeyPressEvent(QKeyEvent *e)
{
    if (polygonActive && mask != nullptr && polygon.size() > 2)
    {
        if (e->key()==Qt::Key_Space)
        {
            polygon.append(polygon.at(0));
            mask->addPolygon(this->polygon);
            polygon.clear();
            this->previewPolygon.clear();
            this->previewActive = false;
            this->draw();
        }
    }
}


void ToolSelection::draw()
{
    if(!toolActive){
        return;
    }

    QImage img(this->sv->canvasSize(), QImage::Format_ARGB32);
    img.fill(qRgba(0,0,0,0));
    QPainter painter(&img);

    painter.setPen(QColor(255,0,0));
    QBrush brush;
    brush.setColor(QColor(122, 163, 39) );


    //painter.fillPath(this->planeToImage(mask->getPath()), QColor(122, 163, 39));
    painter.drawPath(this->planeToImage(mask->getPath()));
    if(previewActive)
    {
        painter.setPen(QColor(255, 255, 0));
        painter.drawPath(this->preview);
    }

    sv->addToDrawStack(this, img); // temporary alternative to emit sliceImageChanged
    sv->redraw();
}

QPainterPath ToolSelection::planeToImage(QPainterPath path)
{
    QPainterPath returnPath;
    //qreal x = ((path.boundingRect().left() - this->planeArea().left())/(this->planeArea().width())) * sv->canvasWidth();
    //qreal y = ((path.boundingRect().top() - this->planeArea().top())/(this->planeArea().height())) * sv->canvasHeight();

    //qDebug() << path.boundingRect().left() << path.boundingRect().top();
    //qDebug() << x << y;
    QTransform translate(1, 0, 0, 1, -this->planeArea().left(), -this->planeArea().top());
    qreal scaleX = sv->canvasWidth() / this->planeArea().width();
    qreal scaleY = sv->canvasHeight() / this->planeArea().height();
    QTransform scale(scaleX, 0, 0, scaleY, 0, 0);

    returnPath = translate.map(path);
    returnPath = scale.map(returnPath);

    return returnPath;
}


QRectF ToolSelection::planeArea()
{
    return QRectF(QPointF(0,0), this->sv->canvasSize());
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
