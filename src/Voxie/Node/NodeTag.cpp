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

#include "NodeTag.hpp"

#include <VoxieClient/Exception.hpp>

#include <VoxieBackend/Component/JsonUtil.hpp>

using namespace vx;

QList<QSharedPointer<vx::NodeTag>> NodeTag::tags = {};

NodeTag::NodeTag(QString pDescription, QString pDisplayName, QString pName) {
  this->description = pDescription;
  this->displayName = pDisplayName;
  this->name = pName;
}

QSharedPointer<NodeTag> NodeTag::createObjectTag(QJsonObject jsonObj) {
  QString desc = jsonObj["Description"].toString();
  QString dName = jsonObj["DisplayName"].toString();
  QString name = jsonObj["Name"].toString();
  if (NodeTag::exist(name)) {
    throw Exception("de.uni_stuttgart.Voxie.Error",
                    "Tag with name: " + name + " already exists");
  }
  QSharedPointer<NodeTag> tag =
      QSharedPointer<NodeTag>(new NodeTag(desc, dName, name));
  NodeTag::tags.append(tag);
  return tag;
}

bool NodeTag::exist(QString name) {
  for (QSharedPointer<NodeTag> tag : NodeTag::tags) {
    if (tag->name == name || tag->displayName == name) {
      return true;
    }
  }
  return false;
}

QSharedPointer<NodeTag> NodeTag::getTag(QString name) {
  for (QSharedPointer<NodeTag> tag : NodeTag::tags) {
    if (tag->name == name || tag->displayName == name) {
      return tag;
    }
  }
  return QSharedPointer<NodeTag>(nullptr);
}

void NodeTag::tagsFromJson(QString jsonFilename) {
  auto jsonDoc = parseJsonFile(jsonFilename);
  QJsonArray tagsArray = jsonDoc.object()["Tags"].toArray();
  for (const auto& tagJson : tagsArray) {
    QJsonObject tagObj = tagJson.toObject();
    if (!tagObj.contains("Description")) {
      throw Exception("de.uni_stuttgart.Voxie.Error",
                      "Tag must contain Description");
    }
    if (!tagObj.contains("DisplayName")) {
      throw Exception("de.uni_stuttgart.Voxie.Error",
                      "Tag must contain DisplayName");
    }
    if (!tagObj.contains("Name")) {
      throw Exception("de.uni_stuttgart.Voxie.Error", "Tag must contain Name");
    }
    NodeTag::createObjectTag(tagObj);
  }
}

QString NodeTag::joinDisplayNames(QList<QSharedPointer<vx::NodeTag>> tagList,
                                  const QString& separator) {
  QStringList nameList;
  for (QSharedPointer<vx::NodeTag> tag : tagList) {
    nameList << tag->displayName;
  }
  return nameList.join(separator);
}
