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

#include "VectorField.hpp"

#include <cmath>
#include <random>

#include <Voxie/IVoxie.hpp>
#include <Voxie/MathQt.hpp>

#include <VoxieBackend/Data/VolumeSeriesData.hpp>

#include <Voxie/Node/PropertyHelper.hpp>

#include <Voxie/Vis/OpenGLWidget.hpp>

#include <PluginVis3D/Prototypes.hpp>
#include <PluginVis3D/Visualizer3DView.hpp>

#include <Voxie/Data/VolumeSeriesNode.hpp>

#include <QtGui/QOpenGLFunctions>
#include <VoxieBackend/Data/VolumeDataVoxel.hpp>
#include <VoxieBackend/Data/VolumeDataVoxelInst.hpp>

VX_NODE_INSTANTIATION(vx::vis3d::VectorField)

using namespace vx;
using namespace vx::visualization;
using namespace vx::vis3d;

namespace {
class VectorFieldPerShareGroup : public Object3DPerShareGroup {
 public:
  QSharedPointer<Object3DRenderContextPerShareGroup> shareGroup;

  // QPointer<vx::vis3d::VectorField> VectorFieldObj;

  QOpenGLFunctions functions;

  // QSharedPointer<VectorFieldData3D> data;

  // QScopedPointer<Vis3DShaders> shaders;

  QSharedPointer<OpenGLDrawUtils> drawUtils;

  VectorFieldPerShareGroup(
      const QSharedPointer<Object3DRenderContextPerShareGroup>& shareGroup)
      : shareGroup(shareGroup) {
    functions.initializeOpenGLFunctions();

    drawUtils.reset(new OpenGLDrawUtils);
    drawUtils->initialize();

    // Initialize temporary shaders without attributes
    /*
    shaders.reset(
        new Vis3DShaders(&functions, QSharedPointer<vx::VectorFieldData>(),
                         "de.uni_stuttgart.Voxie.ShadingTechnique.Flat"));
    */
  }
};
}  // namespace

VectorField::VectorField()
    : Object3DNode(getPrototypeSingleton()),
      properties(new object3d_prop::VectorFieldProperties(this)) {
  QObject::connect(properties,
                   &object3d_prop::VectorFieldProperties::fieldChanged, this,
                   &VectorField::triggerRendering);
  QObject::connect(properties,
                   &object3d_prop::VectorFieldProperties::visibleChanged, this,
                   &VectorField::triggerRendering);

  // Update bounding box when VectorField changes
  QObject::connect(properties,
                   &object3d_prop::VectorFieldProperties::fieldChanged, this,
                   &VectorField::boundingBoxChanged);

  forwardSignalFromPropertyNode(
      properties, &object3d_prop::VectorFieldProperties::field,
      &object3d_prop::VectorFieldProperties::fieldChanged,
      &MovableDataNode::adjustedPositionChanged, this,
      &VectorField::triggerRendering);
  forwardSignalFromPropertyNode(
      properties, &object3d_prop::VectorFieldProperties::field,
      &object3d_prop::VectorFieldProperties::fieldChanged,
      &MovableDataNode::adjustedRotationChanged, this,
      &VectorField::triggerRendering);
  forwardSignalFromPropertyNode(
      properties, &object3d_prop::VectorFieldProperties::field,
      &object3d_prop::VectorFieldProperties::fieldChanged,
      &MovableDataNode::adjustedPositionChanged, this,
      &VectorField::boundingBoxChanged);
  forwardSignalFromPropertyNode(
      properties, &object3d_prop::VectorFieldProperties::field,
      &object3d_prop::VectorFieldProperties::fieldChanged,
      &MovableDataNode::adjustedRotationChanged, this,
      &VectorField::boundingBoxChanged);

  QObject::connect(properties,
                   &object3d_prop::VectorFieldProperties::scaleFactorChanged,
                   this, [this]() {
                     this->arrowsDirty = true;
                     this->triggerRendering();
                   });

  QObject::connect(properties,
                   &object3d_prop::VectorFieldProperties::stepChanged, this,
                   [this]() {
                     this->arrowsDirty = true;
                     this->triggerRendering();
                   });
  QObject::connect(properties,
                   &object3d_prop::VectorFieldProperties::colorModeChanged,
                   this, [this]() {
                     this->arrowsDirty = true;
                     this->triggerRendering();
                   });

  QObject::connect(properties,
                   &object3d_prop::VectorFieldProperties::fixedColorChanged,
                   this, [this]() {
                     this->arrowsDirty = true;
                     this->triggerRendering();
                   });

  QObject::connect(
      properties,
      &object3d_prop::VectorFieldProperties::magnitudeThresholdChanged, this,
      [this]() {
        this->arrowsDirty = true;
        this->triggerRendering();
      });
  QObject::connect(properties,
                   &object3d_prop::VectorFieldProperties::selectGridChanged,
                   this, [this]() {
                     this->arrowsDirty = true;
                     this->triggerRendering();
                   });
}
VectorField::~VectorField() {}

QSharedPointer<Object3DPerShareGroup> VectorField::newPerShareGroup(
    const QSharedPointer<Object3DRenderContextPerShareGroup>& shareGroup) {
  // qDebug() << "VectorField::newPerShareGroup()";
  QSharedPointer<VectorFieldPerShareGroup> ptr(
      new VectorFieldPerShareGroup(shareGroup),
      [](QObject* obj) { obj->deleteLater(); });

  return ptr;
}

static void addArrowToBuffer(OpenGLDrawUtils::PrimitiveBuffer& buffer,
                             const QVector3D& start, const QVector3D& direction,
                             float length, const vx::Color& color,
                             // draw the circle using 20 segments
                             int segments = 6) {
  QVector3D dir = direction.normalized();  // normalized direction(length 1)
  // body of arrow has 70% of the total dimension of the arrow
  // cone of the arrow is 30% of the arrows length
  // last two are thickness
  float shaftLength = length * 0.7f;
  float coneLength = length * 0.3f;
  float shaftRadius = length * 0.02f;
  float coneRadius = length * 0.04f;

  QVector3D shaftEnd = start + dir * shaftLength;
  QVector3D arrowEnd = shaftEnd + dir * coneLength;

  // //build system for direction
  QVector3D up = std::abs(QVector3D::dotProduct(dir, QVector3D(0, 1, 0))) < 0.99
                     ? QVector3D(0, 1, 0)
                     : QVector3D(1, 0, 0);
  QVector3D right = QVector3D::crossProduct(dir, up).normalized();
  QVector3D forward = QVector3D::crossProduct(right, dir).normalized();

  // draw body of arrow
  for (int i = 0; i < segments; ++i) {
    float angle1 = 2 * M_PI * i / segments;
    float angle2 = 2 * M_PI * (i + 1) / segments;

    QVector3D offset1 = right * std::cos(angle1) * shaftRadius +
                        forward * std::sin(angle1) * shaftRadius;
    QVector3D offset2 = right * std::cos(angle2) * shaftRadius +
                        forward * std::sin(angle2) * shaftRadius;

    QVector3D p1 = start + offset1;
    QVector3D p2 = shaftEnd + offset1;
    QVector3D p3 = shaftEnd + offset2;
    QVector3D p4 = start + offset2;

    buffer.addQuad(color, p1, p2, p3, p4);
    // buffer.addQuad(color, p4, p3, p2, p1);
  }

  QVector3D centerStart = start;
  for (int i = 0; i < segments; ++i) {
    float angle1 = 2 * M_PI * i / segments;
    float angle2 = 2 * M_PI * (i + 1) / segments;

    QVector3D offset1 = right * std::cos(angle1) * shaftRadius +
                        forward * std::sin(angle1) * shaftRadius;
    QVector3D offset2 = right * std::cos(angle2) * shaftRadius +
                        forward * std::sin(angle2) * shaftRadius;

    buffer.addTriangle(color, centerStart, start + offset1, start + offset2);
  }

  // draw tip of the arrow
  for (int i = 0; i < segments; ++i) {
    float angle1 = 2 * M_PI * i / segments;
    float angle2 = 2 * M_PI * (i + 1) / segments;

    QVector3D offset1 = right * std::cos(angle1) * coneRadius +
                        forward * std::sin(angle1) * coneRadius;
    QVector3D offset2 = right * std::cos(angle2) * coneRadius +
                        forward * std::sin(angle2) * coneRadius;

    QVector3D base1 = shaftEnd + offset1;
    QVector3D base2 = shaftEnd + offset2;

    // buffer.addTriangle(color, arrowEnd, base1, base2);
    buffer.addTriangle(color, arrowEnd, base2, base1);
  }

  QVector3D centerBase = shaftEnd;
  for (int i = 0; i < segments; ++i) {
    float angle1 = 2 * M_PI * i / segments;
    float angle2 = 2 * M_PI * (i + 1) / segments;

    QVector3D offset1 = right * std::cos(angle1) * coneRadius +
                        forward * std::sin(angle1) * coneRadius;
    QVector3D offset2 = right * std::cos(angle2) * coneRadius +
                        forward * std::sin(angle2) * coneRadius;

    buffer.addTriangle(color, centerBase, shaftEnd + offset1,
                       shaftEnd + offset2);
  }
}

static QVector3D interpolateVectorAtPosition(
    float x, float y, float z, QSharedPointer<vx::VolumeDataVoxel> dataX,
    QSharedPointer<vx::VolumeDataVoxel> dataY,
    QSharedPointer<vx::VolumeDataVoxel> dataZ) {
  int x0 = static_cast<int>(std::floor(x));
  int y0 = static_cast<int>(std::floor(y));
  int z0 = static_cast<int>(std::floor(z));
  int x1 = x0 + 1;
  int y1 = y0 + 1;
  int z1 = z0 + 1;

  auto getVal = [&](QSharedPointer<vx::VolumeDataVoxel> data, int xi, int yi,
                    int zi) {
    return data->performInGenericContext([&](auto& d) {
      return static_cast<float>(d.getVoxelSafe(xi, yi, zi));
    });
  };

  float xd = x - x0;
  float yd = y - y0;
  float zd = z - z0;

  auto interpolate = [&](QSharedPointer<vx::VolumeDataVoxel> data) {
    float c000 = getVal(data, x0, y0, z0);
    float c100 = getVal(data, x1, y0, z0);
    float c010 = getVal(data, x0, y1, z0);
    float c110 = getVal(data, x1, y1, z0);
    float c001 = getVal(data, x0, y0, z1);
    float c101 = getVal(data, x1, y0, z1);
    float c011 = getVal(data, x0, y1, z1);
    float c111 = getVal(data, x1, y1, z1);

    float c00 = c000 * (1 - xd) + c100 * xd;
    float c01 = c001 * (1 - xd) + c101 * xd;
    float c10 = c010 * (1 - xd) + c110 * xd;
    float c11 = c011 * (1 - xd) + c111 * xd;

    float c0 = c00 * (1 - yd) + c10 * yd;
    float c1 = c01 * (1 - yd) + c11 * yd;

    return c0 * (1 - zd) + c1 * zd;
  };

  float vx = interpolate(dataX);
  float vy = interpolate(dataY);
  float vz = interpolate(dataZ);

  return QVector3D(vx, vy, vz);
}

void VectorField::render(
    const QSharedPointer<Object3DPerShareGroup>& perShareGroup,
    const QSharedPointer<Object3DPerContext>& perContext,
    Object3DRenderContext& context) {
  Q_UNUSED(perContext);
  // qDebug()

  if (!properties->visible()) return;

  auto volumeSeries =
      dynamic_cast<VolumeSeriesNode*>(this->properties->field());
  if (!volumeSeries) {
    qDebug() << "...";
    return;
  }
  qDebug() << "Got volume series" << volumeSeries;

  auto dataSeries =
      qSharedPointerDynamicCast<VolumeSeriesData>(volumeSeries->data());
  if (!dataSeries || dataSeries->dimensionCount() != 1) {
    qDebug() << "Did not get data with one dimension";
    return;
  }
  auto dimension = dataSeries->dimensions()[0];
  qDebug() << "Got data with one dimension with size"
           << dimension->entries().size();

  if (dimension->entries().size() != 3) {
    qDebug() << "Did not get exactly 3 values";
    return;
  }
  auto dataX =
      qSharedPointerDynamicCast<VolumeDataVoxel>(dataSeries->lookupEntry({0}));
  auto dataY =
      qSharedPointerDynamicCast<VolumeDataVoxel>(dataSeries->lookupEntry({1}));
  auto dataZ =
      qSharedPointerDynamicCast<VolumeDataVoxel>(dataSeries->lookupEntry({2}));
  qDebug() << "Got data" << dataX << dataY << dataZ;
  if (!dataX || !dataY || !dataZ) {
    qDebug() << "Got non-voxel data";
    return;
  }

  qDebug() << "Number of voxels for X:" << dataX->arrayShape();
  qDebug() << "Voxel size for X:" << dataX->gridSpacing();
  qDebug() << "First voxel value X:"
           << dataX->performInGenericContext([](auto& data) {
                return static_cast<float>(data.getVoxelSafe(0, 0, 0));
              });

  auto psg = qSharedPointerDynamicCast<VectorFieldPerShareGroup>(perShareGroup);
  if (!psg)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Not a VectorFieldPerShareGroup");

  QMatrix4x4 matViewProj = context.projectionMatrix() * context.viewMatrix();

  OpenGLDrawUtils::PrimitiveBuffer drawingBuffer;

  vx::Vector<double, 3> spacingD = dataX->gridSpacing();
  vx::Vector<double, 3> originD = dataX->volumeOrigin();
  QVector3D spacing(spacingD[0], spacingD[1], spacingD[2]);
  QVector3D origin(originD[0], originD[1], originD[2]);

  int step = properties->step();
  int sizeX = static_cast<int>(dataX->arrayShape()[0]);
  int sizeY = static_cast<int>(dataX->arrayShape()[1]);
  int sizeZ = static_cast<int>(dataX->arrayShape()[2]);

  if (this->arrowsDirty) {
    this->cachedArrows.clear();

    float threshold = properties->magnitudeThreshold();
    float scaleFactor = properties->scaleFactor();
    QString colorMode = properties->colorMode();
    QString selectGrid = properties->selectGrid();
    vx::Color color;

    if (selectGrid == "Regular") {
      for (int bz = 0; bz < sizeZ; bz += step) {
        for (int by = 0; by < sizeY; by += step) {
          for (int bx = 0; bx < sizeX; bx += step) {
            float centerX = bx + step / 2.0f;
            float centerY = by + step / 2.0f;
            float centerZ = bz + step / 2.0f;

            QVector3D avgVec = interpolateVectorAtPosition(
                centerX, centerY, centerZ, dataX, dataY, dataZ);

            if (avgVec.length() > threshold) {
              QVector3D dirNorm = avgVec.normalized();
              float magnitude = avgVec.length();
              float normMag = std::log1p(magnitude) / std::log1p(0.6f);
              normMag = std::min(normMag, 1.0f);
              float scaledLength = magnitude * scaleFactor;

              if (colorMode == "Fixed") {
                color = properties->fixedColor();
              } else {
                color = vx::Color(1.0f - normMag, normMag, 0.0f);
              }

              QVector3D pos = origin + QVector3D((bx + step / 2) * spacing.x(),
                                                 (by + step / 2) * spacing.y(),
                                                 (bz + step / 2) * spacing.z());

              this->cachedArrows.push_back({pos, dirNorm, scaledLength, color});
            }
          }
        }
      }
    } else if (selectGrid == "Unstructured") {
      std::uniform_int_distribution<int> distribution(0, step - 1);

      for (int bz = 0; bz < sizeZ; bz += step) {
        for (int by = 0; by < sizeY; by += step) {
          for (int bx = 0; bx < sizeX; bx += step) {
            // deterministic number generator
            std::seed_seq seed{bx, by, bz};
            std::mt19937 generator(seed);

            int offsetX = distribution(generator);
            int offsetY = distribution(generator);
            int offsetZ = distribution(generator);

            float sampleX = bx + offsetX;
            float sampleY = by + offsetY;
            float sampleZ = bz + offsetZ;

            QVector3D interpolatedVec = interpolateVectorAtPosition(
                sampleX, sampleY, sampleZ, dataX, dataY, dataZ);

            if (interpolatedVec.length() > threshold) {
              QVector3D dirNorm = interpolatedVec.normalized();
              float magnitude = interpolatedVec.length();
              float normMag = std::log1p(magnitude) / std::log1p(0.6f);
              normMag = std::min(normMag, 1.0f);
              float scaledLength = magnitude * scaleFactor;

              if (colorMode == "Fixed") {
                color = properties->fixedColor();
              } else {
                color = vx::Color(1.0f - normMag, normMag, 0.0f);
              }

              QVector3D pos = origin + QVector3D((bx + offsetX) * spacing.x(),
                                                 (by + offsetY) * spacing.y(),
                                                 (bz + offsetZ) * spacing.z());

              this->cachedArrows.push_back({pos, dirNorm, scaledLength, color});
            }
          }
        }
      }
    }

    this->arrowsDirty = false;
  }
  for (const auto& arrow : this->cachedArrows) {
    addArrowToBuffer(drawingBuffer, arrow.pos, arrow.dir, arrow.length,
                     arrow.color);
  }

  QMatrix4x4 modelMatrix;
  modelMatrix.setToIdentity();

  psg->functions.glEnable(GL_DEPTH_TEST);

  psg->functions.glEnable(GL_CULL_FACE);
  psg->functions.glCullFace(GL_BACK);
  psg->functions.glFrontFace(GL_CCW);
  // QVector4D lightPos(2.0f, 2.0f, 2.0f, 1.0f);
  QVector4D lightPosition = context.lightPosition();
  psg->drawUtils->draw(drawingBuffer, modelMatrix, matViewProj, &lightPosition);
}

BoundingBox3D VectorField::getBoundingBox() {
  auto volumeSeries =
      dynamic_cast<VolumeSeriesNode*>(this->properties->field());
  if (!volumeSeries) {
    qDebug() << "Bounding box: No volume series";
    return BoundingBox3D::empty();
  }

  auto dataSeries =
      qSharedPointerDynamicCast<VolumeSeriesData>(volumeSeries->data());
  if (!dataSeries || dataSeries->dimensionCount() != 1) {
    qDebug() << "Bounding box: Did not get data with one dimension";
    return BoundingBox3D::empty();
  }
  auto dimension = dataSeries->dimensions()[0];

  if (dimension->entries().size() != 3) {
    qDebug() << "Bounding box: Did not get exactly 3 values";
    return BoundingBox3D::empty();
  }
  auto dataX =
      qSharedPointerDynamicCast<VolumeData>(dataSeries->lookupEntry({0}));
  if (!dataX) {
    qDebug() << "Bounding box: Got non-volume data";
    return BoundingBox3D::empty();
  }
  return BoundingBox3D::point(dataX->volumeOrigin()) +
         BoundingBox3D::point(dataX->volumeOrigin() + dataX->volumeSize());
}
