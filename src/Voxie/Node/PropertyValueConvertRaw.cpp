/*
 * Copyright (c) 2014-2022 The Voxie Authors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "PropertyValueConvertRaw.hpp"

#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <VoxieClient/JsonDBus.hpp>

#include <Voxie/Data/Color.hpp>
#include <Voxie/Data/ColorizerEntry.hpp>
#include <VoxieBackend/Data/DataType.hpp>

#include <VoxieBackend/DBus/DBusTypes.hpp>

#include <Voxie/Node/Node.hpp>

#include <QtCore/QJsonObject>

using namespace vx;

vx::Node* PropertyValueConvertRaw<QDBusObjectPath, vx::Node*>::fromRaw(
    const QDBusObjectPath& raw) {
  if (raw.path() == "/") return nullptr;
  auto ptr = ExportedObject::lookupWeakObject(raw);
  if (!ptr) {
    /*
    throw Exception("de.uni_stuttgart.Voxie.Error",
                             "Failed to look up object " + raw.path());
    */
    qWarning() << "Failed to look up object" << raw.path();
    return nullptr;
  }
  auto obj = dynamic_cast<Node*>(ptr);
  if (!obj) {
    /*
    throw Exception("de.uni_stuttgart.Voxie.Error",
                             "Path " + raw.path() + " is not a Node");
    */
    qWarning() << "Path" << raw.path() << "is not a Node";
    return nullptr;
  }
  return obj;
}
QDBusObjectPath PropertyValueConvertRaw<QDBusObjectPath, vx::Node*>::toRaw(
    vx::Node* cooked) {
  return cooked ? cooked->getPath() : QDBusObjectPath("/");
}

QList<vx::Node*>
PropertyValueConvertRaw<QList<QDBusObjectPath>, QList<vx::Node*>>::fromRaw(
    const QList<QDBusObjectPath>& raw) {
  QList<Node*> res;
  for (const auto& value : raw) {
    if (value.path() == "/") {
      qWarning() << "Node list contains nullptr";
      continue;
    }
    auto ptr = ExportedObject::lookupWeakObject(value);
    if (!ptr) {
      /*
      throw Exception("de.uni_stuttgart.Voxie.Error",
                               "Failed to look up object " + value.path());
      */
      qWarning() << "Failed to look up object" << value.path() << "for list";
      continue;
    }
    auto obj = dynamic_cast<Node*>(ptr);
    if (!obj) {
      /*
      throw Exception(
          "de.uni_stuttgart.Voxie.Error",
          "Path " + value.path() + " is not a Node");
      */
      qWarning() << "Path" << value.path() << "is not a Node for node list";
      continue;
    }
    res.append(obj);
  }
  return res;
}
QList<QDBusObjectPath>
PropertyValueConvertRaw<QList<QDBusObjectPath>, QList<vx::Node*>>::toRaw(
    const QList<vx::Node*>& cooked) {
  QList<QDBusObjectPath> ret;
  for (const auto& obj : cooked) {
    if (obj) {
      ret.append(obj->getPath());
    } else {
      qWarning() << "Node list contains nullptr";
    }
  }
  return ret;
}

QPointF PropertyValueConvertRaw<vx::TupleVector<double, 2>, QPointF>::fromRaw(
    const vx::TupleVector<double, 2>& raw) {
  return QPointF(std::get<0>(raw), std::get<1>(raw));
}
vx::TupleVector<double, 2> PropertyValueConvertRaw<
    vx::TupleVector<double, 2>, QPointF>::toRaw(const QPointF& cooked) {
  return std::make_tuple(cooked.x(), cooked.y());
}

QVector3D
PropertyValueConvertRaw<std::tuple<double, double, double>, QVector3D>::fromRaw(
    const std::tuple<double, double, double>& raw) {
  return QVector3D(std::get<0>(raw), std::get<1>(raw), std::get<2>(raw));
}
std::tuple<double, double, double> PropertyValueConvertRaw<
    std::tuple<double, double, double>, QVector3D>::toRaw(QVector3D cooked) {
  return std::make_tuple(cooked.x(), cooked.y(), cooked.z());
}

QQuaternion PropertyValueConvertRaw<std::tuple<double, double, double, double>,
                                    QQuaternion>::
    fromRaw(const std::tuple<double, double, double, double>& raw) {
  return QQuaternion(std::get<0>(raw), std::get<1>(raw), std::get<2>(raw),
                     std::get<3>(raw));
}
std::tuple<double, double, double, double>
PropertyValueConvertRaw<std::tuple<double, double, double, double>,
                        QQuaternion>::toRaw(QQuaternion cooked) {
  return std::make_tuple(cooked.scalar(), cooked.x(), cooked.y(), cooked.z());
}

VectorSizeT3 PropertyValueConvertRaw<std::tuple<uint64_t, uint64_t, uint64_t>,
                                     VectorSizeT3>::
    fromRaw(const std::tuple<uint64_t, uint64_t, uint64_t>& raw) {
  return VectorSizeT3(std::get<0>(raw), std::get<1>(raw), std::get<2>(raw));
}
std::tuple<uint64_t, uint64_t, uint64_t>
PropertyValueConvertRaw<std::tuple<uint64_t, uint64_t, uint64_t>,
                        VectorSizeT3>::toRaw(VectorSizeT3 cooked) {
  return std::make_tuple(cooked.x, cooked.y, cooked.z);
}

DataType PropertyValueConvertRaw<
    std::tuple<QString, quint32, QString>,
    DataType>::fromRaw(const std::tuple<QString, quint32, QString>& raw) {
  return parseDataTypeStruct(raw);
}
std::tuple<QString, quint32, QString> PropertyValueConvertRaw<
    std::tuple<QString, quint32, QString>, DataType>::toRaw(DataType cooked) {
  // Note: This will return 'native' as byteorder
  return getDataTypeStruct(cooked, true);
}

Color PropertyValueConvertRaw<TupleVector<double, 4>, Color>::fromRaw(
    const TupleVector<double, 4>& raw) {
  return Color(raw);
}
TupleVector<double, 4> PropertyValueConvertRaw<
    TupleVector<double, 4>, Color>::toRaw(const Color& cooked) {
  return cooked.asTuple();
}

QList<ColorizerEntry> PropertyValueConvertRaw<
    QList<std::tuple<double, TupleVector<double, 4>, qint32>>,
    QList<ColorizerEntry>>::
    fromRaw(
        const QList<std::tuple<double, TupleVector<double, 4>, qint32>>& raw) {
  QList<ColorizerEntry> result;
  for (const auto& entry : raw)
    result.append(ColorizerEntry(
        std::get<0>(entry), Color(std::get<1>(entry)),
        ColorInterpolator::InterpolationType(std::get<2>(entry))));
  return result;
}

QList<QVector3D> PropertyValueConvertRaw<
    QList<vx::TupleVector<double, 3>>,
    QList<QVector3D>>::fromRaw(const QList<vx::TupleVector<double, 3>>& raw) {
  QList<QVector3D> res;

  for (const auto& value : raw) {
    res.append(
        QVector3D(std::get<0>(value), std::get<1>(value), std::get<2>(value)));
  }
  return res;
}

QList<vx::TupleVector<double, 3>> PropertyValueConvertRaw<
    QList<vx::TupleVector<double, 3>>,
    QList<QVector3D>>::toRaw(const QList<QVector3D>& cooked) {
  QList<vx::TupleVector<double, 3>> res;

  for (const auto& value : cooked) {
    res.append(vx::TupleVector<double, 3>(value.x(), value.y(), value.z()));
  }
  return res;
}

QList<std::tuple<vx::TupleVector<double, 3>, double>>
PropertyValueConvertRaw<QList<std::tuple<vx::TupleVector<double, 3>, double>>,
                        QList<std::tuple<QVector3D, double>>>::
    toRaw(const QList<std::tuple<QVector3D, double>>& cooked) {
  QList<std::tuple<vx::TupleVector<double, 3>, double>> res;

  for (const auto& value : cooked) {
    res.append(
        std::make_tuple(vx::TupleVector<double, 3>(std::get<0>(value).x(),
                                                   std::get<0>(value).y(),
                                                   std::get<0>(value).z()),
                        std::get<1>(value)));
  }
  return res;
}

QList<std::tuple<QVector3D, double>>
PropertyValueConvertRaw<QList<std::tuple<vx::TupleVector<double, 3>, double>>,
                        QList<std::tuple<QVector3D, double>>>::
    fromRaw(const QList<std::tuple<vx::TupleVector<double, 3>, double>>& raw) {
  QList<std::tuple<QVector3D, double>> res;

  for (const auto& value : raw) {
    res.append(std::make_tuple(QVector3D(std::get<0>(std::get<0>(value)),
                                         std::get<1>(std::get<0>(value)),
                                         std::get<2>(std::get<0>(value))),
                               std::get<1>(value)));
  }
  return res;
}

QList<std::tuple<double, TupleVector<double, 4>, qint32>>
PropertyValueConvertRaw<
    QList<std::tuple<double, TupleVector<double, 4>, qint32>>,
    QList<ColorizerEntry>>::toRaw(const QList<ColorizerEntry>& cooked) {
  QList<std::tuple<double, TupleVector<double, 4>, qint32>> result;
  for (const auto& entry : cooked)
    result.append(std::make_tuple(entry.value(), entry.color().asTuple(),
                                  (int)entry.interpolator().getType()));
  return result;
}
