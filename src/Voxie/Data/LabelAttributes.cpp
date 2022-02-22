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

#include "LabelAttributes.hpp"

#include <Voxie/Data/TableData.hpp>

using namespace vx;

LabelAttributes::LabelAttributes(int labelID, int numberOfVoxel,
                                 QList<std::tuple<int, int>> boundingBox,
                                 std::tuple<int, int, int> centerOfMass,
                                 std::tuple<int, int, int> weightedCenterOfMass,
                                 double sumOfValues, double average) {
  this->labelID = labelID;
  this->numberOfVoxel = numberOfVoxel;
  this->boundingBox = boundingBox;
  this->centerOfMass = centerOfMass;
  this->weightedCenterOfMass = weightedCenterOfMass;
  this->sumOfValues = sumOfValues;
  this->average = average;
}

QStringList LabelAttributes::getAttributeTitles() {
  QStringList returnList;
  returnList << "Label ID"
             << "Number of voxel"
             << "Bounding box"
             << "Center of mass"
             << "Weighted center of mass"
             << "Sum of values"
             << "Average";
  return returnList;
}

double LabelAttributes::getValues(QString attributeName) {
  if (attributeName == "Label ID") {
    return this->labelID;
  }
  if (attributeName == "Number of voxel") {
    return double(this->numberOfVoxel);
  }
  if (attributeName == "Sum of values") {
    return double(this->sumOfValues);
  }
  if (attributeName == "Average") {
    return double(this->average);
  }
  return -1;
}

double LabelAttributes::getValues(const QList<TableColumn>& columns,
                                  const TableRow& row, QString attributeName) {
  QString intName = "";
  if (attributeName == "Label ID") intName = "LabelID";
  if (attributeName == "Number of voxel") intName = "NumberOfVoxels";
  if (attributeName == "Sum of values") intName = "SumOfValues";
  if (attributeName == "Average") intName = "Average";

  if (intName == "") return -1;

  int column = -1;
  for (int i = 0; i < columns.size(); i++) {
    if (columns[i].name() == intName) {
      column = i;
      break;
    }
  }
  if (column == -1) return -1;

  if (column >= row.data().size()) {
    qCritical() << "column >= row.data().size()";
    return -1;
  }
  const auto& val = row.data()[column];
  return val.toDouble();  // TODO
}

QStringList LabelAttributes::getAttributes() {
  QStringList returnList;
  returnList
      << QString::number(this->labelID) << QString::number(this->numberOfVoxel)
      << "([" + QString::number(std::get<0>(this->boundingBox[0])) + "/" +
             QString::number(std::get<1>(this->boundingBox[0])) + "] |" + " [" +
             QString::number(std::get<0>(this->boundingBox[1])) + "/" +
             QString::number(std::get<1>(this->boundingBox[1])) + "] |" + " [" +
             QString::number(std::get<0>(this->boundingBox[2])) + "/" +
             QString::number(std::get<1>(this->boundingBox[2])) + "])"
      << "(" + QString::number(std::get<0>(this->centerOfMass)) + "/" +
             QString::number(std::get<1>(this->centerOfMass)) + "/" +
             QString::number(std::get<2>(this->centerOfMass)) + ")"
      << "(" + QString::number(std::get<0>(this->weightedCenterOfMass)) + "/" +
             QString::number(std::get<1>(this->weightedCenterOfMass)) + "/" +
             QString::number(std::get<2>(this->weightedCenterOfMass)) + ")"
      << QString::number(this->sumOfValues) << QString::number(this->average);
  return returnList;
}
