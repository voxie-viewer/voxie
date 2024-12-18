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

#include "InfoWidget.hpp"

#include <VoxieClient/Format.hpp>

#include <Voxie/Data/TableData.hpp>
#include <Voxie/Data/TableNode.hpp>

#include <PluginVisSlice/SliceVisualizer.hpp>

#include <QtWidgets/QLabel>

// TODO: Also update info when the slice and/or the label object changes

InfoWidget::InfoWidget(SliceVisualizer* sv, QWidget* parent)
    : QWidget(parent), sv(sv) {
  this->layout = new QVBoxLayout();
  this->layout->setContentsMargins(0, 0, 0, 0);
  this->setLayout(layout);

  // this->labelPos = new QLabel("");
  // this->layout->addWidget(this->labelPos);

  auto labelPosMouse = new QLabel("");
  this->layout->addWidget(labelPosMouse);

  auto labelPosPlane = new QLabel("");
  this->layout->addWidget(labelPosPlane);

  auto labelPos3D = new QLabel("");
  this->layout->addWidget(labelPos3D);

  auto labelPosVoxel = new QLabel("");
  this->layout->addWidget(labelPosVoxel);

  auto labelVal = new QLabel("");
  this->layout->addWidget(labelVal);

  auto labelTable = new QLabel("");
  this->layout->addWidget(labelTable);

  QObject::connect(
      sv, &SliceVisualizer::imageMouseMove, this,
      [=](QMouseEvent* e, const vx::Vector<double, 2>& planePos,
          const vx::Vector<double, 3>& pos3D,
          const vx::Vector<double, 3>* posVoxelPtr, double valNearest,
          double valLinear) {
        labelPosMouse->setText(
            vx::format("Mouse position: {} {}", e->pos().x(), e->pos().y()));
        labelPosPlane->setText(vx::format("Plane position: {:.6f} {:.6f}",
                                          planePos.access<0>(),
                                          planePos.access<1>()));
        labelPos3D->setText(vx::format("3D position: {:.6f} {:.6f} {:.6f} m",
                                       pos3D.access<0>(), pos3D.access<1>(),
                                       pos3D.access<2>()));
        if (posVoxelPtr)
          labelPosVoxel->setText(vx::format(
              "Position: {:.2f} {:.2f} {:.2f} vx", posVoxelPtr->access<0>(),
              posVoxelPtr->access<1>(), posVoxelPtr->access<2>()));
        else
          labelPosVoxel->setText(vx::format("Position: - - - vx"));
        labelVal->setText(
            vx::format("Value (nearest): {:.6f}\nValue (trilinear): {:.6f}",
                       valNearest, valLinear));

        QString tableText = "Label info:";
        auto table = dynamic_cast<vx::TableNode*>(sv->properties->infoTable());
        if (table) {
          auto data = table->tableData();
          if (data) {
            const auto& columns = data->columns();
            int col = -1;
            for (int i = 0; i < columns.size(); i++) {
              const auto& column = columns[i];
              if (column.name() == "LabelID") {
                col = i;
                break;
              }
            }
            if (col != -1) {
              // TODO: performance
              auto rows = data->getRowsByIndex();
              for (const auto& row : rows) {
                // if (row.data()[col] == valUnf) {
                if (row.data()[col] == valNearest) {
                  tableText += " ";
                  for (int i = 0; i < columns.size(); i++) {
                    if (i != 0) tableText += " / ";
                    tableText += row.data()[i].toString();
                  }
                  break;
                }
              }
            }
          }
        }
        labelTable->setText(tableText);
      });
}

InfoWidget::~InfoWidget() {}
