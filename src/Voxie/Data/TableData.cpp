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

#include "TableData.hpp"
#include <VoxieClient/DBusAdaptors.hpp>

#include <VoxieBackend/Property/PropertyType.hpp>

#include <Voxie/Node/Types.hpp>

using namespace vx;

// TODO: EnumerationType ?

TableColumn::TableColumn(const QString& name,
                         const QSharedPointer<PropertyType> type,
                         const QString& displayName,
                         const QMap<QString, QDBusVariant>& metadata)
    : name_(name), type_(type), displayName_(displayName), metadata_(metadata) {
  if (!isAllowedType(type_))
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InvalidArgument",
        "Type " + type_->name() + " is not a valid type for table columns");
}
TableColumn::~TableColumn() {}

// Allowed types for table columns. These types should not have any custom
// verify function.
bool TableColumn::isAllowedType(const QSharedPointer<PropertyType>& type) {
  return type == vx::types::BooleanType() || type == vx::types::ColorType() ||
         type == vx::types::FloatType() || type == vx::types::IntType() ||
         type == vx::types::Orientation3DType() ||
         type == vx::types::Point2DType() ||
         type == vx::types::Position3DType() ||
         type == vx::types::Box3DAxisAlignedType() ||
         type == vx::types::SizeInteger3DType() ||
         type == vx::types::StringType();
}

class TableDataAdaptorImpl : public TableDataAdaptor {
  TableData* object;

 public:
  TableDataAdaptorImpl(TableData* object)
      : TableDataAdaptor(object), object(object) {}
  ~TableDataAdaptorImpl() override {}

  QList<std::tuple<QString, QDBusObjectPath, QString,
                   QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>
  columns() const override {
    try {
      QList<
          std::tuple<QString, QDBusObjectPath, QString,
                     QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>
          result;
      for (const auto& column : object->columns()) {
        QMap<QString, QDBusVariant> metadata = column.metadata();
        QMap<QString, QDBusVariant> options;
        result << std::make_tuple(column.name(),
                                  ExportedObject::getPath(column.type()),
                                  column.displayName(), metadata, options);
      }
      return result;
    } catch (Exception& e) {
      e.handle(object);
      return QList<std::tuple<QString, QDBusObjectPath, QString,
                              QMap<QString, QDBusVariant>,
                              QMap<QString, QDBusVariant>>>();
    }
  }

  qulonglong rowCount() const override {
    try {
      return object->rowCount();
    } catch (Exception& e) {
      e.handle(object);
      return 0;
    }
  }

  qulonglong AddRow(const QDBusObjectPath& update, const QList<QVariant>& data,
                    const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      auto updateObj = vx::DataUpdate::lookup(update);
      updateObj->validateCanUpdate(object);

      return object->addRowDBus(updateObj, data);
    } catch (Exception& e) {
      e.handle(object);
      return 0;
    }
  }

  bool RemoveRow(const QDBusObjectPath& update, quint64 rowId,
                 const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      auto updateObj = vx::DataUpdate::lookup(update);
      updateObj->validateCanUpdate(object);

      return object->removeRow(updateObj, rowId);
    } catch (Exception& e) {
      e.handle(object);
      return 0;
    }
  }

  QList<std::tuple<quint64, QList<QDBusVariant>>> GetRows(
      qulonglong firstRowID, qulonglong lastRowID,
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      return object->getRowsDBus(firstRowID, lastRowID);
    } catch (Exception& e) {
      e.handle(object);
      return QList<std::tuple<quint64, QList<QDBusVariant>>>();
    }
  }
};

TableData::TableData(const QList<TableColumn>& columns) : columns_(columns) {
  new TableDataAdaptorImpl(this);
}
TableData::~TableData() {}

QList<QString> TableData::supportedDBusInterfaces() {
  return {
      "de.uni_stuttgart.Voxie.TableData",
  };
}

int TableData::getColumnIndexByName(QString columnName) const {
  for (int i = 0; i < columns().size(); ++i) {
    if (columns()[i].name() == columnName) {
      return i;
    }
  }
  return -1;
}

quint64 TableData::rowCount() {
  QMutexLocker lock(&mutex);
  return rows_.size();
}

quint64 TableData::getMaxRowID() {
  QMutexLocker lock(&mutex);
  return this->maxRowID;
}

quint64 TableData::addRow(const QSharedPointer<DataUpdate>& update,
                          const QList<QVariant>& data) {
  Q_UNUSED(update);  // TODO: check update?

  if (data.size() != columns().size())
    throw Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                    "Number of columns does not match table");
  for (int i = 0; i < data.size(); i++) {
    if (data[i].userType() != columns()[i].type()->getRawQMetaType())
      throw Exception(
          "de.uni_stuttgart.Voxie.InvalidArgument",
          QString("Table value has unexpected type, expected %1, got %2 (%3)")
              .arg(QMetaType::typeName(columns()[i].type()->getRawQMetaType()))
              .arg(QMetaType::typeName(data[i].userType()))
              .arg(data[i].userType()));
  }

  QMutexLocker lock(&mutex);

  maxRowID++;
  auto id = maxRowID;
  rows_ << TableRow(id, data);
  return id;
}
// TODO: Implement based on addRow()?
quint64 TableData::addRowDBus(const QSharedPointer<DataUpdate>& update,
                              const QList<QVariant>& data) {
  Q_UNUSED(update);  // TODO: check update?

  if (data.size() != columns().size())
    throw Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                    "Number of columns does not match table");
  QList<QVariant> dataRaw;
  for (int i = 0; i < data.size(); i++) {
    dataRaw << columns()[i].type()->dbusToRaw(QDBusVariant(data[i]));
    /*
    if (data[i].userType() != columns()[i].type()->getRawQMetaType())
      throw Exception(
          "de.uni_stuttgart.Voxie.InvalidArgument",
          QString("Table value has unexpected type, expected %1, got %2 (%3)")
              .arg(QMetaType::typeName(columns()[i].type()->getRawQMetaType()))
              .arg(QMetaType::typeName(data[i].userType()))
              .arg(data[i].userType()));
    */
  }

  QMutexLocker lock(&mutex);

  maxRowID++;
  auto id = maxRowID;
  rows_ << TableRow(id, dataRaw);
  return id;
}

bool TableData::removeRow(const QSharedPointer<DataUpdate>& update,
                          quint64 rowId) {
  Q_UNUSED(update);  // TODO: check update?

  QMutexLocker lock(&mutex);

  // TODO: should be faster
  for (int i = 0; i < rows_.size(); i++) {
    const auto& row = rows_[i];
    if (row.rowID() != rowId) continue;
    rows_.removeAt(i);
    return true;
  }
  return false;
}

void TableData::clear(const QSharedPointer<DataUpdate>& update) {
  Q_UNUSED(update);  // TODO: check update?

  QMutexLocker lock(&mutex);
  rows_.clear();
}

QList<TableRow> TableData::getRowsByIndex(quint64 firstRowIndex,
                                          quint64 lastRowIndex) {
  QMutexLocker lock(&mutex);

  lastRowIndex = std::min<quint64>(lastRowIndex, rows_.size());

  QList<TableRow> result;
  for (quint64 i = firstRowIndex; i < lastRowIndex; ++i) {
    result << rows_[i];
  }

  return result;
}

QVariant TableData::getRowColumnData(quint64 rowIndex, QString columnName) {
  int colIndex = this->getColumnIndexByName(columnName);
  if (colIndex < 0) {
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InvalidArgument",
        "Column Name " + columnName + " is not a valid name for table columns");
    // return invalid QVariant
    return QVariant();
  }
  return getRowsByIndex(rowIndex, rowIndex + 1).first().data().at(colIndex);
}

// TODO: Implement based on getRows()?
QList<std::tuple<quint64, QList<QDBusVariant>>> TableData::getRowsDBus(
    quint64 firstRowID, quint64 lastRowID) {
  QMutexLocker lock(&mutex);

  QList<std::tuple<quint64, QList<QDBusVariant>>> result;
  // TODO: do this faster by not looking up unneeded values
  for (const auto& row : rows_) {
    if (row.rowID() < firstRowID || row.rowID() > lastRowID) continue;
    QList<QDBusVariant> dataDBus;
    for (int i = 0; i < columns().size(); i++)
      dataDBus << columns()[i].type()->rawToDBus(row.data()[i]);
    result << std::make_tuple(row.rowID(), dataDBus);
  }
  return result;
}

bool TableData::modifyRowEntry(const QSharedPointer<DataUpdate>& update,
                               quint64 rowId, quint64 columId,
                               QVariant newValue) {
  Q_UNUSED(update);  // TODO: check update?
  if (columId >= (quint64)columns().size())
    throw Exception(
        "de.uni_stuttgart.Voxie.InvalidArgument",
        QString(
            "ColumnId does not exist in table, table has %1 columns, got %2")
            .arg(columns().size())
            .arg(columId));

  QMutexLocker lock(&mutex);

  const auto row =
      std::find_if(rows_.begin(), rows_.end(),
                   [rowId](TableRow const& tr) { return tr.rowID() == rowId; });

  if (row != rows_.end()) {
    QList<QVariant> existing = row->data();

    if (newValue.userType() != columns()[columId].type()->getRawQMetaType())
      throw Exception(
          "de.uni_stuttgart.Voxie.InvalidArgument",
          QString("Table value has unexpected type, expected %1, got %2 (%3)")
              .arg(QMetaType::typeName(
                  columns()[columId].type()->getRawQMetaType()))
              .arg(QMetaType::typeName(newValue.userType()))
              .arg(newValue.userType()));

    existing.replace(columId, newValue);
    rows_.replace(row - rows_.begin(), TableRow(rowId, existing));
    return true;
  } else {
    return false;
  }
}

QList<QSharedPointer<SharedMemory>> TableData::getSharedMemorySections() {
  return {};
}
