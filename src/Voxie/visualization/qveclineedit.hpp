#pragma once

#include <Voxie/voxiecore_global.hpp>

#include <QtGui/QQuaternion>
#include <QtGui/QVector3D>

#include <QtWidgets/QLineEdit>

namespace voxie {
namespace visualization {

/**
 * QLineEdit class specially designed for QVector3D and QQuaternion.
 * allows input of comma seperated vector components.
 */
class VOXIECORESHARED_EXPORT QVecLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit QVecLineEdit(QWidget *parent = 0) : QLineEdit(parent) {}

    void setVector(const QVector3D &vec);
    QVector3D getVector(bool* isValid = nullptr);

    void setQuaternion(const QQuaternion& quat);
    QQuaternion getQuaternion(bool* isValid = nullptr);


    static inline QString toString(const QVector3D &vector)
    {
        return QString::number(vector.x()) + ", " +
               QString::number(vector.y()) + ", " +
               QString::number(vector.z());
    }

    static inline QString toString(const QQuaternion &quat)
    {
        return QString::number(quat.scalar(), 'f') + ", " +
               QString::number(quat.x(), 'f') + ", " +
               QString::number(quat.y(), 'f') + ", " +
               QString::number(quat.z(), 'f');
    }

};


}}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
