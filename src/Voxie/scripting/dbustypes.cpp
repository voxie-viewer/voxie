#include "dbustypes.hpp"

#include <QtDBus/QDBusMetaType>

QDBusArgument &operator<<(QDBusArgument &argument, const QVector2D &value) {
  argument.beginStructure();
  argument << ((double) value.x ()) << ((double) value.y ());
  argument.endStructure();
  return argument;
}
const QDBusArgument &operator>>(const QDBusArgument &argument, QVector2D &value) {
  argument.beginStructure();
  double x, y;
  argument >> x >> y;
  argument.endStructure();
  value = QVector2D (x, y);
  return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument, const QVector3D &value) {
  argument.beginStructure();
  argument << ((double) value.x ()) << ((double) value.y ()) << ((double) value.z ());
  argument.endStructure();
  return argument;
}
const QDBusArgument &operator>>(const QDBusArgument &argument, QVector3D &value) {
  argument.beginStructure();
  double x, y, z;
  argument >> x >> y >> z;
  argument.endStructure();
  value = QVector3D (x, y, z);
  return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument, const QQuaternion &value) {
  argument.beginStructure();
  argument << value.scalar () << value.x () << value.y () << value.z ();
  argument.endStructure();
  return argument;
}
const QDBusArgument &operator>>(const QDBusArgument &argument, QQuaternion &value) {
  argument.beginStructure();
  double s, x, y, z;
  argument >> s >> x >> y >> z;
  argument.endStructure();
  value = QQuaternion (s, x, y, z);
  return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument, const voxie::scripting::IntVector2 &value) {
  argument.beginStructure();
  argument << ((quint64) value.x) << ((quint64) value.y);
  argument.endStructure();
  return argument;
}
const QDBusArgument &operator>>(const QDBusArgument &argument, voxie::scripting::IntVector2 &value) {
  argument.beginStructure();
  quint64 x, y;
  argument >> x >> y;
  argument.endStructure();
  value = voxie::scripting::IntVector2 (x, y);
  return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument, const voxie::scripting::IntVector3 &value) {
  argument.beginStructure();
  argument << ((quint64) value.x) << ((quint64) value.y) << ((quint64) value.z);
  argument.endStructure();
  return argument;
}
const QDBusArgument &operator>>(const QDBusArgument &argument, voxie::scripting::IntVector3 &value) {
  argument.beginStructure();
  quint64 x, y, z;
  argument >> x >> y >> z;
  argument.endStructure();
  value = voxie::scripting::IntVector3 (x, y, z);
  return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument, const voxie::scripting::Array2Info &value) {
  argument.beginStructure();
  argument << value.handle;
  argument << (qint64) value.offset;
  argument.beginStructure();
  argument << value.dataType;
  argument << value.dataTypeSize;
  argument << value.byteorder;
  argument.endStructure();
  argument.beginStructure();
  argument << (quint64) value.sizeX << (quint64) value.sizeY;
  argument.endStructure();
  argument.beginStructure();
  argument << (qint64) value.strideX << (qint64) value.strideY;
  argument.endStructure();
  argument << value.metadata;
  argument.endStructure();
  return argument;
}
const QDBusArgument &operator>>(const QDBusArgument &argument, voxie::scripting::Array2Info &value) {
  argument.beginStructure();

  argument >> value.handle;
  qint64 offset;
  argument >> offset;
  value.offset = offset;

  argument.beginStructure();
  argument >> value.dataType;
  quint32 dataTypeSize;
  argument >> dataTypeSize;
  value.dataTypeSize = dataTypeSize;
  argument >> value.byteorder;
  argument.endStructure();

  {
    argument.beginStructure();
    quint64 x, y;
    argument >> x >> y;
    value.sizeX = x;
    value.sizeY = y;
    argument.endStructure();
  }

  {
    argument.beginStructure(); 
    qint64 x, y;
    argument >> x >> y;
    value.strideX = x;
    value.strideY = y;
    argument.endStructure();
  }

  argument >> value.metadata;
  argument.endStructure();
  return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument, const voxie::scripting::Array3Info &value) {
  argument.beginStructure();
  argument << value.handle;
  argument << (qint64) value.offset;
  argument.beginStructure();
  argument << value.dataType;
  argument << value.dataTypeSize;
  argument << value.byteorder;
  argument.endStructure();
  argument.beginStructure();
  argument << (quint64) value.sizeX << (quint64) value.sizeY << (quint64) value.sizeZ;
  argument.endStructure();
  argument.beginStructure();
  argument << (qint64) value.strideX << (qint64) value.strideY << (qint64) value.strideZ;
  argument.endStructure();
  argument << value.metadata;
  argument.endStructure();
  return argument;
}
const QDBusArgument &operator>>(const QDBusArgument &argument, voxie::scripting::Array3Info &value) {
  argument.beginStructure();

  argument >> value.handle;
  qint64 offset;
  argument >> offset;
  value.offset = offset;

  argument.beginStructure();
  argument >> value.dataType;
  quint32 dataTypeSize;
  argument >> dataTypeSize;
  value.dataTypeSize = dataTypeSize;
  argument >> value.byteorder;
  argument.endStructure();

  {
    argument.beginStructure();
    quint64 x, y, z;
    argument >> x >> y >> z;
    value.sizeX = x;
    value.sizeY = y;
    value.sizeZ = z;
    argument.endStructure();
  }

  {
    argument.beginStructure(); 
    qint64 x, y, z;
    argument >> x >> y >> z;
    value.strideX = x;
    value.strideY = y;
    value.strideZ = z;
    argument.endStructure();
  }

  argument >> value.metadata;
  argument.endStructure();
  return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument, const voxie::scripting::Plane &value) {
  argument.beginStructure();
  argument << value.origin << value.rotation;
  argument.endStructure();
  return argument;
}
const QDBusArgument &operator>>(const QDBusArgument &argument, voxie::scripting::Plane &value) {
  argument.beginStructure();
  argument >> value.origin >> value.rotation;
  argument.endStructure();
  return argument;
}

/*
template <typename T>
QDBusArgument &operator<<(QDBusArgument &argument, T* const & value) {
  argument.beginStructure();
  argument << (value ? value->getPath() : QDBusObjectPath ("/"));
  argument.endStructure();
  return argument;
}
template <typename T>
const QDBusArgument &operator>>(const QDBusArgument &argument, T* &value) {
  argument.beginStructure();
  QDBusObjectPath s;
  argument >> s;
  argument.endStructure();
  if (s.path() == "/") {
    value = nullptr;
  } else {
    value = qobject_cast<T*>(voxie::scripting::ScriptingContainerBase::lookupWeakQObject(s.path));
  }
  return argument;
}
*/

namespace voxie {
namespace scripting {

void initDBusTypes() {
    qDBusRegisterMetaType<QVector2D>();
    qDBusRegisterMetaType<QVector3D>();
    qDBusRegisterMetaType<QQuaternion>();

    qDBusRegisterMetaType<QVector<QDBusObjectPath>> ();
    qDBusRegisterMetaType<QMap_QDBusObjectPath_quint64> ();

    qDBusRegisterMetaType<voxie::scripting::IntVector2>();
    qDBusRegisterMetaType<voxie::scripting::IntVector3>();

    qDBusRegisterMetaType<voxie::scripting::Array2Info>();
    qDBusRegisterMetaType<voxie::scripting::Array3Info>();

    qDBusRegisterMetaType<voxie::scripting::Plane>();

    /*
    qDBusRegisterMetaType<voxie::data::VoxelData*>();
    qDBusRegisterMetaType<voxie::data::Slice*>();
    qDBusRegisterMetaType<voxie::visualization::Visualizer*>();
    */
}

}
}
