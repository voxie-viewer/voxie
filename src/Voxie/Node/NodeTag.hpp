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

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QSharedPointer>
#include <QtCore/QString>

namespace vx {
class VOXIECORESHARED_EXPORT NodeTag {
 private:
  QString description;
  QString displayName;
  QString name;

  NodeTag(QString pDescription, QString pDisplayName, QString pName);

  static QList<QSharedPointer<vx::NodeTag>> tags;
  static QSharedPointer<NodeTag> createObjectTag(QJsonObject jsonObj);

 public:
  QString getName() { return name; }
  QString getDisplayName() { return displayName; }
  QString getDescription() { return description; }

  static QSharedPointer<NodeTag> getTag(QString name);
  static bool exist(QString name);
  static void tagsFromJson(QString jsonFilename);
  static QString joinDisplayNames(QList<QSharedPointer<vx::NodeTag>> tagList,
                                  const QString& separator);
};
}  // namespace vx
