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

// QDBusConnection should be included as early as possible:
// https://bugreports.qt.io/browse/QTBUG-48351 /
// https://bugreports.qt.io/browse/QTBUG-48377
#include <QtDBus/QDBusConnection>

#include <Voxie/Gui/ObjectProperties.hpp>

#include <Voxie/Vis/View3D.hpp>
#include <Voxie/Vis/VisualizerNode.hpp>

#include <PluginVis3D/CamProperties.hpp>
#include <PluginVis3D/LightSourceProperties.hpp>
#include <PluginVis3D/RenderImplementationSelection.hpp>
#include <PluginVis3D/ThreadSafe_MxN_Matrix.hpp>

#include <VoxieBackend/lib/CL/cl-patched.hpp>

#include <QtGui/QImage>

#include <QCheckBox>
#include <QCryptographicHash>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSlider>

namespace vx {
class Node;
}  // namespace vx

class RandomNumberGenerationTask : public QRunnable {
 public:
  RandomNumberGenerationTask(int x, int height,
                             ThreadSafe_MxN_Matrix* randomValues,
                             signed long numSamples);

 private:
  void run() override;
  int x;
  int height;
  ThreadSafe_MxN_Matrix* randomValues;
  signed long numSamples;
};

class VolumeRenderingVisualizer;

/**
 * @brief The VolumeRenderingView class contains information about the currently
 * selected dataset.
 */
class VolumeRenderingView : public QWidget {
  Q_OBJECT

  friend class VolumeRenderingVisualizer;

 Q_SIGNALS:
  void beginnRender();
  void generationDone();
  void ambientLightRequest();
  void lightSourceListRequest();

 public Q_SLOTS:
  void ambientLightRequestResponsed(QColor ambientLight);
  void lightSourcesListRequestResponsed(QList<LightSource*>* lightSourcesList);

  void ambientlightScaleChanged(int value);
  void diffuselightScaleChanged(int value);
  void useAbsuluteShadingValueChaned(bool value);

 private:
  // data
  VolumeRenderingVisualizer* visualizer;
  QImage image;
  cl::Kernel kernel;
  cl::Image2D clImage;
  QPoint mouseLast;
  vx::visualization::View3D* view3d;

  float ambientlightScale = 0.8f;
  float diffuselightScale = 0.8f;

  QWidget* sidePanel;
  QCheckBox* antiAliazingCheckBox;
  QRadioButton *radioQ0, *radioQ1, *radioQ2, *radioQ3;
  QSlider *minSlider, *maxSlider, *scaleSlider;
  QLineEdit *minField, *maxField, *scaleField;

  bool generating = false;
  bool generationRequested = false;

  bool useGPU = true;
  bool useAntiAliazing = true;
  bool updateRandomValues = true;
  ThreadSafe_MxN_Matrix* randomValues = nullptr;
  bool useAbsuluteShadingValue = false;

  void setUseGPU(bool useGPU);
  bool getUseGPU();

  void setUseAntiAliazing(bool useAntiAliazing);

  QColor ambientLight;
  QList<LightSource*>* lightSourcesList = nullptr;

  QSharedPointer<vx::VolumeData> data() {
    auto volumeData = dataSet();
    return volumeData ? volumeData->volumeData()
                      : QSharedPointer<vx::VolumeData>();
  }

  /**
   * @brief If changes have occurred during rendering, another rendering will be
   * triggered with the changes.
   */
  void reRendering();

  void paintResult(QSharedPointer<QImage> resultImage);

  void updateButton(bool stub) {
    (void)stub;
    this->update();
  }
  void updateSlider();
  void updateMinSlider();
  void updateMaxSlider();
  void updateField();

  inline vx::VolumeNode* dataSet();

  void setRandomValues(ThreadSafe_MxN_Matrix* randomValues,
                       signed long numSamples);

 public:
  explicit VolumeRenderingView(VolumeRenderingVisualizer* visualizer);
  ~VolumeRenderingView();

  /**
   * @brief dataSetChanged is called whenever the parentNode of the
   * VisualizerNode have changed. Will update the shown VolumeNode when
   * necessary.
   */
  void dataSetChanged();

  virtual void mousePressEvent(QMouseEvent* event) override;
  virtual void mouseMoveEvent(QMouseEvent* event) override;
  virtual void wheelEvent(QWheelEvent* event) override;
  virtual void paintEvent(QPaintEvent* event) override;
  virtual void resizeEvent(QResizeEvent* event) override;

  void setMinimum(int min);
  void setMaximum(int max);
  void setScale(int scale);

  int getMinimum();
  int getMaximum();
  int getScale();
};

/**
 * @brief This class is the overall container for the operations related to the
 * volumerendering. It basically contains the UI elements, responds to user
 * interaction and contains the @link VolumeRenderingView which again is
 * responsible for the rendering of the 3D scene.
 */
class VolumeRenderingVisualizer : public vx::VisualizerNode {
  Q_OBJECT

 private:
  vx::VolumeNode* mainDataset;
  VolumeRenderingView* view;
  ObjectProperties* objProp;
  LightSourceProperties* lightSrcProp;
  RenderImplementationSelection* renderImpl;

  /**
   * @brief settingUpRenderImplementationSelection contains the name setting and
   * connections for settingUpRenderImplementationSelection.
   * @param renderImpl
   */
  void settingUpRenderImplementationSelection(
      RenderImplementationSelection* renderImpl);

  /**
   * @brief settingUpCameraProperties contains the name setting and connections
   * for CameraProperties
   * @param camProp
   */
  void settingUpCameraProperties(CamProperties* camProp);

  /**
   * @brief settingUpObjectProperties contains the name setting and connections
   * for ObjectProperties
   */
  void settingUpObjectProperties();

  /**
   * @brief settingUpLightSourceProperties contains the name setting and
   * connections for ObjectProperties
   */
  void settingUpLightSourceProperties();

  void positionChanged(QVector3D offset);
  void rotationChanged(QQuaternion rotation);

  /**
   * @brief dataSetChanged is called whenever the parentNode of the
   * VisualizerNode have changed. Will update the shown VolumeNode when
   * necessary.
   */
  void dataSetChanged(vx::Node* node);

 public:
  explicit VolumeRenderingVisualizer();

  FACTORY_VISUALIZERMODULE_HPP(VolumeRendering)

  virtual vx::VolumeNode* dataSet() final { return this->mainDataset; }

  QWidget* mainView() override { return view; }

  void setVolumeMin(int min);
  void setVolumeMax(int max);
  void setVolumeScale(int scale);

 public Q_SLOTS:
  void positionEditedFromGUI(QVector3D);
  void rotationEditedFromGUI(QQuaternion rot);
 Q_SIGNALS:
  void noOpenClAvailable();
};

inline vx::VolumeNode* VolumeRenderingView::dataSet() {
  return this->visualizer->dataSet();
}
