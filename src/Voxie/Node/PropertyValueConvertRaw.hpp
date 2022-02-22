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

#pragma once

#include <Voxie/Voxie.hpp>

#include <VoxieClient/DBusTypeList.hpp>

namespace vx {
class NodePrototype;
struct VectorSizeT3;
class Color;
class Node;
class ColorizerEntry;
enum class DataType;

template <typename Raw, typename Cooked>
// Must not be marked with VOXIECORESHARED_EXPORT because it
// PropertyValueConvertRaw<T, T> is inline (not really sure how exporting works
// on windows here)
struct PropertyValueConvertRaw;

template <typename T>
// Must not be marked with VOXIECORESHARED_EXPORT because it is inline
struct PropertyValueConvertRaw<T, T> {
  static T fromRaw(const T& raw) { return raw; }
  static T toRaw(const T& cooked) { return cooked; }
};

template <>
struct VOXIECORESHARED_EXPORT
    PropertyValueConvertRaw<QDBusObjectPath, vx::Node*> {
  static vx::Node* fromRaw(const QDBusObjectPath& raw);
  static QDBusObjectPath toRaw(vx::Node* cooked);
};

template <>
struct VOXIECORESHARED_EXPORT
    PropertyValueConvertRaw<QList<QDBusObjectPath>, QList<vx::Node*>> {
  static QList<vx::Node*> fromRaw(const QList<QDBusObjectPath>& raw);
  static QList<QDBusObjectPath> toRaw(const QList<vx::Node*>& cooked);
};

template <>
struct VOXIECORESHARED_EXPORT
    PropertyValueConvertRaw<vx::TupleVector<double, 2>, QPointF> {
  static QPointF fromRaw(const vx::TupleVector<double, 2>& raw);
  static vx::TupleVector<double, 2> toRaw(const QPointF& cooked);
};

template <>
struct VOXIECORESHARED_EXPORT
    PropertyValueConvertRaw<std::tuple<double, double, double>, QVector3D> {
  static QVector3D fromRaw(const std::tuple<double, double, double>& raw);
  static std::tuple<double, double, double> toRaw(QVector3D cooked);
};

template <>
struct VOXIECORESHARED_EXPORT PropertyValueConvertRaw<
    std::tuple<double, double, double, double>, QQuaternion> {
  static QQuaternion fromRaw(
      const std::tuple<double, double, double, double>& raw);
  static std::tuple<double, double, double, double> toRaw(QQuaternion cooked);
};

template <>
struct VOXIECORESHARED_EXPORT PropertyValueConvertRaw<
    std::tuple<uint64_t, uint64_t, uint64_t>, VectorSizeT3> {
  static VectorSizeT3 fromRaw(
      const std::tuple<uint64_t, uint64_t, uint64_t>& raw);
  static std::tuple<uint64_t, uint64_t, uint64_t> toRaw(VectorSizeT3 cooked);
};

template <>
struct VOXIECORESHARED_EXPORT PropertyValueConvertRaw<
    QList<vx::TupleVector<double, 3>>, QList<QVector3D>> {
  static QList<QVector3D> fromRaw(const QList<vx::TupleVector<double, 3>>& raw);
  static QList<vx::TupleVector<double, 3>> toRaw(
      const QList<QVector3D>& cooked);
};

template <>
struct VOXIECORESHARED_EXPORT PropertyValueConvertRaw<
    QList<std::tuple<vx::TupleVector<double, 3>, double>>,
    QList<std::tuple<QVector3D, double>>> {
  static QList<std::tuple<QVector3D, double>> fromRaw(
      const QList<std::tuple<vx::TupleVector<double, 3>, double>>& raw);
  static QList<std::tuple<vx::TupleVector<double, 3>, double>> toRaw(
      const QList<std::tuple<QVector3D, double>>& cooked);
};

template <>
struct VOXIECORESHARED_EXPORT
    PropertyValueConvertRaw<std::tuple<QString, quint32, QString>, DataType> {
  static DataType fromRaw(const std::tuple<QString, quint32, QString>& raw);
  static std::tuple<QString, quint32, QString> toRaw(DataType cooked);
};

template <>
struct VOXIECORESHARED_EXPORT
    PropertyValueConvertRaw<TupleVector<double, 4>, Color> {
  static Color fromRaw(const TupleVector<double, 4>& raw);
  static TupleVector<double, 4> toRaw(const Color& cooked);
};

template <>
struct VOXIECORESHARED_EXPORT PropertyValueConvertRaw<
    QList<std::tuple<double, TupleVector<double, 4>, qint32>>,
    QList<ColorizerEntry>> {
  static QList<ColorizerEntry> fromRaw(
      const QList<std::tuple<double, TupleVector<double, 4>, qint32>>& raw);
  static QList<std::tuple<double, TupleVector<double, 4>, qint32>> toRaw(
      const QList<ColorizerEntry>& cooked);
};
}  // namespace vx
