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

#include "Grid3D.hpp"

#include <QDebug>

#include <Voxie/Vis/OpenGLWidget.hpp>

Grid3D::Grid3D(QObject* isoVis)
    : QObject(isoVis),
      active(false),
      color(Qt::yellow),
      mode(GridSizeMode::Fixed),
      size(1),
      lengthUnit(SizeUnit::meter),
      opacity(128),
      xyPlane(false),
      xzPlane(false),
      yzPlane(false) {}

void Grid3D::setActive(bool active) {
  this->active = active;
  this->updateGrid();
}

void Grid3D::setColor(QColor color) {
  this->color = color;
  this->updateGrid();
}

void Grid3D::setMode(GridSizeMode mode) {
  this->mode = mode;
  this->updateGrid();
}

void Grid3D::setSizeForWidget(float size) {
  this->size = size;
  this->updateGrid();
}

void Grid3D::setGridSizeForSelf(float value) {
  this->size = value;
  Q_EMIT sizeChanged(value);
}

float Grid3D::getSize() { return this->size; }

void Grid3D::setUnitForWidget(SizeUnit unit) {
  this->lengthUnit = unit;
  this->updateGrid();
}

void Grid3D::setUnitForSelf(SizeUnit unit) {
  this->lengthUnit = unit;
  Q_EMIT unitChanged(unit);
}

void Grid3D::setOpacity(int opacity) {
  this->opacity = opacity;
  this->updateGrid();
}

void Grid3D::setXYPlane(bool checked) {
  this->xyPlane = checked;
  this->updateGrid();
}

void Grid3D::setXZPlane(bool checked) {
  this->xzPlane = checked;
  this->updateGrid();
}

void Grid3D::setYZPlane(bool checked) {
  this->yzPlane = checked;
  this->updateGrid();
}

void Grid3D::updateGrid() { Q_EMIT gridChanged(); }

void Grid3D::drawGrid(PrimitiveBuffer& drawingBuffer, const QVector3D& min,
                      const QVector3D& max, float prefLength) {
  if (!active) {
    return;
  }
  float drawLimit = 0.01f;

  float lowBoundrie = 0.3f;
  float upperBoundrie = fmin(fmin(max.x(), max.y()), max.z()) / 10;

  float enteredDistanceInMeter = this->size * unit();
  float imgDistance = enteredDistanceInMeter;  // TODO: what is this for? (value
                                               // is not used anywhere)
  (void)imgDistance;                           // suppress warning
  float meshWidthOnDisplay = enteredDistanceInMeter / prefLength;

  if (enteredDistanceInMeter == 0) {
    enteredDistanceInMeter = 1;
  } else {
    if (enteredDistanceInMeter < 0) {
      enteredDistanceInMeter = std::abs(enteredDistanceInMeter);
    }
  }

  // Grid mesh width is to small to see each mesh
  if (meshWidthOnDisplay < drawLimit) {
    imgDistance = drawLimit;
  }

  this->color.setAlpha(opacity);
  QVector4D colorVec = QVector4D(this->color.redF(), this->color.greenF(),
                                 this->color.blueF(), this->color.alphaF());

  if (this->mode != GridSizeMode::Automatic &&
      this->mode != GridSizeMode::Fixed) {
    qWarning() << "Grid has no valid mode";
  } else {
    if (this->mode == GridSizeMode::Automatic) {
      float biggerMeshWidthOnDisplay =
          ((this->size * 10) * unit()) / prefLength;
      float smallerMeshWidthOnDisplay =
          ((this->size / 10) * unit()) / prefLength;

      if (lowBoundrie > upperBoundrie) {
        qWarning() << "Visualizer Window is to small for Automatic Gird Mode";

      } else {
        while (meshWidthOnDisplay <= lowBoundrie &&
               biggerMeshWidthOnDisplay < upperBoundrie) {  // Unit is to small
          this->setBiggerUnit();
          enteredDistanceInMeter = this->size * unit();
          meshWidthOnDisplay = enteredDistanceInMeter / prefLength;
          biggerMeshWidthOnDisplay = ((this->size * 10) * unit()) / prefLength;
        }
        while (meshWidthOnDisplay >= upperBoundrie &&
               smallerMeshWidthOnDisplay > lowBoundrie) {  // Unit is to big
          this->setSmallerUnit();
          enteredDistanceInMeter = this->size * unit();
          meshWidthOnDisplay = enteredDistanceInMeter / prefLength;
          smallerMeshWidthOnDisplay = ((this->size / 10) * unit()) / prefLength;
        }
      }

      enteredDistanceInMeter = this->size * unit();
      imgDistance = enteredDistanceInMeter;
    }
  }

  if (xyPlane) {
    // Draw X lines:
    QVector3D xStart = QVector3D(0.0f, min.y(), 0.0f);
    QVector3D xEnd = QVector3D(0.0f, max.y(), 0.0f);

    for (float x = min.x(); x <= max.x(); x += enteredDistanceInMeter) {
      xStart.setX(x);
      xEnd.setX(x);
      drawingBuffer.addLine(colorVec, xStart, xEnd);
    }

    // Draw Y lines:
    QVector3D yStart = QVector3D(min.x(), 0.0f, 0.0f);
    QVector3D yEnd = QVector3D(max.x(), 0.0f, 0.0f);

    for (float y = min.y(); y <= max.y(); y += enteredDistanceInMeter) {
      yStart.setY(y);
      yEnd.setY(y);
      drawingBuffer.addLine(colorVec, yStart, yEnd);
    }
  }
  if (xzPlane) {
    // Draw X lines:
    QVector3D xStart = QVector3D(0.0f, 0.0f, min.z());
    QVector3D xEnd = QVector3D(0.0f, 0.0f, max.z());

    for (float x = min.x(); x <= max.x(); x += enteredDistanceInMeter) {
      xStart.setX(x);
      xEnd.setX(x);
      drawingBuffer.addLine(colorVec, xStart, xEnd);
    }

    // Draw Z lines:
    QVector3D zStart = QVector3D(min.x(), 0.0f, 0.0f);
    QVector3D zEnd = QVector3D(max.x(), 0.0f, 0.0f);

    for (float z = min.z(); z <= max.z(); z += enteredDistanceInMeter) {
      zStart.setZ(z);
      zEnd.setZ(z);
      drawingBuffer.addLine(colorVec, zStart, zEnd);
    }
  }
  if (yzPlane) {
    // Draw Y lines:
    QVector3D yStart = QVector3D(0.0f, 0.0f, min.z());
    QVector3D yEnd = QVector3D(0.0f, 0.0f, max.z());

    for (float y = min.y(); y <= max.y(); y += enteredDistanceInMeter) {
      yStart.setY(y);
      yEnd.setY(y);
      drawingBuffer.addLine(colorVec, yStart, yEnd);
    }

    // Draw Z lines:
    QVector3D zStart = QVector3D(0.0f, min.y(), 0.0f);
    QVector3D zEnd = QVector3D(0.0f, max.y(), 0.0f);

    for (float z = min.z(); z <= max.z(); z += enteredDistanceInMeter) {
      zStart.setZ(z);
      zEnd.setZ(z);
      drawingBuffer.addLine(colorVec, zStart, zEnd);
    }
  }
}

QString Grid3D::unitToString() {
  switch (this->lengthUnit) {
    case SizeUnit::meter:
      return "m";
    case SizeUnit::centimeter:
      return "cm";
    case SizeUnit::dezimeter:
      return "dm";
    case SizeUnit::millimeter:
      return "mm";
    case SizeUnit::mikrometer:
      return "µm";
    case SizeUnit::nanometer:
      return "nm";
    case SizeUnit::pikometer:
      return "pm";
    default:
      return "";
  }
}

void Grid3D::setBiggerUnit() {
  SizeUnit unit = this->lengthUnit;

  if (unit == SizeUnit::pikometer) {
    this->setGridSizeForSelf(size * 10);
    if (size >= 1000) {
      this->setGridSizeForSelf(size / 1000);
      this->setUnitForSelf(SizeUnit::nanometer);
    }
  } else {
    if (unit == SizeUnit::nanometer) {
      this->setGridSizeForSelf(size * 10);
      if (size >= 1000) {
        this->setGridSizeForSelf(size / 1000);
        this->setUnitForSelf(SizeUnit::mikrometer);
      }
    } else {
      if (unit == SizeUnit::mikrometer) {
        this->setGridSizeForSelf(size * 10);
        if (size >= 1000) {
          this->setGridSizeForSelf(size / 1000);
          this->setUnitForSelf(SizeUnit::millimeter);
        }
      } else {
        if (unit == SizeUnit::millimeter) {
          this->setUnitForSelf(SizeUnit::centimeter);
        } else {
          if (unit == SizeUnit::centimeter) {
            this->setUnitForSelf(SizeUnit::dezimeter);
          } else {
            if (unit == SizeUnit::dezimeter) {
              this->setUnitForSelf(SizeUnit::meter);
            } else {
              if (unit == SizeUnit::meter) {
                this->setGridSizeForSelf(size * 10);
              }
            }
          }
        }
      }
    }
  }
}

void Grid3D::setSmallerUnit() {
  SizeUnit unit = this->lengthUnit;

  if (unit == SizeUnit::meter) {
    this->setGridSizeForSelf(size / 10);
    if (size < 1) {
      this->setGridSizeForSelf(size * 1000);
      this->setUnitForSelf(SizeUnit::dezimeter);
    }
  } else {
    if (unit == SizeUnit::dezimeter) {
      this->setUnitForSelf(SizeUnit::centimeter);
    } else {
      if (unit == SizeUnit::centimeter) {
        this->setUnitForSelf(SizeUnit::millimeter);
      } else {
        if (unit == SizeUnit::millimeter) {
          this->setGridSizeForSelf(size / 10);
          if (size < 1) {
            this->setGridSizeForSelf(size * 1000);
            this->setUnitForSelf(SizeUnit::mikrometer);
          }
        } else {
          if (unit == SizeUnit::mikrometer) {
            this->setGridSizeForSelf(size / 10);
            if (size < 1) {
              this->setGridSizeForSelf(size * 1000);
              this->setUnitForSelf(SizeUnit::nanometer);
            }
          } else {
            if (unit == SizeUnit::nanometer) {
              this->setGridSizeForSelf(size / 10);
              if (size < 1) {
                this->setGridSizeForSelf(size * 1000);
                this->setUnitForSelf(SizeUnit::pikometer);
              }
            } else {
              if (unit == SizeUnit::pikometer) {
                this->setGridSizeForSelf(size / 10);
              }
            }
          }
        }
      }
    }
  }
}

float Grid3D::unit() {
  switch (this->lengthUnit) {
    case SizeUnit::pikometer:
      return 1E-12f;
    case SizeUnit::nanometer:
      return 1E-9f;
    case SizeUnit::mikrometer:
      return 1E-6f;
    case SizeUnit::millimeter:
      return 1E-3f;
    case SizeUnit::centimeter:
      return 1E-2f;
    case SizeUnit::dezimeter:
      return 1E-1f;
    case SizeUnit::meter:
      return 1;
    default:
      qWarning() << "No unit could be found for the grid.";
      return 1;
  }
}
