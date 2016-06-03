#include "qveclineedit.hpp"

using namespace voxie::visualization;

static QRegExp parseVector(R"reg((-?\d+(?:\.\d+)?(?:[eE][+-]?[0-9]+)?)\s*,\s*(-?\d+(?:\.\d+)?(?:[eE][+-]?[0-9]+)?)\s*,\s*(-?\d+(?:\.\d+)?(?:[eE][+-]?[0-9]+)?))reg");
static QRegExp parseQuaternion(R"reg((-?\d+(?:\.\d+)?(?:[eE][+-]?[0-9]+)?)\s*,\s*(-?\d+(?:\.\d+)?(?:[eE][+-]?[0-9]+)?)\s*,\s*(-?\d+(?:\.\d+)?(?:[eE][+-]?[0-9]+)?)\s*,\s*(-?\d+(?:\.\d+)?(?:[eE][+-]?[0-9]+)?))reg");

void QVecLineEdit::setVector(const QVector3D& vec)
{
    this->setText(QVecLineEdit::toString(vec));
}

QVector3D QVecLineEdit::getVector(bool *isValid)
{
    if(::parseVector.exactMatch(this->text()) == false)
    {
        if(isValid != nullptr)
            *isValid = false;
        return QVector3D();
    }
    if(isValid != nullptr)
        *isValid = true;
    qreal x, y, z;
    x = ::parseVector.cap(1).toFloat();
    y = ::parseVector.cap(2).toFloat();
    z = ::parseVector.cap(3).toFloat();

    return QVector3D(x,y,z);
}

void QVecLineEdit::setQuaternion(const QQuaternion &quat)
{
    this->setText(QVecLineEdit::toString(quat));
}

QQuaternion QVecLineEdit::getQuaternion(bool* isValid)
{
    if(::parseQuaternion.exactMatch(this->text()) == false)
    {
        if(isValid != nullptr)
            *isValid = false;
        return QQuaternion();
    }
    if(isValid != nullptr)
        *isValid = true;
    qreal x, y, z, w;
    w = ::parseQuaternion.cap(1).toFloat();
    x = ::parseQuaternion.cap(2).toFloat();
    y = ::parseQuaternion.cap(3).toFloat();
    z = ::parseQuaternion.cap(4).toFloat();

    return QQuaternion(w,x,y,z);
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
