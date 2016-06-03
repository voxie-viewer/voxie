#include "selection3dmask.hpp"

using namespace voxie::filter;

Selection3DMask::~Selection3DMask()
{
}

void
Selection3DMask::addShape(Shape3D *shape)
{
    if(shape != nullptr){
        if(shape->parent() == nullptr)
            shape->setParent(this);
        this->selectionShapes.append(shape);
        connect(shape, &Shape3D::changed, this, &Selection3DMask::shapeChanged);
        calcBoundingbox();
        emit this->changed();
    }
}

Shape3D*
Selection3DMask::getShape(int index) const
{
    if(index >= 0 && index < this->numShapes()){
        return this->selectionShapes.at(index);
    } else {
        return nullptr;
    }
}

void
Selection3DMask::removeShapeAt(int index)
{
    if(index >= 0 && index < this->numShapes()){
        Shape3D* shape = this->selectionShapes[index];
        this->selectionShapes.removeAt(index);
        disconnect(shape, &Shape3D::changed, this, &Selection3DMask::shapeChanged);
        if(shape->parent() == this)
            shape->deleteLater();

        calcBoundingbox();
        emit this->changed();
    }
}

bool
Selection3DMask::removeShape(Shape3D *shape)
{
    int index = this->selectionShapes.indexOf(shape);
    if(index < 0){
        return false;
    } else {
        this->removeShapeAt(index);
        return true;
    }
}

void
Selection3DMask::clear()
{
    if(!this->selectionShapes.isEmpty()){
        while(!this->selectionShapes.isEmpty()){
            Shape3D* shape = this->selectionShapes.takeLast();
            disconnect(shape, &Shape3D::changed, this, &Selection3DMask::shapeChanged);
            if(shape->parent() == this)
                shape->deleteLater();
        }
        this->selectionShapes.clear();
        calcBoundingbox();
        emit this->changed();
    }
}

void
Selection3DMask::calcBoundingbox()
{
    if(this->isEmpty()){
        boundingBox.setDimension(QVector3D(0,0,0));
        boundingBox.setOrigin(QVector3D(0,0,0));
    } else {
        Shape3D* s = this->selectionShapes[0];
        qreal minMax[6] = {-s->minX(), -s->minY(), -s->minZ(),   s->maxX(), s->maxY(), s->maxZ()};
        qreal minMaxTemp[6];
        for(int i = 1; i < this->selectionShapes.length(); i++){
            s = selectionShapes[i];
            minMaxTemp[0]=-s->minX(); minMaxTemp[1]=-s->minY(); minMaxTemp[2]=-s->minZ();
            minMaxTemp[3]=s->maxX();  minMaxTemp[4]=s->maxY();  minMaxTemp[5]=s->maxZ();
            for(int j = 0; j < 6; j++){
                minMax[j] = minMaxTemp[j] > minMax[j] ? minMaxTemp[j] : minMax[j];
            }
        }
        minMax[0] = - minMax[0]; minMax[1] = - minMax[1]; minMax[2] = - minMax[2];
        boundingBox.setOrigin(QVector3D(minMax[0],minMax[1],minMax[2]));
        boundingBox.setDimension(QVector3D(minMax[3]-minMax[0], minMax[4]-minMax[1], minMax[5]-minMax[2]));
    }
    //qDebug() << "new bounding box" << this->boundingBox.getOrigin() << boundingBox.getDimension();
}

void
Selection3DMask::shapeChanged()
{
    calcBoundingbox();
    emit this->changed();
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
