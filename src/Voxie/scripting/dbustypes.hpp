#pragma once

#include <QtCore/QVector>

#include <QtDBus/QDBusArgument>
#include <QtDBus/QDBusObjectPath>

#include <QtGui/QQuaternion>
#include <QtGui/QVector2D>
#include <QtGui/QVector3D>

#if defined(VOXIECORE_LIBRARY)
#include <Voxie/voxiecore_global.hpp>
#endif

Q_DECLARE_METATYPE(QVector<QDBusObjectPath>)

typedef QMap<QDBusObjectPath, quint64> QMap_QDBusObjectPath_quint64;
Q_DECLARE_METATYPE(QMap_QDBusObjectPath_quint64)

namespace voxie {
namespace scripting {
struct IntVector3 {
    size_t x, y, z;

    IntVector3() {}

    IntVector3(size_t x, size_t y, size_t z) : x(x), y(y), z(z) {}

    QVector3D toQVector3D() const {
        return QVector3D(x, y, z);
    }
};

struct IntVector2 {
public:
    size_t x, y;

    IntVector2() {}

    IntVector2(size_t x, size_t y) : x(x), y(y) {}

    QVector2D toQVector2D() const {
        return QVector2D(x, y);
    }
};

struct Array2Info {
    Array2Info () : 
        handle (), offset (0), dataType (""), dataTypeSize (0), byteorder (""),
        sizeX (0), sizeY (0), strideX (0), strideY (0),
        metadata ()
    {}
    QMap<QString, QDBusVariant> handle;
    qint64 offset;

    QString dataType;
    quint32 dataTypeSize;
    QString byteorder;

    quint64 sizeX, sizeY;
    qint64 strideX, strideY;

    QMap<QString, QDBusVariant> metadata;
};

struct Array3Info {
    Array3Info () : 
        handle (), offset (0), dataType (""), dataTypeSize (0), byteorder (""),
        sizeX (0), sizeY (0), sizeZ (0), strideX (0), strideY (0), strideZ (0),
        metadata ()
    {}
    QMap<QString, QDBusVariant> handle;
    qint64 offset;

    QString dataType;
    quint32 dataTypeSize;
    QString byteorder;

    quint64 sizeX, sizeY, sizeZ;
    qint64 strideX, strideY, strideZ;

    QMap<QString, QDBusVariant> metadata;
};

struct Plane {
    /**
     * @brief origin of the planes coordinatesystem
     */
    QVector3D origin;
    /**
     * @brief rotation of the plane
     */
    QQuaternion rotation;
};

#if defined(VOXIECORE_LIBRARY)
VOXIECORESHARED_EXPORT
#endif
void initDBusTypes();

}
}

QDBusArgument &operator<<(QDBusArgument &argument, const QVector2D &value);
const QDBusArgument &operator>>(const QDBusArgument &argument, QVector2D &value);
QDBusArgument &operator<<(QDBusArgument &argument, const QVector3D &value);
const QDBusArgument &operator>>(const QDBusArgument &argument, QVector3D &value);
QDBusArgument &operator<<(QDBusArgument &argument, const QQuaternion &value);
const QDBusArgument &operator>>(const QDBusArgument &argument, QQuaternion &value);
QDBusArgument &operator<<(QDBusArgument &argument, const voxie::scripting::IntVector2 &value);
const QDBusArgument &operator>>(const QDBusArgument &argument, voxie::scripting::IntVector2 &value);
QDBusArgument &operator<<(QDBusArgument &argument, const voxie::scripting::IntVector3 &value);
const QDBusArgument &operator>>(const QDBusArgument &argument, voxie::scripting::IntVector3 &value);
QDBusArgument &operator<<(QDBusArgument &argument, const voxie::scripting::Array2Info &value);
const QDBusArgument &operator>>(const QDBusArgument &argument, voxie::scripting::Array2Info &value);
QDBusArgument &operator<<(QDBusArgument &argument, const voxie::scripting::Array3Info &value);
const QDBusArgument &operator>>(const QDBusArgument &argument, voxie::scripting::Array3Info &value);
QDBusArgument &operator<<(QDBusArgument &argument, const voxie::scripting::Plane &value);
const QDBusArgument &operator>>(const QDBusArgument &argument, voxie::scripting::Plane &value);

Q_DECLARE_METATYPE(voxie::scripting::IntVector2)
Q_DECLARE_METATYPE(voxie::scripting::IntVector3)

Q_DECLARE_METATYPE(voxie::scripting::Array2Info)
Q_DECLARE_METATYPE(voxie::scripting::Array3Info)

Q_DECLARE_METATYPE(voxie::scripting::Plane)

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
