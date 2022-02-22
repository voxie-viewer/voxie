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

#include <Voxie/Node/DataNode.hpp>

#include <QStringList>

namespace vx {
class TableColumn;
class TableRow;

// TODO: remove

/**
 * @brief The LabelAttributes class stored the Attributes from the CCA for one
 * Label.
 */
class VOXIECORESHARED_EXPORT LabelAttributes {
 public:
  /**
   * @brief LabelAttributes initialize the following Attributes:
   * @param labelID is the ID for the Label, that is defined by the CCL
   * @param numberOfVoxel is the number of Voxel within the label
   * @param boundingBox lowest and highest x, y and z coordinate, that the label
   * contains
   * @param centerOfMass of a label
   * @param weightedCenterOfMass of a label
   * @param sumOfValues is the sum of the original values within the label.
   * @param average is the average of the original values within the label.
   */
  LabelAttributes(int labelID, int numberOfVoxel,
                  QList<std::tuple<int, int>> boundingBox,
                  std::tuple<int, int, int> centerOfMass,
                  std::tuple<int, int, int> weightedCenterOfMass =
                      std::tuple<int, int, int>(-1, -1, -1),
                  double sumOfValues = -1, double average = -1);
  /**
   * @brief getAttributeTitles definies the titel of the attributes for the
   * tabel header.
   * @return a StringList with the titels for the header of the tabelview.
   */
  static QStringList getAttributeTitles();
  /**
   * @brief getAttributes convert the values to a String and put all into one
   * list.
   * @return the attributes as a list of Strings
   */
  QStringList getAttributes();

  /**
   * @brief getValues @return the value debending on @param attributeName
   */
  double getValues(QString attributeName);

  static double getValues(const QList<TableColumn>& columns,
                          const TableRow& row, QString attributeName);

  int labelID;

 private:
  int numberOfVoxel;
  QList<std::tuple<int, int>> boundingBox;
  std::tuple<int, int, int> centerOfMass;
  std::tuple<int, int, int> weightedCenterOfMass;
  double sumOfValues;
  double average;
};
}  // namespace vx
