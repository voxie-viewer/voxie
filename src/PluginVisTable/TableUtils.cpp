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

#include "TableUtils.hpp"

#include <Voxie/Data/BoundingBox3D.hpp>

#include <Voxie/Node/Types.hpp>

#include <VoxieClient/DBusTypeList.hpp>

bool TableUtils::isNumericType(const vx::PropertyType& type) {
  static QSet<QString> numericTypes = {
      vx::types::FloatType()->name(),
      vx::types::IntType()->name(),
      vx::types::BooleanType()->name(),
  };

  return numericTypes.contains(type.name());
}

bool TableUtils::isComparableType(
    const QSharedPointer<vx::PropertyType>& type) {
  /*
  static QSet<QString> comparableTypes = {
      vx::types::StringType()->name(),
      vx::types::EnumerationType()->name(),
  };

  return isNumericType(type) || comparableTypes.contains(type.name());
  */
  return type->isComparable();
}

bool TableUtils::isPolyNumericType(const vx::PropertyType& type) {
  static QSet<QString> polyNumericTypes = {
      vx::types::Position3DType()->name(),
      vx::types::BoundingBox3DType()->name(),
  };

  return isNumericType(type) || polyNumericTypes.contains(type.name());
}

QString TableUtils::getColumnLabel(QSharedPointer<vx::TableData> tableData,
                                   QString columnName) {
  if (tableData) {
    int index = tableData->getColumnIndexByName(columnName);
    if (index >= 0) {
      return getColumnLabel(tableData->columns()[index]);
    }
  }
  return "";
}

QString TableUtils::getColumnLabel(const vx::TableColumn& column) {
  QString unitSuffix =
      column.metadata().contains("unit")
          ? " [" + column.metadata()["unit"].variant().toString() + "]"
          : "";
  return column.displayName() + unitSuffix;
}

TableUtils::NumberExtractor TableUtils::getNumberExtractor(
    const vx::PropertyType& type) {
  if (isNumericType(type)) {
    return [](const QVariant& value) {
      NumberArray array;
      array.append(value.toDouble());
      return array;
    };
  }

  if (type.name() == vx::types::Position3DType()->name()) {
    return [](const QVariant& value) {
      auto position = value.value<vx::types::Position3D::RawType>();
      NumberArray array;
      array.append(std::get<0>(position));
      array.append(std::get<1>(position));
      array.append(std::get<2>(position));
      return array;
    };
  }

  if (type.name() == vx::types::BoundingBox3DType()->name()) {
    return [](const QVariant& value) {
      auto box = value.value<vx::types::BoundingBox3D::RawType>();
      NumberArray array;
      array.append(std::get<0>(std::get<0>(box)));
      array.append(std::get<1>(std::get<0>(box)));
      array.append(std::get<2>(std::get<0>(box)));
      array.append(std::get<0>(std::get<1>(box)));
      array.append(std::get<1>(std::get<1>(box)));
      array.append(std::get<2>(std::get<1>(box)));
      return array;
    };
  }

  return [](const QVariant&) { return NumberArray(); };
}

TableUtils::NumberManipulator TableUtils::getNumberManipulator(
    const vx::PropertyType& type) {
  if (isNumericType(type)) {
    return [](const QVariant& value, const NumberFunction& function) {
      return QVariant::fromValue(function(value.toDouble()));
    };
  }

  if (type.name() == vx::types::Position3DType()->name()) {
    return [](const QVariant& value, const NumberFunction& function) {
      auto position = value.value<vx::types::Position3D::RawType>();
      auto tuple = std::make_tuple(function(std::get<0>(position)),
                                   function(std::get<1>(position)),
                                   function(std::get<2>(position)));
      return QVariant::fromValue(tuple);
    };
  }

  if (type.name() == vx::types::BoundingBox3DType()->name()) {
    return [](const QVariant& value, const NumberFunction& function) {
      auto box = value.value<vx::types::BoundingBox3D::RawType>();
      auto tuple = std::make_tuple(
          std::make_tuple(function(std::get<0>(std::get<0>(box))),
                          function(std::get<1>(std::get<0>(box))),
                          function(std::get<2>(std::get<0>(box)))),
          std::make_tuple(function(std::get<0>(std::get<1>(box))),
                          function(std::get<1>(std::get<1>(box))),
                          function(std::get<2>(std::get<1>(box)))));
      return QVariant::fromValue(tuple);
    };
  }

  return [](const QVariant& value, const NumberFunction&) { return value; };
}
