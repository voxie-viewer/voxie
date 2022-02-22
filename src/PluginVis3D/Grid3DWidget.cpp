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

#include "Grid3DWidget.hpp"
#include "ui_Grid3DWidget.h"

#include <QColorDialog>
#include <QDebug>

Grid3DWidget::Grid3DWidget(QWidget* parent, Grid3D* grid)
    : QWidget(parent), ui(new Ui::Grid3DWidget), grid(grid) {
  ui->setupUi(this);

  QWidget* opacityPlaneWid = this->findChild<QWidget*>("opacityAndPlane");
  QWidget* colorWid = this->findChild<QWidget*>("colorLabelAndButton_2");
  QWidget* meshWidthWid = this->findChild<QWidget*>("meshWidth");

  opacityPlaneWid->setEnabled(false);
  colorWid->setEnabled(false);
  meshWidthWid->setEnabled(false);

  opacityPlaneWid->update();
  colorWid->update();
  meshWidthWid->update();

  // Called when the grid itself changes the unit.
  connect(this->grid, &Grid3D::unitChanged, this,
          [this] { this->updateLineEdit(); });

  // Called when the grid itself changes the size.
  connect(this->grid, &Grid3D::sizeChanged, this,
          [this] { this->updateLineEdit(); });
}

Grid3DWidget::~Grid3DWidget() { delete ui; }

void Grid3DWidget::on_checkBox_clicked(bool checked) {
  QWidget* opacityPlaneWid = this->findChild<QWidget*>("opacityAndPlane");
  QWidget* colorWid = this->findChild<QWidget*>("colorLabelAndButton_2");
  QWidget* meshWidthWid = this->findChild<QWidget*>("meshWidth");

  if (checked) {
    opacityPlaneWid->setEnabled(true);
    colorWid->setEnabled(true);
    meshWidthWid->setEnabled(true);
  } else {
    opacityPlaneWid->setEnabled(false);
    colorWid->setEnabled(false);
    meshWidthWid->setEnabled(false);
  }
  opacityPlaneWid->update();
  colorWid->update();
  meshWidthWid->update();

  grid->setActive(checked);
}

void Grid3DWidget::on_colorButton_clicked() {
  QPushButton* clButt = this->findChild<QPushButton*>("colorButton");

  QColor color =
      QColorDialog::getColor(clButt->palette().window().color(), this);
  if (color.isValid()) {
    QString clStr = "background-color:";
    clStr.operator+=(color.name());
    clStr.operator+=(" ; border: 1px solid black; ");
    clButt->setStyleSheet(clStr);
    clButt->update();
  }
  grid->setColor(color);
}

void Grid3DWidget::on_comboBox_currentTextChanged(const QString& mode) {
  if (mode == "Automatic") {
    grid->setMode(GridSizeMode::Automatic);
  } else {
    if (mode == "Fixed") {
      grid->setMode(GridSizeMode::Fixed);
    }
  }
}

void Grid3DWidget::on_gridMeshWidth_editingFinished() {
  QRegularExpression regExpText(
      "^(\\d+(?:\\.\\d+)?)(?:\\s*)(pm|nm|um|µm|mm|cm|dm|m|Vx)$");
  QRegularExpressionMatch match;

  QString text = this->findChild<QLineEdit*>("gridMeshWidth")->text();
  text = text.trimmed();

  match = regExpText.match(text);

  if (!match.hasMatch()) {
    qWarning() << "wrong Syntax in Grid size Input";
    this->updateLineEdit();
  } else {
    QStringList matchList = match.capturedTexts();

    if (matchList.size() == 3) {
      this->grid->setSizeForWidget(((QString)matchList.at(1)).toFloat());
      stringToUnit((QString)matchList.at(2));
    }
  }
}

void Grid3DWidget::on_opacitySlider_valueChanged(int value) {
  grid->setOpacity(value);
}

void Grid3DWidget::on_XY_clicked(bool checked) { grid->setXYPlane(checked); }

void Grid3DWidget::on_XZ_clicked(bool checked) { grid->setXZPlane(checked); }

void Grid3DWidget::on_YZ_clicked(bool checked) { grid->setYZPlane(checked); }

void Grid3DWidget::stringToUnit(const QString& unit) {
  if (unit == "pm") {
    this->grid->setUnitForWidget(SizeUnit::pikometer);
  } else {
    if (unit == "nm") {
      this->grid->setUnitForWidget(SizeUnit::nanometer);
    } else {
      if (unit == "um" || unit == "µm") {
        this->grid->setUnitForWidget(SizeUnit::mikrometer);
      } else {
        if (unit == "mm") {
          this->grid->setUnitForWidget(SizeUnit::millimeter);
        } else {
          if (unit == "cm") {
            this->grid->setUnitForWidget(SizeUnit::centimeter);
          } else {
            if (unit == "dm") {
              this->grid->setUnitForWidget(SizeUnit::dezimeter);
            } else {
              if (unit == "m") {
                this->grid->setUnitForWidget(SizeUnit::meter);
              } else {
                if (unit == "Vx") {
                  this->grid->setUnitForWidget(SizeUnit::voxel);
                }
              }
            }
          }
        }
      }
    }
  }
}

void Grid3DWidget::updateLineEdit() {
  QLineEdit* unit = this->findChild<QLineEdit*>("gridMeshWidth");
  float size = this->grid->getSize();
  QString unitText = "";
  unitText = QString::number(size) + " " + this->grid->unitToString();
  unit->setText(unitText);
}
