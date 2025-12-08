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

#include "LineVisualizer.hpp"

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
#include <QtGui/QOpenGLFunctions_3_3_Core>
#include <VoxieBackend/Data/VolumeDataVoxel.hpp>
#include <VoxieBackend/Data/VolumeDataVoxelInst.hpp>

VX_NODE_INSTANTIATION(vx::vis3d::LineVisualizer)

using namespace vx;
using namespace vx::visualization;
using namespace vx::vis3d;

namespace {
class LineVisualizerPerShareGroup : public Object3DPerShareGroup {
 public:
  QSharedPointer<Object3DRenderContextPerShareGroup> shareGroup;

  // QPointer<vx::vis3d::LineVisualizer> LineVisualizerObj;

  QOpenGLFunctions functions;

  // QSharedPointer<LineVisualizerData3D> data;

  QScopedPointer<Vis3DShaders> shaders;

  QSharedPointer<OpenGLDrawUtils> drawUtils;

  LineVisualizerPerShareGroup(
      const QSharedPointer<Object3DRenderContextPerShareGroup>& shareGroup)
      : shareGroup(shareGroup) {
    functions.initializeOpenGLFunctions();

    drawUtils.reset(new OpenGLDrawUtils);
    drawUtils->initialize();
    shaders.reset(
        new Vis3DShaders(&functions, QSharedPointer<vx::SurfaceData>(),
                         "de.uni_stuttgart.Voxie.ShadingTechnique.Flat"));

    // Initialize temporary shaders without attributes
    /*
    shaders.reset(
        new Vis3DShaders(&functions, QSharedPointer<vx::LineVisualizerData>(),
                         "de.uni_stuttgart.Voxie.ShadingTechnique.Flat"));
    */
  }
};
}  // namespace

LineVisualizer::LineVisualizer()
    : Object3DNode(getPrototypeSingleton()),
      properties(new object3d_prop::LineVisualizerProperties(this)) {
  QObject::connect(properties,
                   &object3d_prop::LineVisualizerProperties::fieldChanged, this,
                   &LineVisualizer::triggerRendering);
  QObject::connect(properties,
                   &object3d_prop::LineVisualizerProperties::visibleChanged,
                   this, &LineVisualizer::triggerRendering);

  // Update bounding box when LineVisualizer changes
  QObject::connect(properties,
                   &object3d_prop::LineVisualizerProperties::fieldChanged, this,
                   &LineVisualizer::boundingBoxChanged);

  forwardSignalFromPropertyNode(
      properties, &object3d_prop::LineVisualizerProperties::field,
      &object3d_prop::LineVisualizerProperties::fieldChanged,
      &MovableDataNode::adjustedPositionChanged, this,
      &LineVisualizer::triggerRendering);
  forwardSignalFromPropertyNode(
      properties, &object3d_prop::LineVisualizerProperties::field,
      &object3d_prop::LineVisualizerProperties::fieldChanged,
      &MovableDataNode::adjustedRotationChanged, this,
      &LineVisualizer::triggerRendering);
  forwardSignalFromPropertyNode(
      properties, &object3d_prop::LineVisualizerProperties::field,
      &object3d_prop::LineVisualizerProperties::fieldChanged,
      &MovableDataNode::adjustedPositionChanged, this,
      &LineVisualizer::boundingBoxChanged);
  forwardSignalFromPropertyNode(
      properties, &object3d_prop::LineVisualizerProperties::field,
      &object3d_prop::LineVisualizerProperties::fieldChanged,
      &MovableDataNode::adjustedRotationChanged, this,
      &LineVisualizer::boundingBoxChanged);

  QObject::connect(properties,
                   &object3d_prop::LineVisualizerProperties::scaleFactorChanged,
                   this, [this]() {
                     this->streamlinesDirty = true;
                     this->triggerRendering();
                   });

  QObject::connect(properties,
                   &object3d_prop::LineVisualizerProperties::stepChanged, this,
                   [this]() {
                     this->streamlinesDirty = true;
                     this->triggerRendering();
                   });
  QObject::connect(properties,
                   &object3d_prop::LineVisualizerProperties::colorModeChanged,
                   this, [this]() { this->triggerRendering(); });

  QObject::connect(properties,
                   &object3d_prop::LineVisualizerProperties::fixedColorChanged,
                   this, [this]() {
                     this->streamlinesDirty = true;
                     this->triggerRendering();
                   });

  QObject::connect(
      properties,
      &object3d_prop::LineVisualizerProperties::magnitudeThresholdChanged, this,
      [this]() {
        this->streamlinesDirty = true;
        this->triggerRendering();
      });

  QObject::connect(properties,
                   &object3d_prop::LineVisualizerProperties::numStepsChanged,
                   this, [this]() {
                     this->streamlinesDirty = true;
                     this->triggerRendering();
                   });

  QObject::connect(
      properties,
      &object3d_prop::LineVisualizerProperties::maxStreamlinesChanged, this,
      [this]() {
        this->streamlinesDirty = true;
        this->triggerRendering();
      });

  QObject::connect(properties,
                   &object3d_prop::LineVisualizerProperties::selectGridChanged,
                   this, [this]() {
                     this->streamlinesDirty = true;
                     this->triggerRendering();
                   });
  QObject::connect(properties,
                   &object3d_prop::LineVisualizerProperties::integratorChanged,
                   this, [this]() {
                     this->streamlinesDirty = true;
                     this->triggerRendering();
                   });
}
LineVisualizer::~LineVisualizer() {}

QSharedPointer<Object3DPerShareGroup> LineVisualizer::newPerShareGroup(
    const QSharedPointer<Object3DRenderContextPerShareGroup>& shareGroup) {
  // qDebug() << "LineVisualizer::newPerShareGroup()";
  QSharedPointer<LineVisualizerPerShareGroup> ptr(
      new LineVisualizerPerShareGroup(shareGroup),
      [](QObject* obj) { obj->deleteLater(); });

  return ptr;
}

static QVector3D interpolateVectorAtPos(
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

static std::vector<QVector3D> traceStreamlineRK4(
    QVector3D startPos, int numSteps, float stepSize, QVector3D shape,
    QSharedPointer<vx::VolumeDataVoxel> dataX,
    QSharedPointer<vx::VolumeDataVoxel> dataY,
    QSharedPointer<vx::VolumeDataVoxel> dataZ,
    int direction = 1,  // +1 forward -1 backwards
    float maxMagnitude = 1.0f, float threshold = 0.0f) {
  std::vector<QVector3D> result;

  QVector3D currentPos = startPos;
  result.push_back(currentPos);

  // qDebug() << " Start RK4 at" << startPos;

  for (int i = 0; i < numSteps; ++i) {
    if (currentPos.x() < 0 || currentPos.x() >= shape.x() ||
        currentPos.y() < 0 || currentPos.y() >= shape.y() ||
        currentPos.z() < 0 || currentPos.z() >= shape.z())
      break;

    // vec(x)
    auto f = [&](const QVector3D& pos) {
      return interpolateVectorAtPos(pos.x(), pos.y(), pos.z(), dataX, dataY,
                                    dataZ);
    };
    QVector3D vec = f(currentPos);
    vec /= maxMagnitude;
    if (vec.length() < threshold) break;

    QVector3D v1 = f(currentPos);
    QVector3D v2 =
        f(currentPos + direction * 0.5f * stepSize * v1.normalized());
    QVector3D v3 =
        f(currentPos + direction * 0.5f * stepSize * v2.normalized());
    QVector3D v4 = f(currentPos + direction * stepSize * v3.normalized());

    QVector3D delta = (v1 + 2.0f * v2 + 2.0f * v3 + v4) / 6.0f;
    QVector3D deltaVoxel = direction * (delta.normalized() * stepSize);

    deltaVoxel = QVector3D(deltaVoxel.x() / dataX->gridSpacing()[0],
                           deltaVoxel.y() / dataX->gridSpacing()[1],
                           deltaVoxel.z() / dataX->gridSpacing()[2]);

    currentPos += deltaVoxel;
    // qDebug() << " RK4 result size:" << result.size();
    result.push_back(currentPos);
  }

  return result;
}

template <typename T>
T clamp(T value, T minVal, T maxVal) {
  return std::max(minVal, std::min(value, maxVal));
}

static std::vector<QVector3D> traceStreamlineRK45(
    QVector3D startPos, int maxSteps, float initialStepSize, QVector3D shape,
    QSharedPointer<vx::VolumeDataVoxel> dataX,
    QSharedPointer<vx::VolumeDataVoxel> dataY,
    QSharedPointer<vx::VolumeDataVoxel> dataZ, int direction = 1,
    float maxMagnitude = 1.0f, float threshold = 0.0f, float tolerance = 1e-3f,
    float minStep = 0.01f, float maxStep = 2.0f) {
  std::vector<QVector3D> result;
  QVector3D currentPos = startPos;
  float stepSize = initialStepSize;

  result.push_back(currentPos);

  auto f = [&](const QVector3D& pos) {
    if (pos.x() < 1 || pos.x() >= shape.x() - 1 || pos.y() < 1 ||
        pos.y() >= shape.y() - 1 || pos.z() < 1 || pos.z() >= shape.z() - 1)
      return QVector3D(0, 0, 0);

    QVector3D v =
        interpolateVectorAtPos(pos.x(), pos.y(), pos.z(), dataX, dataY, dataZ);
    if (!std::isfinite(v.x()) || !std::isfinite(v.y()) || !std::isfinite(v.z()))
      return QVector3D(0, 0, 0);
    return v / maxMagnitude;
  };

  for (int i = 0; i < maxSteps; ++i) {
    QVector3D k1 = f(currentPos);
    if (k1.length() < threshold) break;

    QVector3D k2 = f(currentPos + direction * stepSize * k1 * (1.0f / 4.0f));
    QVector3D k3 = f(currentPos + direction * stepSize *
                                      (3.0f / 32.0f * k1 + 9.0f / 32.0f * k2));
    QVector3D k4 =
        f(currentPos + direction * stepSize *
                           (1932.0f / 2197.0f * k1 - 7200.0f / 2197.0f * k2 +
                            7296.0f / 2197.0f * k3));
    QVector3D k5 =
        f(currentPos + direction * stepSize *
                           (439.0f / 216.0f * k1 - 8.0f * k2 +
                            3680.0f / 513.0f * k3 - 845.0f / 4104.0f * k4));
    QVector3D k6 =
        f(currentPos +
          direction * stepSize *
              (-8.0f / 27.0f * k1 + 2.0f * k2 - 3544.0f / 2565.0f * k3 +
               1859.0f / 4104.0f * k4 - 11.0f / 40.0f * k5));

    QVector3D y4 =
        currentPos + direction * stepSize *
                         (25.0f / 216.0f * k1 + 1408.0f / 2565.0f * k3 +
                          2197.0f / 4104.0f * k4 - 1.0f / 5.0f * k5);
    QVector3D y5 =
        currentPos +
        direction * stepSize *
            (16.0f / 135.0f * k1 + 6656.0f / 12825.0f * k3 +
             28561.0f / 56430.0f * k4 - 9.0f / 50.0f * k5 + 2.0f / 55.0f * k6);

    float error = (y5 - y4).length();
    if (!std::isfinite(error)) break;

    // Accept step if error within tolerance
    if (error < tolerance) {
      currentPos = y4;

      result.push_back(currentPos);
    }

    // Adjust step size (adaptive step control)
    float safety = 0.9f;
    float scale = safety * std::pow(tolerance / std::max(error, 1e-6f), 0.2f);
    stepSize = clamp(stepSize * scale, minStep, maxStep);

    if (currentPos.x() < 0 || currentPos.x() >= shape.x() ||
        currentPos.y() < 0 || currentPos.y() >= shape.y() ||
        currentPos.z() < 0 || currentPos.z() >= shape.z())
      break;
  }

  return result;
}

void LineVisualizer::render(
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

  auto psg =
      qSharedPointerDynamicCast<LineVisualizerPerShareGroup>(perShareGroup);
  if (!psg)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Not a LineVisualizerPerShareGroup");

  QMatrix4x4 matViewProj = context.projectionMatrix() * context.viewMatrix();

  static std::vector<std::vector<QVector3D>> allStreamlinesVis;

  vx::Vector<double, 3> spacingD = dataX->gridSpacing();
  vx::Vector<double, 3> originD = dataX->volumeOrigin();
  QVector3D spacing(spacingD[0], spacingD[1], spacingD[2]);
  QVector3D origin(originD[0], originD[1], originD[2]);

  QVector3D shape(dataX->arrayShape()[0], dataX->arrayShape()[1],
                  dataX->arrayShape()[2]);

  float visScale = 1e9f;
  QVector3D spacingVis = spacing * visScale;
  QVector3D originVis = origin * visScale;

  float stepSizeRK4 = spacing.x();

  // voxels which have magnitude lower than magnitudeThreshold are ignored
  float threshold = properties->magnitudeThreshold();
  // how big we choose the cube to be
  int step = properties->step();
  // how many steps we do each time when calling rk4 (how big the line is)
  int numSteps = properties->numSteps();
  // maximum number of lines
  int maxStreamlines = properties->maxStreamlines();

  QString colorMode = properties->colorMode();
  // vx::Color color = properties->fixedColor();

  // select if we use rk4 or rk45
  QString integrator = properties->integrator();

  // calculate max magnitude for normalisation
  float maxMagnitude = 0.0f;
  for (int x = 0; x < shape.x(); x += step) {
    for (int y = 0; y < shape.y(); y += step) {
      for (int z = 0; z < shape.z(); z += step) {
        QVector3D vec = interpolateVectorAtPos(x, y, z, dataX, dataY, dataZ);
        float mag = vec.length();
        if (mag > maxMagnitude) maxMagnitude = mag;
      }
    }
  }

  if (this->streamlinesDirty) {
    allStreamlinesVis.clear();
    int count = 0;

    QString selectGrid = properties->selectGrid();

    if (selectGrid == "Regular") {
      // we look into cubes with dimension of step x step x step where step is a
      // variable
      for (int x = 0; x < shape.x(); x += step) {
        for (int y = 0; y < shape.y(); y += step) {
          for (int z = 0; z < shape.z(); z += step) {
            // if there are too many streamlines generated, break
            if (count >= maxStreamlines) break;

            QVector3D seed(x, y, z);
            QVector3D vec =
                interpolateVectorAtPos(x, y, z, dataX, dataY, dataZ);

            if (maxMagnitude > 0.0f) vec /= maxMagnitude;

            // not all cubs should be taken into consideration because some are
            // too weak simple threshold
            if (vec.length() < threshold) continue;

            // line that goes in the direction the vector is pointing at

            std::vector<QVector3D> streamlineForward, streamlineBackward;

            if (integrator == "RK4") {
              streamlineForward =
                  traceStreamlineRK4(seed, numSteps, stepSizeRK4, shape, dataX,
                                     dataY, dataZ, +1, maxMagnitude, threshold);
              streamlineBackward =
                  traceStreamlineRK4(seed, numSteps, stepSizeRK4, shape, dataX,
                                     dataY, dataZ, -1, maxMagnitude, threshold);
            } else if (integrator == "RK45") {
              streamlineForward = traceStreamlineRK45(
                  seed, numSteps, stepSizeRK4, shape, dataX, dataY, dataZ, +1,
                  maxMagnitude, threshold);
              streamlineBackward = traceStreamlineRK45(
                  seed, numSteps, stepSizeRK4, shape, dataX, dataY, dataZ, -1,
                  maxMagnitude, threshold);
            }

            std::vector<QVector3D> streamline = streamlineBackward;
            // std::vector<QVector3D> streamline = streamlineForward;
            std::reverse(streamline.begin(), streamline.end());
            if (!streamlineForward.empty())
              streamline.insert(streamline.end(), streamlineForward.begin() + 1,
                                streamlineForward.end());

            if (streamline.size() < 2) continue;

            std::vector<QVector3D> transformed;
            for (const auto& p : streamline) {
              transformed.push_back(originVis + spacingVis * p);
            }
            qDebug() << "🔹 Streamline #" << count << " has" << streamline.size()
                     << "points:";
            for (const auto& p : streamline) {
              qDebug() << "   →" << p;
            }

            allStreamlinesVis.push_back(transformed);
            qDebug() << "Number of streamlines to draw:"
                     << allStreamlinesVis.size();

            ++count;
          }
        }
      }
    } else if (selectGrid == "Unstructured") {
      std::uniform_int_distribution<int> distribution(0, step - 1);
      for (int x = 0; x < shape.x(); x += step) {
        for (int y = 0; y < shape.y(); y += step) {
          for (int z = 0; z < shape.z(); z += step) {
            if (count >= maxStreamlines) break;

            std::seed_seq seedSeq{x, y, z};
            std::mt19937 generator(seedSeq);
            int offsetX = distribution(generator);
            int offsetY = distribution(generator);
            int offsetZ = distribution(generator);

            QVector3D seed(x + offsetX, y + offsetY, z + offsetZ);
            QVector3D vec = interpolateVectorAtPos(seed.x(), seed.y(), seed.z(),
                                                   dataX, dataY, dataZ);
            if (maxMagnitude > 0.0f) vec /= maxMagnitude;
            if (vec.length() < threshold) continue;

            std::vector<QVector3D> streamlineForward, streamlineBackward;

            if (integrator == "RK4") {
              streamlineForward =
                  traceStreamlineRK4(seed, numSteps, stepSizeRK4, shape, dataX,
                                     dataY, dataZ, +1, maxMagnitude, threshold);
              streamlineBackward =
                  traceStreamlineRK4(seed, numSteps, stepSizeRK4, shape, dataX,
                                     dataY, dataZ, -1, maxMagnitude, threshold);
            } else if (integrator == "RK45") {
              streamlineForward = traceStreamlineRK45(
                  seed, numSteps, stepSizeRK4, shape, dataX, dataY, dataZ, +1,
                  maxMagnitude, threshold);
              streamlineBackward = traceStreamlineRK45(
                  seed, numSteps, stepSizeRK4, shape, dataX, dataY, dataZ, -1,
                  maxMagnitude, threshold);
            }

            std::vector<QVector3D> streamline = streamlineBackward;
            std::reverse(streamline.begin(), streamline.end());
            if (!streamlineForward.empty())
              streamline.insert(streamline.end(), streamlineForward.begin() + 1,
                                streamlineForward.end());
            if (streamline.size() < 2) continue;

            std::vector<QVector3D> transformed;
            for (const auto& p : streamline)
              transformed.push_back(originVis + spacingVis * p);

            allStreamlinesVis.push_back(transformed);
            ++count;
          }
        }
      }
    }
    this->streamlinesDirty = false;
  }

  // DRAWING PART

  OpenGLDrawUtils::PrimitiveBuffer drawingBuffer;

  float scale = properties->scaleFactor();
  for (const auto& streamline : allStreamlinesVis) {
    if (streamline.size() < 2) continue;

    for (size_t i = 1; i < streamline.size(); ++i) {
      QVector3D p1 = streamline[i - 1] * scale;
      QVector3D p2 = streamline[i] * scale;

      vx::Color color;
      if (colorMode == "Fixed") {
        color = properties->fixedColor();
      } else {
        QVector3D pMidVis = (streamline[i] + streamline[i - 1]) / 2.0f;
        QVector3D pMid = (pMidVis - originVis) / spacingVis;

        QVector3D fieldVec = interpolateVectorAtPos(
            pMid.x(), pMid.y(), pMid.z(), dataX, dataY, dataZ);
        // qDebug() << "Interpolated vec at" << pMid << ":" << fieldVec;

        float magnitude = fieldVec.length();

        // float normMag = std::log1p(magnitude) / std::log1p(0.6f);
        float normMag = magnitude / maxMagnitude;
        normMag = std::min(normMag, 1.0f);

        color = vx::Color(1.0f - normMag, normMag, 0.0f);
      }

      drawingBuffer.addLine(color, p1, p2);
    }
  }

  QMatrix4x4 modelMatrix;
  modelMatrix.setToIdentity();

  psg->drawUtils->draw(drawingBuffer, modelMatrix, matViewProj, nullptr);

  psg->functions.glEnable(GL_DEPTH_TEST);
  psg->functions.glEnable(GL_CULL_FACE);
  psg->functions.glCullFace(GL_BACK);
  psg->functions.glFrontFace(GL_CCW);
}

BoundingBox3D LineVisualizer::getBoundingBox() {
  auto volumeSeries =
      dynamic_cast<VolumeSeriesNode*>(this->properties->field());
  if (!volumeSeries) {
    // qDebug() << "Bounding box: No volume series";
    return BoundingBox3D::empty();
  }

  auto dataSeries =
      qSharedPointerDynamicCast<VolumeSeriesData>(volumeSeries->data());
  if (!dataSeries || dataSeries->dimensionCount() != 1) {
    // qDebug() << "Bounding box: Did not get data with one dimension";
    return BoundingBox3D::empty();
  }
  auto dimension = dataSeries->dimensions()[0];

  if (dimension->entries().size() != 3) {
    // qDebug() << "Bounding box: Did not get exactly 3 values";
    return BoundingBox3D::empty();
  }
  auto dataX =
      qSharedPointerDynamicCast<VolumeData>(dataSeries->lookupEntry({0}));
  if (!dataX) {
    // qDebug() << "Bounding box: Got non-volume data";
    return BoundingBox3D::empty();
  }
  return BoundingBox3D::point(dataX->volumeOrigin()) +
         BoundingBox3D::point(dataX->volumeOrigin() + dataX->volumeSize());
}
