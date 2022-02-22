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

#include "JsonUtil.hpp"

#include <VoxieClient/Exception.hpp>

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>

QJsonDocument vx::parseJsonData(const QByteArray& data,
                                const QString& filename) {
  QJsonParseError error;
  auto jsonDoc = QJsonDocument().fromJson(data, &error);
  if (jsonDoc.isNull()) {
    // Note: Offset seems to often (but not always) point one char *past* the
    // problematic point
    int line = 1;
    int col = 0;
    for (int i = 0; i < data.size() && i < error.offset; i++) {
      auto b = data[i];
      if (b == '\r') {
        line++;
        col = 0;
      } else if (b == '\n') {
        // '<CR> <LF>' goes only 1 line forward
        if (i == 0 || data[i - 1] != '\r') line++;
        col = 0;
      } else if ((uint8_t)b >= 0x80 && (uint8_t)b < 0xc0) {
        // Byte is binary 10xxxxxx, non-initial byte of UTF-8 char, ignore
        // https://en.wikipedia.org/w/index.php?title=UTF-8&oldid=965782675#Description
      } else {
        col++;
      }
    }
    // Note that col is in codepoints
    throw Exception("de.uni_stuttgart.Voxie.JsonParseError",
                    "Error parsing JSON file '" + filename + "': at line " +
                        QString::number(line) + ", column " +
                        QString::number(col) + ": " + error.errorString());
  }

  return jsonDoc;
}

QJsonDocument vx::parseJsonFile(const QString& filename) {
  QFile jsonFile(filename);
  jsonFile.open(QFile::ReadOnly);

  QByteArray data = jsonFile.readAll();

  if (jsonFile.error())
    // Note: QVariant(jsonFile.error()).toString()) does not seem to work in
    // Qt 5.11, but maybe it will work in the future
    throw Exception("de.uni_stuttgart.Voxie.JsonParseError",
                    "Error reading JSON file '" + filename +
                        "': " + QVariant(jsonFile.error()).toString());

  return parseJsonData(data, filename);
}

QString vx::jsonTypeToString(QJsonValue::Type type) {
  switch (type) {
    case QJsonValue::Null:
      return "null";
    case QJsonValue::Bool:
      return "bool";
    case QJsonValue::Double:
      return "double";
    case QJsonValue::String:
      return "string";
    case QJsonValue::Array:
      return "array";
    case QJsonValue::Object:
      return "object";
    case QJsonValue::Undefined:
      return "undefined";
    default:
      return QString::number(type);
  }
}

QString vx::expectString(const QJsonValue& value) {
  if (value.type() != QJsonValue::String)
    throw Exception("de.uni_stuttgart.Voxie.JsonError",
                    "Error in JSON data: Expected a string, got a " +
                        jsonTypeToString(value.type()));
  return value.toString();
}
QJsonObject vx::expectObject(const QJsonValue& value) {
  if (value.type() != QJsonValue::Object)
    throw Exception("de.uni_stuttgart.Voxie.JsonError",
                    "Error in JSON data: Expected an object, got a " +
                        jsonTypeToString(value.type()));
  return value.toObject();
}
QJsonArray vx::expectArray(const QJsonValue& value) {
  if (value.type() != QJsonValue::Array)
    throw Exception("de.uni_stuttgart.Voxie.JsonError",
                    "Error in JSON data: Expected an array, got a " +
                        jsonTypeToString(value.type()));
  return value.toArray();
}

QJsonObject vx::expectObject(const QJsonDocument& doc) {
  if (!doc.isObject())
    throw Exception("de.uni_stuttgart.Voxie.JsonError",
                    "Error in JSON data: Expected a document with an object");
  return doc.object();
}
QJsonArray vx::expectArray(const QJsonDocument& doc) {
  if (!doc.isArray())
    throw Exception("de.uni_stuttgart.Voxie.JsonError",
                    "Error in JSON data: Expected a document with an array");
  return doc.array();
}
