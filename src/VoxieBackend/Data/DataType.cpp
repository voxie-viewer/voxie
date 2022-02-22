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

#include "DataType.hpp"

#include <VoxieClient/Exception.hpp>

using namespace vx;

// clang-format off
QMap<DataType, QString> vx::DataTypeNames {
  {DataType::Float16, "float16"},
  {DataType::Float32, "float32"},
  {DataType::Float64, "float64"},
  {DataType::Int8, "int8"},
  {DataType::Int16, "int16"},
  {DataType::Int32, "int32"},
  {DataType::Int64, "int64"},
  {DataType::UInt8, "uint8"},
  {DataType::UInt16, "uint16"},
  {DataType::UInt32, "uint32"},
  {DataType::UInt64, "uint64"},
  {DataType::Bool8, "bool8"}
  // TODO: Add a bit-aligned boolean
};
// clang-format on

// TODO: Show list of supported data types?
void vx::switchOverDataTypeFailed(DataType dt) {
  throw vx::Exception(
      "de.uni_stuttgart.Voxie.Error",
      "Data type unsupported in this context: " + getDataTypeString(dt));
}

QString vx::getNativeByteorder() {
  if (QSysInfo::Endian::ByteOrder == QSysInfo::Endian::BigEndian) {
    return "big";
  } else if (QSysInfo::Endian::ByteOrder == QSysInfo::Endian::LittleEndian) {
    return "little";
  } else {
    return "unknown";
  }
}

void vx::getDataTypeInfo(DataType type, QString& dataType,
                         quint32& dataTypeSize, QString& byteorder,
                         bool getAsNative) {
  switch (type) {
    case DataType::Float16:
    case DataType::Float32:
    case DataType::Float64:
      dataType = "float";
      break;
    case DataType::UInt8:
    case DataType::UInt16:
    case DataType::UInt32:
    case DataType::UInt64:
      dataType = "uint";
      break;
    case DataType::Int8:
    case DataType::Int16:
    case DataType::Int32:
    case DataType::Int64:
      dataType = "int";
      break;
    case DataType::Bool8:
      dataType = "bool";
      break;
    default:
      throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                          "Data type not supported.");
  }

  switch (type) {
    case DataType::Int8:
    case DataType::UInt8:
      dataTypeSize = 8;
      break;

    case DataType::Int16:
    case DataType::UInt16:
    case DataType::Float16:
      dataTypeSize = 16;
      break;

    case DataType::Float32:
    case DataType::Int32:
    case DataType::UInt32:
      dataTypeSize = 32;
      break;

    case DataType::Float64:
    case DataType::UInt64:
    case DataType::Int64:
      dataTypeSize = 64;
      break;

    case DataType::Bool8:
      dataTypeSize = 8;
      break;

    default:
      throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                          "Data type not supported.");
  }

  if (getAsNative) {
    byteorder = "native";
  } else {
    if (dataTypeSize == 8) {
      byteorder = "none";
    } else {
      byteorder = getNativeByteorder();
    }
  }
}

DataType vx::parseDataTypeString(const QString& type) {
  for (const auto& key : DataTypeNames.keys())
    if (DataTypeNames[key] == type) return key;
  throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                      "Unknown data type");
}

QString vx::getDataTypeString(DataType type) {
  if (!DataTypeNames.contains(type))
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "Data type not supported.");
  return DataTypeNames[type];
}

DataType vx::parseDataTypeStruct(
    const std::tuple<QString, quint32, QString>& type) {
  QString dataType = std::get<0>(type);
  quint32 dataTypeSize = std::get<1>(type);
  QString byteorder = std::get<2>(type);

  // byteorder 'native' is treated specially (is ignored here)
  if (byteorder != "native") {
    QString actualByteorder;
    if (dataTypeSize == 8) {
      actualByteorder = "none";
    } else {
      if (QSysInfo::Endian::ByteOrder == QSysInfo::Endian::BigEndian) {
        actualByteorder = "big";
      } else if (QSysInfo::Endian::ByteOrder ==
                 QSysInfo::Endian::LittleEndian) {
        actualByteorder = "little";
      } else {
        actualByteorder = "unknown";
      }
    }
    if (byteorder != actualByteorder)
      throw vx::Exception("de.uni_stuttgart.Voxie.InvalidDataType",
                          "Expected byteorder '" + actualByteorder +
                              "', got '" + byteorder + "'");
  }

  if (dataType == "float") {
    switch (dataTypeSize) {
      case 16:
        return DataType::Float16;
      case 32:
        return DataType::Float32;
      case 64:
        return DataType::Float64;
    }
  } else if (dataType == "uint") {
    switch (dataTypeSize) {
      case 8:
        return DataType::UInt8;
      case 16:
        return DataType::UInt16;
      case 32:
        return DataType::UInt32;
      case 64:
        return DataType::UInt64;
    }
  } else if (dataType == "int") {
    switch (dataTypeSize) {
      case 8:
        return DataType::Int8;
      case 16:
        return DataType::Int16;
      case 32:
        return DataType::Int32;
      case 64:
        return DataType::Int64;
    }
  } else if (dataType == "bool") {
    switch (dataTypeSize) {
      case 8:
        return DataType::Bool8;
    }
  }
  throw vx::Exception("de.uni_stuttgart.Voxie.InvalidDataType",
                      "Don't know data type '" + dataType + "' with size " +
                          QString::number(dataTypeSize));
}

std::tuple<QString, quint32, QString> vx::getDataTypeStruct(DataType type,
                                                            bool getAsNative) {
  QString dataType;
  quint32 dataTypeSize;
  QString byteorder;
  getDataTypeInfo(type, dataType, dataTypeSize, byteorder, getAsNative);
  return std::make_tuple(dataType, dataTypeSize, byteorder);
}

size_t vx::getElementSizeBytes(DataType type) {
  switch (type) {
    case DataType::Bool8:
    case DataType::UInt8:
    case DataType::Int8:
      return 1;
    case DataType::Float16:
    case DataType::UInt16:
    case DataType::Int16:
      return 2;
    case DataType::Float32:
    case DataType::UInt32:
    case DataType::Int32:
      return 4;
    case DataType::Float64:
    case DataType::UInt64:
    case DataType::Int64:
      return 8;
    default:
      throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                          "Invalid DataType");
  }
}
