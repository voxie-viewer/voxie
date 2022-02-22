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

#include "VolumeImageRenderer.hpp"

#include <VoxieBackend/Data/VolumeDataVoxelInst.hpp>

#include <cmath>

using namespace vx;

Ray::Ray() {}

static int intersectBox(Ray* ray, QVector3D boxmin, QVector3D boxmax,
                        float* tnear, float* tfar);

ImageRender::ImageRender(
    QImage* inputImage, QSharedPointer<VolumeDataVoxel> sourceVolume,
    QMatrix4x4 invViewProjection, QVector2D voxelRange, int numSamples,
    float raytraceScale, bool useAntiAliazing, QVector3D spacing,
    QColor ambientLight, float ambientlightScale, float diffuselightScale,
    QList<LightSource*>* lightSourceList, bool useAbsuluteShadingValue,
    ThreadSafe_MxN_Matrix* randomValues) {
  this->inputImage = inputImage;
  this->sourceVolume = sourceVolume;
  this->invViewProjection = invViewProjection;
  this->voxelRange = voxelRange;
  this->numSamples = numSamples;
  this->raytraceScale = raytraceScale;
  this->useAntiAliazing = useAntiAliazing;
  this->spacing = spacing;
  this->ambientLight = ambientLight;
  this->ambientlightScale = ambientlightScale;
  this->diffuselightScale = diffuselightScale;
  this->lightSourceList = lightSourceList;
  this->useAbsuluteShadingValue = useAbsuluteShadingValue;
  this->randomValues = randomValues;
}

void ImageRender::render() {
  QVector4D tmpVector = invViewProjection.column(2);
  QVector3D origin =
      QVector3D(tmpVector.x(), tmpVector.y(), tmpVector.z()) / tmpVector.w();
  ThreadSafeQImage* resultImage = new ThreadSafeQImage(this->inputImage);
  QSharedPointer<ThreadSafeQImage> sharedResultImage =
      QSharedPointer<ThreadSafeQImage>(resultImage);
  QThreadPool thPool;
  for (int x = 0; x < resultImage->getImageWidth(); x++) {
    RenderTask* renderTask = new RenderTask(
        x, &origin, sharedResultImage, this->sourceVolume,
        this->invViewProjection, this->voxelRange, this->numSamples,
        this->raytraceScale, this->useAntiAliazing, this->spacing,
        this->ambientLight, this->ambientlightScale, this->diffuselightScale,
        this->lightSourceList, this->useAbsuluteShadingValue,
        this->randomValues);
    thPool.start(renderTask);
  }
  thPool.waitForDone(-1);
  *this->inputImage = *sharedResultImage->getImage();
  Q_EMIT generationDone();
}

RenderTask::RenderTask(int x, QVector3D* origin,
                       QSharedPointer<ThreadSafeQImage> sharedResultImage,
                       QSharedPointer<VolumeDataVoxel> sourceVolume,
                       QMatrix4x4 invViewProjection, QVector2D voxelRange,
                       int numSamples, float raytraceScale,
                       bool useAntiAliazing, QVector3D spacing,
                       QColor ambientLight, float ambientlightScale,
                       float diffuselightScale,
                       QList<LightSource*>* lightSourceList,
                       bool useAbsuluteShadingValue,
                       ThreadSafe_MxN_Matrix* randomValues) {
  this->x = x;
  this->origin = origin;
  this->sharedResultImage = sharedResultImage;
  this->sourceVolume = sourceVolume;
  this->invViewProjection = invViewProjection;
  this->voxelRange = voxelRange;
  this->numSamples = numSamples;
  this->raytraceScale = raytraceScale;
  this->useAntialiazing = useAntiAliazing;
  this->spacing = spacing;
  this->ambientLight = ambientLight;
  this->ambientlightScale = ambientlightScale;
  this->diffuselightScale = diffuselightScale;
  this->lightSourceList = lightSourceList;
  this->useAbsuluteShadingValue = useAbsuluteShadingValue;
  this->randomValues = randomValues;
}

void RenderTask::run() {
  int y = 0;

  for (; y < this->sharedResultImage->getImageHeight(); y++) {
    float fx =
        ((float)this->x + 0.5f) / this->sharedResultImage->getImageWidth() * 2 -
        1;
    float fy =
        1 - ((float)y + 0.5f) / this->sharedResultImage->getImageHeight() * 2;

    QVector4D temp0 = QVector4D(fx, fy, 0, 1);
    QVector4D temp1 = QVector4D(fx, fy, 1, 1);
    QVector4D Mtemp0, Mtemp1;

    Mtemp0.setX(QVector4D::dotProduct(temp0, this->invViewProjection.row(0)));
    Mtemp0.setY(QVector4D::dotProduct(temp0, this->invViewProjection.row(1)));
    Mtemp0.setZ(QVector4D::dotProduct(temp0, this->invViewProjection.row(2)));
    Mtemp0.setW(QVector4D::dotProduct(temp0, this->invViewProjection.row(3)));

    Mtemp1.setX(QVector4D::dotProduct(temp1, this->invViewProjection.row(0)));
    Mtemp1.setY(QVector4D::dotProduct(temp1, this->invViewProjection.row(1)));
    Mtemp1.setZ(QVector4D::dotProduct(temp1, this->invViewProjection.row(2)));
    Mtemp1.setW(QVector4D::dotProduct(temp1, this->invViewProjection.row(3)));

    Ray ray;
    ray.origin = this->origin;
    ray.direction =
        QVector3D(Mtemp0.w() * Mtemp1.x() - Mtemp1.w() * Mtemp0.x(),
                  Mtemp0.w() * Mtemp1.y() - Mtemp1.w() * Mtemp0.y(),
                  Mtemp0.w() * Mtemp1.z() - Mtemp1.w() * Mtemp0.z());

    ray.direction.normalize();

    float tnear, tfar;
    int rayresult = intersectBox(
        &ray, sourceVolume->origin(),
        sourceVolume->origin() + sourceVolume->getDimensionsMetric(), &tnear,
        &tfar);

    if (rayresult == 0) {
      // Didn't hit anything
      this->sharedResultImage->setNoHitPixel(this->x, y);
      continue;
    }

    // Clamp to camera
    if (tnear < 0.0f) {
      tnear = 0.0f;
    }
    if (tfar < tnear) {
      // We hit the box completly behind us. tnear and tfar reversed
      this->sharedResultImage->setDebugPixel(this->x, y);
      continue;
    }

    float accu = 0.0f;

    QVector3D diffuseResultColor = QVector3D(0.0f, 0.0f, 0.0f);
    QVector3D ambientResultColor = QVector3D(0.0f, 0.0f, 0.0f);

    for (int sampleId = 0; sampleId < this->numSamples; sampleId++) {
      float iRandom = (float)sampleId;
      if (useAntialiazing && randomValues) {
        float randomNumber = randomValues->getValue(y, x);
        iRandom += randomNumber;
      } else {
        iRandom += 0.5f;
      }

      float delta = iRandom / (float)(numSamples);

      QVector3D position =
          *ray.origin + (tnear + delta * (tfar - tnear)) * ray.direction;

      float voxel =
          sourceVolume->performInGenericContext([&position](auto& data) {
            float tmp =
                data.getVoxelMetric(position, vx::InterpolationMethod::Linear);
            if (std::isnan(tmp)) {
              tmp = 0;
            }
            return tmp;
          });

      float accuTmp = (voxel - this->voxelRange.x()) /
                      (this->voxelRange.y() - this->voxelRange.x());
      accu += fmin(fmax(accuTmp, 0.0f), 1.0f);  // accu: 0.0f to 1.0f

      // #### Shading ####

      float x1 =
          sourceVolume->performInGenericContext([&position, this](auto& data) {
            float tmp = data.getVoxelMetric(position.x() + this->spacing.x(),
                                            position.y(), position.z(),
                                            vx::InterpolationMethod::Linear);
            if (std::isnan(tmp)) {
              tmp = 0;
            }
            return tmp;
          });

      float x2 =
          sourceVolume->performInGenericContext([&position, this](auto& data) {
            float tmp = data.getVoxelMetric(position.x() - this->spacing.x(),
                                            position.y(), position.z(),
                                            vx::InterpolationMethod::Linear);
            if (std::isnan(tmp)) {
              tmp = 0;
            }
            return tmp;
          });

      float x_Gradient = (x1 - x2) / (2 * this->spacing.x());

      float y1 =
          sourceVolume->performInGenericContext([&position, this](auto& data) {
            float tmp = data.getVoxelMetric(
                position.x(), position.y() + this->spacing.y(), position.z(),
                vx::InterpolationMethod::Linear);
            if (std::isnan(tmp)) {
              tmp = 0;
            }
            return tmp;
          });

      float y2 =
          sourceVolume->performInGenericContext([&position, this](auto& data) {
            float tmp = data.getVoxelMetric(
                position.x(), position.y() - this->spacing.y(), position.z(),
                vx::InterpolationMethod::Linear);
            if (std::isnan(tmp)) {
              tmp = 0;
            }
            return tmp;
          });

      float y_Gradient = (y1 - y2) / (2 * this->spacing.y());

      float z1 =
          sourceVolume->performInGenericContext([&position, this](auto& data) {
            float tmp = data.getVoxelMetric(position.x(), position.y(),
                                            position.z() + this->spacing.z(),
                                            vx::InterpolationMethod::Linear);
            if (std::isnan(tmp)) {
              tmp = 0;
            }
            return tmp;
          });

      float z2 =
          sourceVolume->performInGenericContext([&position, this](auto& data) {
            float tmp = data.getVoxelMetric(position.x(), position.y(),
                                            position.z() - this->spacing.z(),
                                            vx::InterpolationMethod::Linear);
            if (std::isnan(tmp)) {
              tmp = 0;
            }
            return tmp;
          });

      float z_Gradient = (z1 - z2) / 2 * this->spacing.z();

      QVector3D normalVec = QVector3D(x_Gradient, y_Gradient, z_Gradient);
      normalVec *= -1;

      //### Ambient-Shading ###
      QVector3D ambientColorF = QVector3D(
          ambientLight.redF(), ambientLight.greenF(), ambientLight.blueF());
      float normalVecLength = normalVec.length();

      if (normalVecLength >
          0) {  // this if is nessesary to prevent nan result corrupt result
        ambientResultColor += ambientColorF * normalVecLength;
      }

      // ### Diffuse-Shading ###
      if (lightSourceList) {
        for (int lightSourceId = 0; lightSourceId < lightSourceList->size();
             lightSourceId++) {
          auto lightSource = lightSourceList->at(lightSourceId);
          QVector3D lightVec = QVector3D(0, 0, 0);

          if (lightSource && lightSource->isActive()) {
            QVector4D lightPos = lightSource->getPosition();

            float w2 = 1.0f;  // since position is only a QVector3D, its w = 1.
            lightVec =
                QVector3D(lightPos.x() * w2 - position.x() * lightPos.w(),
                          lightPos.y() * w2 - position.y() * lightPos.w(),
                          lightPos.z() * w2 - position.z() * lightPos.w());
            lightVec.normalize();
            float shadingValue = QVector3D::dotProduct(lightVec, normalVec);

            if (useAbsuluteShadingValue) {
              shadingValue = fabs(shadingValue);
            } else {
              if (shadingValue <= 0) {
                continue;
              }
            }
            QColor lightColor = lightSource->getLightColor();
            QVector3D lightColorF = QVector3D(
                lightColor.redF(), lightColor.greenF(), lightColor.blueF());
            diffuseResultColor += lightColorF * shadingValue;
          }
        }  // lightSourceList for-loop END
      }
    }  // samples for-loop END

    // Scaling Constants
    const float ambientLightFactor = 1.0f / 50000.0f;
    const float diffuseLightFactor = 1.0f / 21097.04641f;
    const float grayValueFactor = 0.25f;

    // Scale accumulator
    accu *= (this->raytraceScale * grayValueFactor) / (tfar - tnear) /
            this->numSamples;

    QVector3D grayValue = QVector3D(accu, accu, accu);

    // Scale Colors
    ambientResultColor *= (this->ambientlightScale * ambientLightFactor) /
                          (tfar - tnear) / this->numSamples;
    diffuseResultColor *= (this->diffuselightScale * diffuseLightFactor) /
                          (tfar - tnear) / this->numSamples;

    QVector3D resultColorF =
        grayValue + ambientResultColor + diffuseResultColor;

    // clamp resultColor
    resultColorF.setX(fmin(fmax(resultColorF.x(), 0.0f), 1.0f));
    resultColorF.setY(fmin(fmax(resultColorF.y(), 0.0f), 1.0f));
    resultColorF.setZ(fmin(fmax(resultColorF.z(), 0.0f), 1.0f));

    QColor resultColor;
    resultColor.setRgbF(resultColorF.x(), resultColorF.y(), resultColorF.z());
    this->sharedResultImage->setImagePixel(this->x, y, resultColor);
  }
}

ThreadSafeQImage::ThreadSafeQImage(QImage* inputImage) {
  this->resultImage = new QImage(*inputImage);
  this->noHitColor = QColor(0, 0, 0);
  this->debugColor = QColor(0, 255, 0);
}

void ThreadSafeQImage::setImagePixel(int x, int y, QColor resultColor) {
  QMutexLocker locker(&mutex);

  this->resultImage->setPixel(x, y, resultColor.rgb());
}
void ThreadSafeQImage::setNoHitPixel(int x, int y) {
  QMutexLocker locker(&mutex);

  this->resultImage->setPixel(x, y, this->noHitColor.rgb());
}
void ThreadSafeQImage::setDebugPixel(int x, int y) {
  QMutexLocker locker(&mutex);

  this->resultImage->setPixel(x, y, this->debugColor.rgb());
}

QImage* ThreadSafeQImage::getImage() {
  QMutexLocker locker(&mutex);

  return this->resultImage;
}

int ThreadSafeQImage::getImageWidth() {
  QMutexLocker locker(&mutex);

  return this->resultImage->width();
}

int ThreadSafeQImage::getImageHeight() {
  QMutexLocker locker(&mutex);

  return this->resultImage->height();
}

int intersectBox(Ray* ray, QVector3D boxmin, QVector3D boxmax, float* tnear,
                 float* tfar) {
  // compute intersection of ray with all six bbox planes
  QVector3D invR = QVector3D(1.0f, 1.0f, 1.0f) / ray->direction;
  QVector3D tbot = invR * (boxmin - *ray->origin);
  QVector3D ttop = invR * (boxmax - *ray->origin);

  // re-order intersections to find smallest and largest on each axis

  QVector3D tmin;
  tmin.setX(std::min(ttop.x(), tbot.x()));
  tmin.setY(std::min(ttop.y(), tbot.y()));
  tmin.setZ(std::min(ttop.z(), tbot.z()));

  QVector3D tmax;
  tmax.setX(std::max(ttop.x(), tbot.x()));
  tmax.setY(std::max(ttop.y(), tbot.y()));
  tmax.setZ(std::max(ttop.z(), tbot.z()));

  // find the largest tmin and the smallest tmax
  float largest_tmin = std::max(std::max(tmin.x(), tmin.y()), tmin.z());
  float smallest_tmax = std::min(std::min(tmax.x(), tmax.y()), tmax.z());

  *tnear = largest_tmin;
  *tfar = smallest_tmax;

  return smallest_tmax > largest_tmin;
}
