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

#include <VoxieBackend/Data/Data.hpp>

#include <memory>

#include <QtCore/QMutex>

namespace vx {
class PropertyType;

class VOXIECORESHARED_EXPORT TableColumn {
  QString name_;
  QSharedPointer<PropertyType> type_;
  QString displayName_;
  QMap<QString, QDBusVariant> metadata_;

 public:
  TableColumn(const QString& name, const QSharedPointer<PropertyType> type,
              const QString& displayName,
              const QMap<QString, QDBusVariant>& metadata);
  ~TableColumn();

  const QString& name() const { return name_; }
  const QSharedPointer<PropertyType>& type() const { return type_; }
  const QString& displayName() const { return displayName_; }
  const QMap<QString, QDBusVariant>& metadata() const { return metadata_; }

  static bool isAllowedType(const QSharedPointer<PropertyType>& type);
};

class VOXIECORESHARED_EXPORT TableRow {
  quint64 rowID_;
  QList<QVariant> data_;

 public:
  TableRow(quint64 rowID, const QList<QVariant>& data)
      : rowID_(rowID), data_(data) {}

  quint64 rowID() const { return rowID_; }
  const QList<QVariant>& data() const { return data_; }
};

class VOXIECORESHARED_EXPORT TableData : public Data {
  Q_OBJECT
  VX_REFCOUNTEDOBJECT

 private:
  // read-only
  QList<TableColumn> columns_;

  QMutex mutex;
  // protected by mutex
  QList<TableRow> rows_;
  quint64 maxRowID = 0;

 public:
  // throws Exception
  // TODO: should this be private so that only the create() method can call it?
  TableData(const QList<TableColumn>& columns = QList<TableColumn>());

 public:
  ~TableData();

  QList<QString> supportedDBusInterfaces() override;

  const QList<TableColumn>& columns() const { return columns_; }

  int getColumnIndexByName(QString columnName) const;

  quint64 rowCount();

  quint64 getMaxRowID();

  quint64 addRow(const QSharedPointer<DataUpdate>& update,
                 const QList<QVariant>& data);
  quint64 addRowDBus(const QSharedPointer<DataUpdate>& update,
                     const QList<QVariant>& data);

  bool modifyRowEntry(const QSharedPointer<DataUpdate>& update, quint64 rowId,
                      quint64 columId, QVariant newValue);

  bool removeRow(const QSharedPointer<DataUpdate>& update, quint64 rowId);

  void clear(const QSharedPointer<DataUpdate>& update);

  QList<TableRow> getRowsByIndex(
      quint64 firstRowID = 0,
      quint64 lastRowID = std::numeric_limits<quint64>::max());

  QVariant getRowColumnData(quint64 rowIndex, QString columnName);

  QList<std::tuple<quint64, QList<QDBusVariant>>> getRowsDBus(
      quint64 firstRowID, quint64 lastRowID);

  QList<QSharedPointer<SharedMemory>> getSharedMemorySections() override;
};
}  // namespace vx
