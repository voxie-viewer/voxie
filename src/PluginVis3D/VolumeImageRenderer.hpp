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

#include <QColor>
#include <QDebug>
#include <QImage>
#include <QMatrix4x4>
#include <QSharedPointer>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>
#include <QVector>

#include <PluginVis3D/LightSource.hpp>
#include <PluginVis3D/ThreadSafe_MxN_Matrix.hpp>
#include <VoxieBackend/Data/VolumeDataVoxel.hpp>

class ThreadSafeQImage : public QObject {
 public:
  ThreadSafeQImage(QImage* inputImage);
  void setImagePixel(int x, int y, QColor resultColor);
  void setNoHitPixel(int x, int y);
  void setDebugPixel(int x, int y);
  QImage* getImage();
  int getImageWidth();
  int getImageHeight();

 private:
  QImage* resultImage;
  QColor noHitColor;
  QColor debugColor;
  QMutex mutex;
};

class ImageRender : public QObject {
  Q_OBJECT

 Q_SIGNALS:
  void generationDone();

 public:
  ImageRender(QImage* inputImage,
              QSharedPointer<vx::VolumeDataVoxel> sourceVolume,
              QMatrix4x4 invViewProjection, QVector2D voxelRange,
              int numSamples, float raytraceScale, bool useAntiAliazing,
              QVector3D spacing, QColor ambientLight, float ambientlightScale,
              float diffuselightScale, QList<LightSource*>* lightSourceList,
              bool useAbsuluteShadingValue,
              ThreadSafe_MxN_Matrix* randomValues);

  void render();

 private:
  QImage* inputImage;
  QSharedPointer<vx::VolumeDataVoxel> sourceVolume;
  QMatrix4x4 invViewProjection;
  QVector2D voxelRange;
  int numSamples;
  float raytraceScale;
  bool useAntiAliazing;
  QVector3D spacing;
  QColor ambientLight;
  float ambientlightScale;
  float diffuselightScale;
  QList<LightSource*>* lightSourceList;
  bool useAbsuluteShadingValue;
  ThreadSafe_MxN_Matrix* randomValues;
};

class Ray : private QObject {
 public:
  Ray();
  QVector3D* origin;
  QVector3D direction;
};

class RenderTask : public QRunnable {
 public:
  RenderTask(int x, QVector3D* origin,
             QSharedPointer<ThreadSafeQImage> sharedResultImage,
             QSharedPointer<vx::VolumeDataVoxel> sourceVolume,
             QMatrix4x4 invViewProjection, QVector2D voxelRange, int numSamples,
             float raytraceScale, bool useAntiAliazing, QVector3D spacing,
             QColor ambientLight, float ambientlightScale,
             float diffuselightScale, QList<LightSource*>* lightSourceList,
             bool useAbsuluteShadingValue, ThreadSafe_MxN_Matrix* randomValues);

 private:
  void run() override;
  int x;
  QVector3D* origin;
  QSharedPointer<ThreadSafeQImage> sharedResultImage;
  QSharedPointer<vx::VolumeDataVoxel> sourceVolume;
  QMatrix4x4 invViewProjection;
  QVector2D voxelRange;
  int numSamples;
  float raytraceScale;
  bool useAntialiazing;
  QVector3D spacing;
  QColor ambientLight;
  float ambientlightScale;
  float diffuselightScale;
  QList<LightSource*>* lightSourceList;
  bool useAbsuluteShadingValue;
  ThreadSafe_MxN_Matrix* randomValues;
};
